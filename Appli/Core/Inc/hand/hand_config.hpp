/**
 * @file hand_config.hpp
 * @brief Zentrales Konfigurations-Header für die Handsteuerung (6 Servos pro Hand)
 */

#ifndef HAND_CONFIG_HPP
#define HAND_CONFIG_HPP

#include <cstdint>
#include <array>
#include <string_view>

namespace HandControl {

/**
 * @brief Definition der Finger-Indizes innerhalb einer Hand
 */
enum class Finger : uint8_t {
    Thumb = 0,
    Index = 1,
    Middle = 2,
    Ring = 3,
    Pinky = 4,
    ThumbRotation = 5,
    Count = 6
};

/**
 * @brief Verfügbare Griff-Typen
 */
enum class GripType : uint8_t {
    Open,
    Spitzgriff,
    Dreipunktgriff,
    Schluesselgriff,
    Zylindergriff,
    Hakengriff,
    SphaerischerGriff,
    Count
};

/**
 * @brief Physische Limits und Parameter eines einzelnen Motors
 */
struct FingerConfig {
    std::string_view name;
    uint16_t minPos;      // Entspricht 0 in Tinkerforge-Einheiten
    uint16_t maxPos;      // Entspricht 9000 in Tinkerforge-Einheiten
    uint16_t maxSpeed;    // Standard: 2000
    uint16_t maxCurrent;  // in mA
};

/**
 * @brief Definition eines Griffs (Soll-Positionen für alle 6 Motoren)
 */
struct GripConfig {
    GripType type;
    std::string_view name;
    std::array<uint16_t, static_cast<size_t>(Finger::Count)> positions;
};

/**
 * @brief Globale Achsen-Konfiguration (identisch für links und rechts)
 */
constexpr std::array<FingerConfig, static_cast<size_t>(Finger::Count)> AxisSettings = {{
    {"Thumb Stretch", 0, 9000, 2000, 1500},
    {"Index Stretch", 0, 9000, 2000, 1500},
    {"Middle Stretch", 0, 9000, 2000, 1500},
    {"Ring Stretch", 0, 9000, 2000, 1500},
    {"Pinky Stretch", 0, 9000, 2000, 1500},
    {"Thumb Opposition", 0, 9000, 2000, 1500}
}};

/**
 * @brief Griff-Datenbank basierend auf Tinkerforge-Einheiten (0-9000)
 */
constexpr std::array<GripConfig, static_cast<size_t>(GripType::Count)> GripDatabase = {{
    {GripType::Open, "OPEN", {0, 0, 0, 0, 0, 0}},
    {GripType::Spitzgriff, "SPITZGRIFF", {9000, 9000, 9000, 9000, 9000, 9000}},
    {GripType::Dreipunktgriff, "DREIPUNKTGRIFF", {7000, 7000, 7000, 0, 0, 4500}},
    {GripType::Schluesselgriff, "SCHLUESSELGRIFF", {6000, 3000, 0, 0, 0, 6000}},
    {GripType::Zylindergriff, "ZYLINDERGRIFF", {8000, 8000, 8000, 8000, 8000, 3000}},
    {GripType::Hakengriff, "HAKENGRIFF", {0, 8000, 8000, 8000, 8000, 0}},
    {GripType::SphaerischerGriff, "SPHAERISCHER_GRIFF", {6000, 6000, 6000, 6000, 6000, 4000}}
}};

/**
 * @brief Hilfsklasse zur ID-Auflösung
 */
class Hand {
public:
    enum class Side { Left, Right };

    /**
     * @brief Berechnet die Servo-ID basierend auf Handseite und Finger
     * Rechts: 1-6, Links: 7-12
     */
    static constexpr uint8_t getServoID(Side side, Finger finger) {
        uint8_t baseID = (side == Side::Right) ? 1 : 7;
        return baseID + static_cast<uint8_t>(finger);
    }

    /**
     * @brief Rechnet Tinkerforge-Einheiten (0-9000) in STS3215 Einheiten (0-4095) um
     */
    static constexpr uint16_t mapToServoPos(uint16_t tinkerPos) {
        // Lineare Skalierung: (tinkerPos / 9000) * 4095
        return static_cast<uint16_t>((static_cast<uint32_t>(tinkerPos) * 4095) / 9000);
    }
};

} // namespace HandControl

#endif // HAND_CONFIG_HPP