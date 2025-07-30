#include "raspberry.h"
#include <iostream>

#ifdef _WIN32
#  define STAT(x) (0)   // stub pour code Windows
#else
#  include <fcntl.h>
#  include <unistd.h>
#  include <sys/ioctl.h>
#  include <linux/spi/spidev.h>
#  include <thread>
#  include <chrono>
#endif

Raspberry::Raspberry(QObject *parent)
    : QObject(parent)
{
#ifndef _WIN32
    // Ouvre le chip GPIO
    chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip)
        std::cerr << "Erreur: impossible d'ouvrir gpiochip0" << std::endl;
#endif
}

Raspberry::~Raspberry() {
    close();
}

bool Raspberry::init() {
#ifndef _WIN32
    if (!chip)        return false;
    if (!init_gpio()) return false;

    // Séquence de reset matériel du DRV8711
    // Assurer SLEEPn = HIGH
    writePin(PIN_SLEEPn, true);
    // Pulse RESET LOW puis HIGH
    writePin(PIN_RESET, false);
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    writePin(PIN_RESET, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (!init_spi())  return false;
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

    if (chip) {
        gpiod_chip_close(chip);
        chip = nullptr;
    }
    if (spiFd >= 0) {
        ::close(spiFd);
        spiFd = -1;
    }
    std::cout << "[Linux] Fermeture GPIO/SPI" << std::endl;
#else
    // sous Windows, rien à faire
#endif
}

#ifndef _WIN32

bool Raspberry::init_gpio() {
    // Configuration des GPIO en sortie
    for (auto pin : outputPins) {
        auto *line = gpiod_chip_get_line(chip, pin);
        if (line && gpiod_line_request_output_flags(line, "raspberry", 0, 0) == 0) {
            outputLines[pin] = line;
        } else {
            std::cerr << "Erreur init sortie GPIO " << int(pin) << std::endl;
        }
    }

    // Configuration des GPIO en entrée avec pull-up
    for (auto pin : inputPins) {
        auto *line = gpiod_chip_get_line(chip, pin);
        if (line && gpiod_line_request_input_flags(line, "raspberry",
                                                   GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) == 0) {
            inputLines[pin] = line;
        } else {
            std::cerr << "Erreur init GPIO entrée " << int(pin) << std::endl;
        }
    }

    // État initial : CS idle, EN1/EN2/EN3 = HIGH, SLEEPn = HIGH, RESET = HIGH
    writePin(PIN_SCS,    true);
    writePin(EN1_PIN,    true);
    writePin(EN2_PIN,    true);
    writePin(EN3_PIN,    true);
    writePin(PIN_SLEEPn, true);
    writePin(PIN_RESET,  true);

    std::cout << "[Linux] GPIOs initialisés (CS idle, ENs=HIGH, SLEEPn/RESET=HIGH)" << std::endl;
    return true;
}

bool Raspberry::init_spi(const char *device, uint32_t speed) {
    spiSpeed = speed;
    spiFd = ::open(device, O_RDWR);
    if (spiFd < 0) {
        std::cerr << "Erreur ouverture SPI " << device << std::endl;
        return false;
    }
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 16;
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
}

bool Raspberry::readPin(uint8_t pin) {
    auto it = inputLines.find(pin);
    if (it != inputLines.end())
        return gpiod_line_get_value(it->second) > 0;
    return false;
}

uint16_t Raspberry::transfer(uint16_t word) {
    if (spiFd < 0) return 0;
    struct spi_ioc_transfer tr{};
    uint16_t tx = word, rx = 0;
    tr.tx_buf        = reinterpret_cast<unsigned long>(&tx);
    tr.rx_buf        = reinterpret_cast<unsigned long>(&rx);
    tr.len           = 2;
    tr.speed_hz      = spiSpeed;
    tr.bits_per_word = 16;

    writePin(PIN_SCS, false);
    int ret = ioctl(spiFd, SPI_IOC_MESSAGE(1), &tr);
    writePin(PIN_SCS, true);
    if (ret < 0)
        std::cerr << "Erreur transfert SPI" << std::endl;

    emit spiTransfered(word, rx);
    return rx;
}

#else

// Stub Windows : ne fait rien
uint16_t Raspberry::transfer(uint16_t) {
    return 0;
}

#endif
