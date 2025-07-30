#include "raspberry.h"

#ifndef _WIN32

#include <gpiod.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <iostream>
#include <thread>
#include <chrono>

Raspberry::Raspberry(QObject *parent) : QObject(parent) {
    chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip) {
        std::cerr << "Erreur: impossible d'ouvrir gpiochip0\n";
    }
}

Raspberry::~Raspberry() {
    close();
}

bool Raspberry::init() {
    if (!chip) return false;
    if (!init_gpio()) return false;
    if (!init_spi()) return false;
    return true;
}

bool Raspberry::init_gpio() {
    // Configure GPIO output pins
    for (uint8_t pin : outputPins) {
        gpiod_line* line = gpiod_chip_get_line(chip, pin);
        if (line && gpiod_line_request_output_flags(line, "raspberry", 0, 0) == 0) {
            outputLines[pin] = line;
        } else {
            std::cerr << "Erreur init sortie GPIO " << (int)pin << "\n";
        }
    }

    // Configure GPIO input pins with pull-up
    for (uint8_t pin : inputPins) {
        gpiod_line* line = gpiod_chip_get_line(chip, pin);
        if (line && gpiod_line_request_input_flags(line, "raspberry", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) == 0) {
            inputLines[pin] = line;
        } else {
            std::cerr << "Erreur init entrée GPIO " << (int)pin << "\n";
        }
    }

    // CS high when idle
    writePin(PIN_SCS, true);
    std::cout << "[Linux] GPIOs initialisés via libgpiod.\n";
    return true;
}

bool Raspberry::init_spi(const char *device, uint32_t speed) {
    spiSpeed = speed;
    spiFd = ::open(device, O_RDWR);
    if (spiFd < 0) {
        std::cerr << "Erreur ouverture SPI " << device << "\n";
        return false;
    }
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 16;
    if (ioctl(spiFd, SPI_IOC_WR_MODE, &mode) < 0 ||
        ioctl(spiFd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
        ioctl(spiFd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        std::cerr << "Erreur configuration SPI\n";
        ::close(spiFd);
        spiFd = -1;
        return false;
    }
    return true;
}

void Raspberry::close() {
    // Libère toutes les lignes GPIO
    for (auto& pair : outputLines) {
        gpiod_line_release(pair.second);
    }
    outputLines.clear();

    for (auto& pair : inputLines) {
        gpiod_line_release(pair.second);
    }
    inputLines.clear();

    if (chip) {
        gpiod_chip_close(chip);
        chip = nullptr;
    }
    if (spiFd >= 0) {
        ::close(spiFd);
        spiFd = -1;
    }

    std::cout << "[Linux] Fermeture des GPIO/SPI via libgpiod.\n";
}

void Raspberry::writePin(uint8_t pin, bool value) {
    auto it = outputLines.find(pin);
    if (it != outputLines.end()) {
        gpiod_line_set_value(it->second, value ? 1 : 0);
    } else {
        std::cerr << "GPIO " << (int)pin << " non configuré en sortie\n";
    }
}

bool Raspberry::readPin(uint8_t pin) {
    auto it = inputLines.find(pin);
    if (it != inputLines.end()) {
        return gpiod_line_get_value(it->second) > 0;
    } else {
        std::cerr << "GPIO " << (int)pin << " non configuré en entrée\n";
        return false;
    }
}

void Raspberry::selectDriver(int n) {
    writePin(EN1_PIN, n == 1);
    writePin(EN2_PIN, n == 2);
    writePin(EN3_PIN, n == 3);
}

void Raspberry::setOutputPins(bool high) {
    for (uint8_t p : outputPins) {
        writePin(p, high);
    }
}

Raspberry::Status Raspberry::readStatus() {
    Status s{readPin(STALLN_PIN), readPin(FAULTN_PIN), readPin(BEMF_PIN)};
    return s;
}

uint16_t Raspberry::transfer(uint16_t word) {
    if (spiFd < 0)
        return 0;
    struct spi_ioc_transfer tr{};
    uint16_t tx = word;
    uint16_t rx = 0;
    tr.tx_buf = (unsigned long)&tx;
    tr.rx_buf = (unsigned long)&rx;
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

#endif
