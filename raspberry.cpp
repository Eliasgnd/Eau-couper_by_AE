#include "raspberry.h"

#ifndef _WIN32

#include <gpiod.h>
#include <iostream>
#include <thread>
#include <chrono>

Raspberry::Raspberry() {
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

    // Configure les GPIO en sortie
    for (uint8_t pin : outputPins) {
        gpiod_line* line = gpiod_chip_get_line(chip, pin);
        if (line && gpiod_line_request_output(line, "raspberry", 0) == 0) {
            outputLines[pin] = line;
        } else {
            std::cerr << "Erreur init sortie GPIO " << (int)pin << "\n";
        }
    }

    // Configure les GPIO en entrée
    for (uint8_t pin : inputPins) {
        gpiod_line* line = gpiod_chip_get_line(chip, pin);
        if (line && gpiod_line_request_input(line, "raspberry") == 0) {
            inputLines[pin] = line;
        } else {
            std::cerr << "Erreur init entrée GPIO " << (int)pin << "\n";
        }
    }

    std::cout << "[Linux] GPIOs initialisés via libgpiod.\n";
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

    std::cout << "[Linux] Fermeture des GPIO via libgpiod.\n";
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

void Raspberry::testOutputPins() {
    std::cout << "[Linux] Test GPIO sortie...\n";
    for (int i = 0; i < 10; ++i) {
        for (uint8_t pin : outputPins) {
            writePin(pin, true);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        for (uint8_t pin : outputPins) {
            writePin(pin, false);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "[Linux] Test GPIO sortie terminé.\n";
}

void Raspberry::testInputPins() {
    std::cout << "[Linux] Test GPIO entrée...\n";
    for (int i = 0; i < 10; ++i) {
        for (uint8_t pin : inputPins) {
            bool val = readPin(pin);
            std::cout << "GPIO " << (int)pin << " = " << (val ? "HIGH" : "LOW") << "\n";
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "[Linux] Test GPIO entrée terminé.\n";
}

#endif
