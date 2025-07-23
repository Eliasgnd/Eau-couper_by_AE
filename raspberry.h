#pragma once
#include <vector>
#include <cstdint>
#include <map>
#ifndef _WIN32
#include <gpiod.h>
#endif

class Raspberry {
public:
    Raspberry();
    ~Raspberry();

    bool init();
    void close();

private:
    // Broches GPIO BCM
    static constexpr uint8_t PIN_SCS     = 20;  // Chip Select (CS) - utilisé comme GPIO
    static constexpr uint8_t PIN_RESET   = 21;
    static constexpr uint8_t PIN_SLEEPn  = 26;
    static constexpr uint8_t STEP_PIN    = 13;
    static constexpr uint8_t DIR_PIN     = 6;
    static constexpr uint8_t EN1_PIN     = 7;
    static constexpr uint8_t EN2_PIN     = 8;
    static constexpr uint8_t EN3_PIN     = 25;
    static constexpr uint8_t STALLN_PIN  = 23;
    static constexpr uint8_t FAULTN_PIN  = 24;
    static constexpr uint8_t BIN1_PIN    = 17;
    static constexpr uint8_t BIN2_PIN    = 27;
    static constexpr uint8_t BEMF_PIN    = 18;

    // ⚠️ Les broches SPI suivantes sont gérées par le contrôleur SPI matériel
    static constexpr uint8_t SDATI_PIN   = 10;  // SPI MOSI - sortie Pi
    static constexpr uint8_t SDATAO_PIN  = 9;   // SPI MISO - entrée Pi
    static constexpr uint8_t SCLK_PIN    = 11;  // SPI Clock

    // Pins configurées en sortie (GPIO output) — SANS les broches SPI
    std::vector<uint8_t> outputPins = {
        PIN_SCS, PIN_RESET, PIN_SLEEPn,
        STEP_PIN, DIR_PIN,
        EN1_PIN, EN2_PIN, EN3_PIN,
        BIN1_PIN, BIN2_PIN
    };

    // Pins configurées en entrée (GPIO input) — SANS les broches SPI
    std::vector<uint8_t> inputPins = {
        STALLN_PIN, FAULTN_PIN, BEMF_PIN
    };

#ifndef _WIN32
    void setupPinOutput(uint8_t pin);
    void setupPinInput(uint8_t pin);
    void writePin(uint8_t pin, bool value);
    bool readPin(uint8_t pin);
    gpiod_chip *chip {nullptr};
    std::map<uint8_t, gpiod_line*> outputLines;
    std::map<uint8_t, gpiod_line*> inputLines;
#endif
};
