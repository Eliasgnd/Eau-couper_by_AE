#pragma once
#include <vector>
#include <cstdint>
#include <map>

#ifdef DISABLE_GPIO
// ==== Stub Raspberry (no GPIO) ====
class Raspberry {
public:
    Raspberry() {}
    ~Raspberry() {}

    bool init() { return true; }
    void close() {}
};
#else
// ==== Version Linux avec libgpiod ====
#include <gpiod.h>

class Raspberry {
public:
    Raspberry();
    ~Raspberry();

    bool init();
    void close();

private:
    gpiod_chip *chip {nullptr};
    std::map<uint8_t, gpiod_line*> outputLines;
    std::map<uint8_t, gpiod_line*> inputLines;
};
#endif
