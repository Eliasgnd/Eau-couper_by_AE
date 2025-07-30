#pragma once

#include <vector>
#include <cstdint>
#include <map>
#include <QObject>

#ifndef _WIN32
#  include <gpiod.h>
#endif

class Raspberry : public QObject {
    Q_OBJECT

public:
    // Prototypes uniquement, sans corps
    explicit Raspberry(QObject *parent = nullptr);
    ~Raspberry() override;

    // Méthodes d'initialisation et de gestion
    bool init();
    void close();

    bool init_gpio();
    bool init_spi(const char *device = "/dev/spidev0.0", uint32_t speed = 1000000);
    void selectDriver(int n);
    void setOutputPins(bool high);

    struct Status { bool stall; bool fault; bool bemf; };
    Status readStatus();

    // ✅ Pins rendues publiques
    static constexpr uint8_t STALLN_PIN = 23;
    static constexpr uint8_t FAULTN_PIN = 24;
    static constexpr uint8_t BEMF_PIN   = 18;

#ifndef _WIN32
    uint16_t transfer(uint16_t word);
#endif

signals:
    void spiTransfered(quint16 tx, quint16 rx);

private:
    static constexpr uint8_t PIN_SCS    = 20;
    static constexpr uint8_t PIN_RESET  = 21;
    static constexpr uint8_t PIN_SLEEPn = 26;
    static constexpr uint8_t STEP_PIN   = 13;
    static constexpr uint8_t DIR_PIN    = 6;
    static constexpr uint8_t EN1_PIN    = 7;
    static constexpr uint8_t EN2_PIN    = 8;
    static constexpr uint8_t EN3_PIN    = 25;
    static constexpr uint8_t BIN1_PIN   = 17;
    static constexpr uint8_t BIN2_PIN   = 27;

    static constexpr uint8_t SDATI_PIN  = 10;
    static constexpr uint8_t SDATAO_PIN = 9;
    static constexpr uint8_t SCLK_PIN   = 11;

    std::vector<uint8_t> outputPins = {
        PIN_SCS, PIN_RESET, PIN_SLEEPn,
        STEP_PIN, DIR_PIN,
        EN1_PIN, EN2_PIN, EN3_PIN,
        BIN1_PIN, BIN2_PIN
    };
    std::vector<uint8_t> inputPins = {
        STALLN_PIN, FAULTN_PIN, BEMF_PIN
    };

#ifndef _WIN32
    void writePin(uint8_t pin, bool value);
    bool readPin(uint8_t pin);

    gpiod_chip *chip{nullptr};
    std::map<uint8_t, gpiod_line*> outputLines;
    std::map<uint8_t, gpiod_line*> inputLines;

    int spiFd{-1};
    uint32_t spiSpeed{1000000};
#endif
};
