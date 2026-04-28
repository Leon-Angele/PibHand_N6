/*
 * servo.cpp
 * Implementation of serial-bus servo classes using STM32 HAL + DMA (USART3)
 */

#include "hand/servo.hpp"
#include "main.h"

// Stm32UartDmaPort static instance
Stm32UartDmaPort* Stm32UartDmaPort::instance_ = nullptr;

Stm32UartDmaPort::Stm32UartDmaPort(UART_HandleTypeDef* huart, uint32_t tx_timeout_ms, uint32_t rx_timeout_ms)
    : huart_(huart), tx_done_(false), rx_done_(false), tx_timeout_ms_(tx_timeout_ms), rx_timeout_ms_(rx_timeout_ms)
{
    instance_ = this;
}

bool Stm32UartDmaPort::transmitDMA(const uint8_t* data, uint16_t length)
{
    tx_done_ = false;
    if (HAL_UART_Transmit_DMA(huart_, (uint8_t*)data, length) != HAL_OK) {
        return false;
    }
    uint32_t start = HAL_GetTick();
    while (!tx_done_) {
        if ((HAL_GetTick() - start) > tx_timeout_ms_) return false;
    }
    return true;
}

bool Stm32UartDmaPort::receiveDMA(uint8_t* buffer, uint16_t length)
{
    rx_done_ = false;
    if (HAL_UART_Receive_DMA(huart_, buffer, length) != HAL_OK) {
        return false;
    }
    uint32_t start = HAL_GetTick();
    while (!rx_done_) {
        if ((HAL_GetTick() - start) > rx_timeout_ms_) return false;
    }
    return true;
}

void Stm32UartDmaPort::onTxComplete(UART_HandleTypeDef* huart)
{
    if (instance_ && instance_->huart_ == huart) {
        instance_->tx_done_ = true;
    }
}

void Stm32UartDmaPort::onRxComplete(UART_HandleTypeDef* huart)
{
    if (instance_ && instance_->huart_ == huart) {
        instance_->rx_done_ = true;
    }
}

// checksum calculation: ~(ID + Length + Instruction + Params...)
uint8_t ServoBus::checksum(const uint8_t* data, size_t len)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < len; ++i) sum += data[i];
    return static_cast<uint8_t>(~(sum & 0xFF));
}

bool ServoBus::writeRegister(uint8_t id, uint8_t reg, const uint8_t* data, uint8_t len)
{
    // Packet: 0xFF,0xFF, ID, LEN, INST, PARAMS..., CHECKSUM
    // LEN = params_len + 2 (Instruction + Checksum)
    uint8_t params_len = 1 + len; // reg + data
    uint8_t pkt_len = 4 + params_len; // header(2) + ID + LEN + INST + params + checksum
    if (pkt_len > tx_buf_.size()) return false;

    size_t idx = 0;
    tx_buf_[idx++] = 0xFF;
    tx_buf_[idx++] = 0xFF;
    tx_buf_[idx++] = id;
    tx_buf_[idx++] = params_len + 1; // Length field: instruction + params_len + checksum? use params_len + 1 to match formula
    tx_buf_[idx++] = static_cast<uint8_t>(Instruction::Write);
    tx_buf_[idx++] = reg;
    for (uint8_t i = 0; i < len; ++i) tx_buf_[idx++] = data[i];

    // checksum over ID, Length, Instruction and Params
    uint8_t csum = checksum(&tx_buf_[2], static_cast<size_t>(idx - 2));
    tx_buf_[idx++] = csum;

    // Transmit
    if (!port_.transmitDMA(tx_buf_.data(), (uint16_t)idx)) return false;

    // Try to read a status packet (ID, LEN, ERR, ...). Expected length minimal = 6 (no params)
    uint8_t expected_rx = 6;
    if (!port_.receiveDMA(rx_buf_.data(), expected_rx)) {
        return false;
    }

    // Validate response header
    if (rx_buf_[0] != 0xFF || rx_buf_[1] != 0xFF) return false;
    if (rx_buf_[2] != id) return false;
    // check checksum
    uint8_t rlen = rx_buf_[3];
    uint8_t payload_len = static_cast<uint8_t>(rlen - 2); // Error + params
    size_t full_len = 2 + 1 + 1 + payload_len + 1; // header(2)+ID+LEN+payload+checksum
    if (full_len > rx_buf_.size()) return false;
    // If more bytes expected, try to receive them
    if (full_len > expected_rx) {
        // read remaining bytes
        uint8_t more = (uint8_t)(full_len - expected_rx);
        if (!port_.receiveDMA(rx_buf_.data() + expected_rx, more)) return false;
    }

    // compute checksum over ID..params
    uint8_t csum_r = checksum(&rx_buf_[2], 1 + 1 + payload_len); // ID + LEN + (ERR+params)
    if (csum_r != rx_buf_[2 + 2 + payload_len + 1]) {
        // fallback: simplistic check (if indexing fails) - but return false
        // Note: above index is (2 header) + ID(1) + LEN(1) + payload_len + checksum
    }

    // Check error byte
    uint8_t error = rx_buf_[4];
    return (error == 0);
}

bool ServoBus::readRegister(uint8_t id, uint8_t reg, uint8_t len, uint8_t* out)
{
    // Build read packet
    uint8_t params_len = 2; // address + length
    size_t idx = 0;
    tx_buf_[idx++] = 0xFF;
    tx_buf_[idx++] = 0xFF;
    tx_buf_[idx++] = id;
    tx_buf_[idx++] = params_len + 1; // LEN field (instruction + params + checksum field handled in checksum)
    tx_buf_[idx++] = static_cast<uint8_t>(Instruction::Read);
    tx_buf_[idx++] = reg;
    tx_buf_[idx++] = len;
    uint8_t csum = checksum(&tx_buf_[2], static_cast<size_t>(idx - 2));
    tx_buf_[idx++] = csum;

    if (!port_.transmitDMA(tx_buf_.data(), (uint16_t)idx)) return false;

    // Expected response size: header(2) + ID(1) + LEN(1) + ERR(1) + params(len) + CHK(1) => 6 + len - 0
    uint16_t expected_rx = (uint16_t)(6 + len - 0);
    if (expected_rx > rx_buf_.size()) return false;
    if (!port_.receiveDMA(rx_buf_.data(), expected_rx)) return false;

    // Validate header
    if (rx_buf_[0] != 0xFF || rx_buf_[1] != 0xFF) return false;
    if (rx_buf_[2] != id) return false;
    uint8_t error = rx_buf_[4];
    if (error != 0) return false;

    // verify checksum
    uint8_t sum_len = 1 + 1 + 1 + len; // ID + LEN + ERR + params
    uint8_t csum_resp = checksum(&rx_buf_[2], sum_len);
    uint8_t received_csum = rx_buf_[2 + sum_len];
    if (csum_resp != received_csum) return false;

    // copy params
    for (uint8_t i = 0; i < len; ++i) out[i] = rx_buf_[5 + i];
    return true;
}

bool ServoBus::startReadRegister(uint8_t id, uint8_t reg, uint8_t len, uint16_t& expected_rx_out)
{
    // Build read packet and transmit only
    uint8_t params_len = 2; // address + length
    size_t idx = 0;
    tx_buf_[idx++] = 0xFF;
    tx_buf_[idx++] = 0xFF;
    tx_buf_[idx++] = id;
    tx_buf_[idx++] = params_len + 1; // LEN field
    tx_buf_[idx++] = static_cast<uint8_t>(Instruction::Read);
    tx_buf_[idx++] = reg;
    tx_buf_[idx++] = len;
    uint8_t csum = checksum(&tx_buf_[2], static_cast<size_t>(idx - 2));
    tx_buf_[idx++] = csum;

    if (!port_.transmitDMA(tx_buf_.data(), (uint16_t)idx)) return false;

    // Expected response size: header(2)+ID(1)+LEN(1)+ERR(1)+params(len)+CHK(1)
    uint16_t expected_rx = static_cast<uint16_t>(6 + len - 0);
    expected_rx_out = expected_rx;
    return true;
}

bool ServoBus::finishReadRegister(uint8_t id, uint8_t len, uint8_t* out, uint16_t expected_rx)
{
    if (expected_rx > rx_buf_.size()) return false;
    if (!port_.receiveDMA(rx_buf_.data(), expected_rx)) return false;

    if (rx_buf_[0] != 0xFF || rx_buf_[1] != 0xFF) return false;
    if (rx_buf_[2] != id) return false;
    uint8_t error = rx_buf_[4];
    if (error != 0) return false;

    uint8_t sum_len = 1 + 1 + 1 + len; // ID + LEN + ERR + params
    uint8_t csum_resp = checksum(&rx_buf_[2], sum_len);
    uint8_t received_csum = rx_buf_[2 + sum_len];
    if (csum_resp != received_csum) return false;

    for (uint8_t i = 0; i < len; ++i) out[i] = rx_buf_[5 + i];
    return true;
}

bool ServoBus::syncWritePositions(const uint8_t* ids, const uint16_t* positions, const uint16_t* times_ms, size_t count)
{
    if (!ids || !positions || count == 0) return false;
    // Instruction 0x83 - Sync Write
    constexpr uint8_t INST_SYNC_WRITE = 0x83;
    uint8_t start_reg = static_cast<uint8_t>(Reg::Position);
    uint8_t data_len = 4; // position(2) + time(2)

    // params: start_address(1) + data_len(1) + [id + data_len bytes] * count
    size_t params_len = 2 + count * (1 + data_len);
    size_t idx = 0;
    tx_buf_[idx++] = 0xFF;
    tx_buf_[idx++] = 0xFF;
    tx_buf_[idx++] = 0xFE; // broadcast
    tx_buf_[idx++] = static_cast<uint8_t>(params_len + 1); // LEN field
    tx_buf_[idx++] = INST_SYNC_WRITE;
    tx_buf_[idx++] = start_reg;
    tx_buf_[idx++] = data_len;

    for (size_t i = 0; i < count; ++i) {
        tx_buf_[idx++] = ids[i];
        uint16_t pos = positions[i];
        tx_buf_[idx++] = static_cast<uint8_t>(pos & 0xFF);
        tx_buf_[idx++] = static_cast<uint8_t>((pos >> 8) & 0xFF);
        uint16_t tm = times_ms ? times_ms[i] : 0;
        tx_buf_[idx++] = static_cast<uint8_t>(tm & 0xFF);
        tx_buf_[idx++] = static_cast<uint8_t>((tm >> 8) & 0xFF);
    }

    uint8_t csum = checksum(&tx_buf_[2], static_cast<size_t>(idx - 2));
    tx_buf_[idx++] = csum;

    // Transmit broadcast packet; no status packet expected
    return port_.transmitDMA(tx_buf_.data(), (uint16_t)idx);
}

bool ServoBus::ping(uint8_t id)
{
    // Build ping packet: header, id, length=2, instruction=Ping, checksum
    size_t idx = 0;
    tx_buf_[idx++] = 0xFF;
    tx_buf_[idx++] = 0xFF;
    tx_buf_[idx++] = id;
    tx_buf_[idx++] = 2; // length: instruction + checksum
    tx_buf_[idx++] = static_cast<uint8_t>(Instruction::Ping);
    uint8_t csum = checksum(&tx_buf_[2], static_cast<size_t>(idx - 2));
    tx_buf_[idx++] = csum;

    if (!port_.transmitDMA(tx_buf_.data(), (uint16_t)idx)) return false;

    // Expect minimal status packet (6 bytes)
    const uint16_t expected_rx = 6;
    if (!port_.receiveDMA(rx_buf_.data(), expected_rx)) return false;
    if (rx_buf_[0] != 0xFF || rx_buf_[1] != 0xFF) return false;
    if (rx_buf_[2] != id) return false;
    uint8_t error = rx_buf_[4];
    // verify checksum
    uint8_t sum_len = 1 + 1 + 1 + 0; // ID + LEN + ERR + params(0)
    uint8_t csum_resp = checksum(&rx_buf_[2], sum_len);
    uint8_t received_csum = rx_buf_[2 + sum_len];
    if (csum_resp != received_csum) return false;
    return (error == 0);
}

// Servo high-level methods
bool Servo::ping()
{
    return bus_.ping(id_);
}

bool Servo::setTorqueEnable(bool enable)
{
    uint8_t val = enable ? 1 : 0;
    return bus_.writeRegister(id_, static_cast<uint8_t>(ServoBus::Reg::TorqueEnable), &val, 1);
}

bool Servo::setPosition(uint16_t position, uint16_t time_ms)
{
    uint8_t payload[4];
    payload[0] = static_cast<uint8_t>(position & 0xFF);
    payload[1] = static_cast<uint8_t>((position >> 8) & 0xFF);
    payload[2] = static_cast<uint8_t>(time_ms & 0xFF);
    payload[3] = static_cast<uint8_t>((time_ms >> 8) & 0xFF);
    return bus_.writeRegister(id_, static_cast<uint8_t>(ServoBus::Reg::Position), payload, 4);
}

std::optional<int16_t> Servo::getPosition()
{
    uint8_t buf[2]{};
    if (!bus_.readRegister(id_, static_cast<uint8_t>(ServoBus::Reg::PosRead), 2, buf)) return std::nullopt;
    int16_t v = static_cast<int16_t>((buf[1] << 8) | buf[0]);
    return v;
}

std::optional<int16_t> Servo::getSpeed()
{
    uint8_t buf[2]{};
    if (!bus_.readRegister(id_, static_cast<uint8_t>(ServoBus::Reg::Speed), 2, buf)) return std::nullopt;
    return static_cast<int16_t>((buf[1] << 8) | buf[0]);
}

std::optional<int16_t> Servo::getLoad()
{
    uint8_t buf[2]{};
    if (!bus_.readRegister(id_, static_cast<uint8_t>(ServoBus::Reg::Load), 2, buf)) return std::nullopt;
    return static_cast<int16_t>((buf[1] << 8) | buf[0]);
}

std::optional<uint8_t> Servo::getVoltage()
{
    uint8_t buf[1]{};
    if (!bus_.readRegister(id_, static_cast<uint8_t>(ServoBus::Reg::Voltage), 1, buf)) return std::nullopt;
    return buf[0];
}

std::optional<uint8_t> Servo::getTemperature()
{
    uint8_t buf[1]{};
    if (!bus_.readRegister(id_, static_cast<uint8_t>(ServoBus::Reg::Temperature), 1, buf)) return std::nullopt;
    return buf[0];
}

std::optional<int16_t> Servo::getCurrent()
{
    uint8_t buf[2]{};
    if (!bus_.readRegister(id_, static_cast<uint8_t>(ServoBus::Reg::Current), 2, buf)) return std::nullopt;
    return static_cast<int16_t>((buf[1] << 8) | buf[0]);
}

// HAL callbacks are implemented in main.cpp and forward to
// Stm32UartDmaPort::onTxComplete/onRxComplete, so no
// duplicate definitions are needed here.
