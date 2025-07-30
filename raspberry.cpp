// raspberry.cpp
#include "raspberry.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>  // ✅ Pour ::close()
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <thread>
#include <chrono>

// Constructeur et destructeur toujours définis
Raspberry::Raspberry(QObject *parent)
    : QObject(parent)
{
#ifndef _WIN32
    // Ouvre le chip GPIO
    chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip) {
        std::cerr << "Erreur: impossible d'ouvrir gpiochip0" << std::endl;
    }
#endif
}

Raspberry::~Raspberry() {
    close();
}

bool Raspberry::init() {
#ifndef _WIN32
    if (!chip)           return false;
    if (!init_gpio())    return false;
    if (!init_spi())     return false;
#endif
    return true;
}

void Raspberry::close() {
#ifndef _WIN32
    // Libération GPIO
    for (auto &p : outputLines) gpiod_line_release(p.second);
    for (auto &p : inputLines)  gpiod_line_release(p.second);
    outputLines.clear();
    inputLines.clear();

    // Fermeture chip
    if (chip) {
        gpiod_chip_close(chip);
        chip = nullptr;
    }

    // Fermeture SPI
    if (spiFd >= 0) {
        ::close(spiFd);  // ✅ Problème résolu ici
        spiFd = -1;
    }

    std::cout << "[Linux] Fermeture GPIO/SPI" << std::endl;
#else
    // Sous Windows, rien à faire
#endif
}

#ifndef _WIN32

bool Raspberry::init_gpio() {
    // Configure GPIO output
    for (auto pin : outputPins) {
        auto *line = gpiod_chip_get_line(chip, pin);
        if (line && gpiod_line_request_output_flags(line, "raspberry", 0, 0) == 0) {
            outputLines[pin] = line;
        } else {
            std::cerr << "Erreur init sortie GPIO " << int(pin) << std::endl;
        }
    }

    // Configure GPIO input avec pull-up
    for (auto pin : inputPins) {
        auto *line = gpiod_chip_get_line(chip, pin);
        if (line && gpiod_line_request_input_flags(line, "raspberry",
                                                   GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) == 0) {
            inputLines[pin] = line;
        } else {
            std::cerr << "Erreur init entrée GPIO " << int(pin) << std::endl;
        }
    }

    // CS idle, puis EN1/EN2/EN3 à HIGH
    writePin(PIN_SCS, true);
    writePin(EN1_PIN, true);
    writePin(EN2_PIN, true);
    writePin(EN3_PIN, true);

    std::cout << "[Linux] GPIOs initialisés (CS idle, EN1/EN2/EN3=HIGH)" << std::endl;
    return true;
}

bool Raspberry::init_spi(const char *device, uint32_t speed) {
    spiSpeed = speed;
    spiFd = ::open(device, O_RDWR);
    if (spiFd < 0) {
        std::cerr << "Erreur ouverture SPI " << device << std::endl;
        return false;
    }
    uint8_t mode = SPI_MODE_0, bits = 16;
    if (ioctl(spiFd, SPI_IOC_WR_MODE, &mode) < 0 ||
        ioctl(spiFd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
        ioctl(spiFd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        std::cerr << "Erreur configuration SPI" << std::endl;
        ::close(spiFd);
        spiFd = -1;
        return false;
    }
    return true;
}

void Raspberry::writePin(uint8_t pin, bool value) {
    auto it = outputLines.find(pin);
    if (it != outputLines.end())
        gpiod_line_set_value(it->second, value ? 1 : 0);
    else
        std::cerr << "GPIO " << int(pin) << " non configuré en sortie" << std::endl;
}

bool Raspberry::readPin(uint8_t pin) {
    auto it = inputLines.find(pin);
    if (it != inputLines.end())
        return gpiod_line_get_value(it->second) > 0;
    std::cerr << "GPIO " << int(pin) << " non configuré en entrée" << std::endl;
    return false;
}

void Raspberry::selectDriver(int n) {
    writePin(EN1_PIN, n == 1);
    writePin(EN2_PIN, n == 2);
    writePin(EN3_PIN, n == 3);
}

void Raspberry::setOutputPins(bool high) {
    for (auto pin : outputPins) writePin(pin, high);
}

Raspberry::Status Raspberry::readStatus() {
    return { readPin(STALLN_PIN), readPin(FAULTN_PIN), readPin(BEMF_PIN) };
}

uint16_t Raspberry::transfer(uint16_t word) {
    if (spiFd < 0) return 0;
    struct spi_ioc_transfer tr{};
    uint16_t tx = word, rx = 0;
    tr.tx_buf = reinterpret_cast<unsigned long>(&tx);
    tr.rx_buf = reinterpret_cast<unsigned long>(&rx);
    tr.len = 2;
    tr.speed_hz = spiSpeed;
    tr.bits_per_word = 16;

    writePin(PIN_SCS, false);
    int ret = ioctl(spiFd, SPI_IOC_MESSAGE(1), &tr);
    writePin(PIN_SCS, true);
    if (ret < 0)
        std::cerr << "Erreur transfert SPI" << std::endl;

    emit spiTransfered(word, rx);
    return rx;
}

#endif // _WIN32
