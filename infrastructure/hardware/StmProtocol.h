#pragma once
#include <cstdint>

// ============================================================
//  StmProtocol.h — Types partagés pour le protocole UART
//  STM32G474MCT6 ↔ Raspberry Pi  (voir interface_STM_RPi.pdf)
// ============================================================

// --- Constantes machine ---
static constexpr int    STEPS_PER_MM   = 64;        // pas moteur par mm
static constexpr int    STM_BUFFER_MAX = 4096;       // taille buffer STM (segments)
static constexpr int    UART_BAUDRATE  = 115200;
static constexpr int    STM_ACK_BATCH  = 100;        // ACK STM toutes les N trames reçues
static constexpr int    ACK_TIMEOUT_MS = 5000;       // timeout ACK batch (ms) — étendu pour couvrir 100 trames
static constexpr int    MAX_NAK_RETRY  = 3;          // NAK consécutifs avant erreur

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

// --- Segment de trajectoire (une trame binaire de 11 octets) ---
struct StmSegment {
    int16_t  dx    = 0;     // Déplacement relatif en pas, axe X (big-endian)
    int16_t  dy    = 0;     // Déplacement relatif en pas, axe Y (big-endian)
    int16_t  dz    = 0;     // Déplacement relatif en pas, axe Z (big-endian)
    uint16_t v_max = 1000;  // Valeur ARR timer (logique inversée : petit = rapide)
    uint8_t  flags = 0;     // Combinaison de FLAG_*
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
