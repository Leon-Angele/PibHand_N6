#include "hand/hand_controller.hpp"
#include "hand/hand_config.hpp"
#include <algorithm>
#include <cstring>

using namespace HandControl;

HandController::HandController(Hand::Side side, ServoBus& bus) noexcept
    : side_(side), bus_(bus)
{
    // Initialize current positions from GripDatabase Open (safe default)
    for (size_t i = 0; i < AXIS_COUNT; ++i) {
        current_pos_[i] = Hand::mapToServoPos(static_cast<uint16_t>(0));
        start_pos_[i] = current_pos_[i];
        target_pos_[i] = current_pos_[i];
        duration_ms_[i] = 0;
        start_time_ms_[i] = 0;
        moving_[i] = false;
    }
}

void HandController::setTargetGrip(GripType grip, uint16_t duration_ms) noexcept
{
    const auto& cfg = GripDatabase[static_cast<size_t>(grip)];
    uint32_t now = HAL_GetTick();
    for (size_t i = 0; i < AXIS_COUNT; ++i) {
        uint16_t tgt_tinker = cfg.positions[i];
        uint16_t tgt = Hand::mapToServoPos(tgt_tinker);
        start_pos_[i] = current_pos_[i];
        target_pos_[i] = tgt;
        start_time_ms_[i] = now;
        duration_ms_[i] = duration_ms;
        moving_[i] = (start_pos_[i] != target_pos_[i]);
    }
}

float HandController::smoothstep(float t) noexcept
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

float HandController::predictGraspAdjustment(uint8_t /*finger_idx*/, int16_t /*current_load*/, int16_t /*current_speed*/, float /*position_error*/) noexcept
{
    // Placeholder: no adjustment by default. Return 0.0f meaning no correction.
    // Replace with actual AI model integration (X-CUBE-AI) later.
    return 0.0f;
}

void HandController::sendSyncWrite(uint16_t time_ms) noexcept
{
    uint8_t ids[AXIS_COUNT];
    uint16_t poses[AXIS_COUNT];
    uint16_t times[AXIS_COUNT];
    for (size_t i = 0; i < AXIS_COUNT; ++i) {
        ids[i] = Hand::getServoID(side_, static_cast<Finger>(i));
        poses[i] = current_pos_[i];
        times[i] = time_ms;
    }
    // best-effort; ignore return value (could log)
    (void) bus_.syncWritePositions(ids, poses, times, AXIS_COUNT);
}

void HandController::update() noexcept
{
    const uint32_t now = HAL_GetTick();

    // 1) Interpolate trajectories (every call, expected 100Hz)
    for (size_t i = 0; i < AXIS_COUNT; ++i) {
        if (!moving_[i]) continue;
        uint32_t start = start_time_ms_[i];
        uint32_t dur = duration_ms_[i];
        uint32_t elapsed = (now >= start) ? (now - start) : 0;
        float t = (dur == 0) ? 1.0f : (static_cast<float>(elapsed) / static_cast<float>(dur));
        if (t >= 1.0f) {
            current_pos_[i] = target_pos_[i];
            moving_[i] = false;
        } else {
            float s = smoothstep(t);
            int32_t startp = static_cast<int32_t>(start_pos_[i]);
            int32_t endp = static_cast<int32_t>(target_pos_[i]);
            int32_t val = static_cast<int32_t>(startp + static_cast<int32_t>((endp - startp) * s));
            if (val < 0) val = 0;
            if (val > 0xFFFF) val = 0xFFFF;
            current_pos_[i] = static_cast<uint16_t>(val);
        }
    }

    // 2) Send synchronous position update for all axes (non-blocking transmit)
    // Use a conservative time slice: remaining max of durations or 50ms
    uint16_t send_time_ms = 50;
    sendSyncWrite(send_time_ms);

    // 3) Telemetry state-machine: every ~50ms, request then read next axis
    const uint32_t telemetry_period = 50;
    if (!pending_read_) {
        if ((now - last_telemetry_ms_) >= telemetry_period) {
            // start read for telemetry_idx_
            uint8_t id = Hand::getServoID(side_, static_cast<Finger>(telemetry_idx_));
            uint16_t expected = 0;
            // request reading Load (2 bytes) and Speed (2 bytes) together by reading PosRead(0x38) maybe
            // We'll read Load (2) and Speed (2) sequentially across ticks to keep it light.
            // Start with Load register
            bool ok = bus_.startReadRegister(id, static_cast<uint8_t>(ServoBus::Reg::Load), 2, expected);
            if (ok) {
                pending_expected_rx_ = expected;
                pending_read_ = true;
                pending_id_ = id;
            } else {
                // if start failed, skip and advance
                telemetry_idx_ = (telemetry_idx_ + 1) % AXIS_COUNT;
                last_telemetry_ms_ = now;
            }
        }
    } else {
        // finish read for pending_id_
        uint8_t buf[2] = {0};
        if (bus_.finishReadRegister(pending_id_, 2, buf, pending_expected_rx_)) {
            int16_t load = static_cast<int16_t>((buf[1] << 8) | buf[0]);
            // For demonstration: we compute a simple position error and feed into predictor
            // position_error: target - current
            uint8_t finger_idx = 0xFF;
            // reverse lookup finger index from id
            for (size_t i = 0; i < AXIS_COUNT; ++i) {
                if (Hand::getServoID(side_, static_cast<Finger>(i)) == pending_id_) { finger_idx = static_cast<uint8_t>(i); break; }
            }
            if (finger_idx != 0xFF) {
                int16_t speed = 0;
                // quick attempt to read speed (blocking) - omitted for strict non-blocking; placeholder 0
                float pos_err = static_cast<float>(static_cast<int32_t>(target_pos_[finger_idx]) - static_cast<int32_t>(current_pos_[finger_idx]));
                float adj = predictGraspAdjustment(finger_idx, load, speed, pos_err);
                (void)adj; // placeholder usage
            }
        }
        // clear pending and advance
        pending_read_ = false;
        pending_expected_rx_ = 0;
        pending_id_ = 0;
        telemetry_idx_ = (telemetry_idx_ + 1) % AXIS_COUNT;
        last_telemetry_ms_ = now;
    }
}
