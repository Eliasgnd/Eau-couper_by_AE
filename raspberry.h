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
    explicit Raspberry(QObject *parent = nullptr);
    ~Raspberry() override;

    bool init();
    void close();

    bool init_gpio();
    bool init_spi(const char *device = "/dev/spidev0.0", uint32_t speed = 1000000);

    // Toujours déclarées pour toute plateforme
    void writePin(uint8_t pin, bool value);
    bool readPin(uint8_t pin);

    /// Sélectionne lequel des 3 DRV8711 est « connecté » au bus via EN1/2/3
    void selectDriver(int n);

    /// Pour forcer toutes les broches de sortie en HIGH ou LOW
    void setOutputPins(bool high);

    struct Status { bool stall, fault, bemf; };
    Status readStatus();

    /// Full-duplex SPI 16 bits
    uint16_t transfer(uint16_t word);

signals:
    void spiTransfered(quint16 tx, quint16 rx);

private:
    // Broches d'état (entrée)
    static constexpr uint8_t STALLN_PIN = 23;
    static constexpr uint8_t FAULTN_PIN = 24;
    static constexpr uint8_t BEMF_PIN   = 18;

    // Broches de contrôle
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

    // Broches SPI matériel
    static constexpr uint8_t SDATI_PIN  = 10; // MOSI
    static constexpr uint8_t SDATAO_PIN = 9;  // MISO
    static constexpr uint8_t SCLK_PIN   = 11; // SCLK

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
    // libgpiod pour GPIO
    gpiod_chip* chip{nullptr};
    std::map<uint8_t, gpiod_line*> outputLines;
    std::map<uint8_t, gpiod_line*> inputLines;

    // Descripteur SPI
    int       spiFd{-1};
    uint32_t  spiSpeed{1000000};
#endif
};
