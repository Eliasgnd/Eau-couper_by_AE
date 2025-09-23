#include "raspberry.h"

#ifndef DISABLE_GPIO   // Compile uniquement sur Linux avec GPIO

#include <gpiod.h>
#include <iostream>

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
    std::cout << "[Linux] GPIO initialisés via libgpiod.\n";
    return true;
}

void Raspberry::close() {
    if (chip) {
        gpiod_chip_close(chip);
        chip = nullptr;
    }
    std::cout << "[Linux] Fermeture des GPIO via libgpiod.\n";
}

#endif // DISABLE_GPIO
