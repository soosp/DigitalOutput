/**
 * @file MixedRelays.ino
 * @brief Example sketch demonstrating polymorphic and thread-safe control of
 *        both local GPIO, MCP23X08 and MCP23X17 expander-based digital outputs
 *        using a unified array.
 * 
 * This example showcases how the base DigitalOutput class acts as a generic
 * interface, allowing the application loop to interact with different hardware
 * types uniformly.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X08.h>
#include <Adafruit_MCP23X17.h>

#include <DigitalOutput.h>
#include <McpDigitalOutput.h>

/** @brief Global hardware instance for the MCP23008 port expander. */
Adafruit_MCP23X08 mcp0;

/** @brief Global hardware instance for the MCP23017 port expander. */
Adafruit_MCP23X17 mcp1;

#if !defined(ARDUINO_ARCH_AVR) && !defined(ARDUINO_ARCH_ESP8266)
/** @brief Shared hardware mutex to serialize all I2C transactions targeting this I2C bus. */
std::timed_mutex bus0_mutex;
#endif

/** @brief DigitalOutput 0: Connected directly to microcontroller GPIO pin 7 (Active LOW). */
DigitalOutput localRelay(7, true);

#if !defined(ARDUINO_ARCH_AVR) && !defined(ARDUINO_ARCH_ESP8266)
/** @brief DigitalOutput 1: Connected to MCP23008 GP0 pin (Active LOW, protected by shared mutex). */
McpDigitalOutput mcpRelay0(mcp0, 0, bus0_mutex, true);
/** @brief DigitalOutput 2: Connected to MCP23017 GPA0 pin (Active LOW, protected by shared mutex). */
McpDigitalOutput mcpRelay1(mcp1, 0, bus0_mutex, true);
/** @brief DigitalOutput 3: Connected to MCP23017 GPA1 pin (Active LOW, protected by shared mutex). */
McpDigitalOutput mcpRelay2(mcp1, 1, bus0_mutex, true);
#else
/** @brief DigitalOutput 1: Connected to MCP23008 GP0 pin (Active LOW, protected by shared mutex). */
McpDigitalOutput mcpRelay0(mcp0, 0, true);
/** @brief DigitalOutput 2: Connected to MCP23017 GPA0 pin (Active LOW, protected by shared mutex). */
McpDigitalOutput mcpRelay1(mcp1, 0, true);
/** @brief DigitalOutput 3: Connected to MCP23017 GPA1 pin (Active LOW, protected by shared mutex). */
McpDigitalOutput mcpRelay2(mcp1, 1, true);
#endif

/** @brief Total number of registered devices in the system. */
const uint8_t RELAY_COUNT = 4;

/** 
 * @brief Polymorphic registry array containing base class pointers to all outputs.
 * 
 * This allows the loop to iterate through all devices and invoke operations 
 * regardless of whether they are local GPIOs or external I2C expander pins.
 */
DigitalOutput* relaySystem[RELAY_COUNT] = {
    &localRelay,
    &mcpRelay0,
    &mcpRelay1,
    &mcpRelay2
};

/**
 * @brief Standard Arduino setup function.
 */
void setup() {
    Serial.begin(115200);
    while(!Serial); // Wait for serial port connection (optional)
    Serial.println("Digital Output System Initialization...");

    // Start the standard I2C bus
    Wire.begin();

    // Initialize the MCP23008 expander at the default I2C address 0x20
    if (!mcp0.begin_I2C(0x20)) {
        Serial.println("Error: MCP23008 not found! System halted.");
        while (1); 
    }
    Serial.println("MCP23017 initialized successfully.");

    // Initialize the MCP23017 expander at I2C address 0x21
    if (!mcp1.begin_I2C(0x21)) {
        Serial.println("Error: MCP23017 not found! System halted.");
        while (1); 
    }
    Serial.println("MCP23017 initialized successfully.");

    // Initialize all outputs dynamically via loop (triggers respective overridden _init methods)
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
        relaySystem[i]->begin(DigitalOutput::State::OFF); // Safe, glitch-free initial state enforcement
    }
    Serial.println("All devices initialized glitch-free in OFF state.");
}

/**
 * @brief Standard Arduino application loop.
 */
void loop() {
    Serial.println("\n--- Turning ALL outputs ON sequentially ---");
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
        relaySystem[i]->on();
        delay(500); // 500 ms staggered delay between activations
    }

    delay(2000); // Maintain all outputs in active state for 2 seconds

    Serial.println("--- Toggling ALL outputs simultaneously ---");
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
        relaySystem[i]->toggle(); // Inverts the current state of each device
    }

    delay(2000);

    Serial.println("--- Turning ALL outputs OFF ---");
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
        relaySystem[i]->off();
    }

    delay(5000); // Wait 5 seconds before restarting the demonstration cycle
}
