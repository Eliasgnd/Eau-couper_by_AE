#pragma once
#include <QMetaType>
#include <QString>
#include <cstdint>

// ============================================================
//  StmProtocol.h — Types partagés pour le protocole UART
//  STM32G474MCT6 ↔ Raspberry Pi  (voir interface_STM_RPi.pdf)
// ============================================================

// --- Constantes machine ---
static constexpr int    STEPS_PER_MM   = 64;        // pas moteur par mm
static constexpr int    STM_BUFFER_MAX      = 4096;  // taille buffer STM (segments)
static constexpr int    STM_SAFETY_MARGIN   = 96;   // réserve de sécurité dans le buffer STM
static constexpr int    STM_SEND_AHEAD_MAX  = STM_BUFFER_MAX - STM_SAFETY_MARGIN; // fenêtre max
static constexpr int    UART_BAUDRATE       = 115200;
static constexpr int    ACK_TIMEOUT_MS      = 3000;  // timeout ACK (ms), sur END_SEQ uniquement
static constexpr int    SEG_DONE_INTERVAL   = 10;    // miroir du #define firmware : SEG_DONE émis toutes les N exécutions
static constexpr int    MAX_NAK_RETRY       = 3;     // NAK consécutifs avant erreur
static constexpr int    STM_MAX_IN_FLIGHT   = 15;
static constexpr int    STM_FRAME_LEN       = 20;    // sync + seq + dx/dy/dz + v + flags + CRC16
static constexpr int    STM_HEARTBEAT_MS    = 500;
static constexpr int    STM_LINK_TIMEOUT_MS = 1500;
static constexpr int    STM_PROTOCOL_FAULT_LIMIT = 3;

// --- Flags de segment (byte [9] de la trame binaire) ---
static constexpr uint8_t FLAG_END_SEQ   = 0x01;  // Dernier segment — décélération + DONE
static constexpr uint8_t FLAG_HOME_SEQ  = 0x02;  // Déclenche homing complet (ignore dx/dy/dz)
static constexpr uint8_t FLAG_VALVE_ON  = 0x04;  // Ouvre vanne AVANT ce segment
static constexpr uint8_t FLAG_VALVE_OFF = 0x08;  // Ferme vanne AVANT ce segment

// --- Octet de synchronisation trame binaire ---
static constexpr uint8_t STM_SYNC_BYTE = 0xAA;

// --- État global de la machine (reflète les états du STM32) ---
enum class MachineState {
    DISCONNECTED,     // Port série non ouvert
    READY,            // Prêt à recevoir des commandes
    MOVING,           // Trajectoire en cours d'exécution
    HOMING,           // Séquence de homing en cours
    RECOVERY_WAIT,    // En attente de GO après recovery
    EMERGENCY,        // Arrêt d'urgence actif
    ALARM             // Alarme driver ou fin de course
};

struct StmHealth {
    bool valid = false;
    MachineState state = MachineState::DISCONNECTED;
    bool armed = false;
    bool homed = false;
    QString fault;
};
Q_DECLARE_METATYPE(StmHealth)

inline uint16_t stmCrc16(const uint8_t* data, int len)
{
    uint16_t crc = 0xFFFFu;
    for (int i = 0; i < len; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000u)
                crc = static_cast<uint16_t>((crc << 1) ^ 0x1021u);
            else
                crc = static_cast<uint16_t>(crc << 1);
        }
    }
    return crc;
}

// --- Segment de trajectoire (une trame binaire de 11 octets) ---
struct StmSegment {
    int32_t  dx    = 0;     // CHANGÉ : Déplacement relatif 32-bits
    int32_t  dy    = 0;     // CHANGÉ : Déplacement relatif 32-bits
    int32_t  dz    = 0;     // CHANGÉ : Déplacement relatif 32-bits
    uint16_t v_max = 1000;
    uint8_t  flags = 0;
};

// --- Données de recovery après arrêt d'urgence ---
struct RecoveryData {
    int seg = -1;  // Index du dernier segment complètement ACKé
    int x   = 0;   // Position X en pas au moment de l'AU
    int y   = 0;   // Position Y en pas au moment de l'AU
    int z   = 0;   // Position Z en pas au moment de l'AU
};

// --- Helpers de conversion ---
inline double stepsToMm(int steps)  { return steps / static_cast<double>(STEPS_PER_MM); }
inline int    mmToSteps(double mm)  { return static_cast<int>(mm * STEPS_PER_MM); }

// ARR = 1 000 000 / (vitesse_mm_s * STEPS_PER_MM)
// Petit ARR = rapide, grand ARR = lent
inline uint16_t speedToArr(double vitesse_mm_s) {
    if (vitesse_mm_s <= 0.0) return 65535u;
    double arr = 1000000.0 / (vitesse_mm_s * STEPS_PER_MM);
    if (arr < 1.0)      return 1u;
    if (arr > 65535.0)  return 65535u;
    return static_cast<uint16_t>(arr);
}

inline double arrToSpeed_mm_s(uint16_t arr) {
    if (arr == 0) return 0.0;
    return 1000000.0 / (static_cast<double>(arr) * STEPS_PER_MM);
}
