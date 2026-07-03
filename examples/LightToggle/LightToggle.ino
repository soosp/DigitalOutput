/**
 * @file LightToggle.ino
 * @brief Basic example demonstrating how to control a local digital output.
 * 
 * This sketch initializes a single digital output pin (e.g., connected to a light relay)
 * and toggles its state back and forth with a 1-second delay in a safe, glitch-free manner.
 */

#include <Arduino.h>
#include <DigitalOutput.h>

/** 
 * @brief Create a digital output object on pin 7.
 * 
 * Most relay/transistor modules use active LOW logic by default, 
 * so the second argument (active_low) is omitted here as it defaults to true.
 */
DigitalOutput light(7);

/**
 * @brief Standard Arduino setup function.
 * 
 * Initializes the hardware pin safely.
 */
void setup() {
    // Start the digital output device (defaults to State::OFF)
    light.begin(); 
}

/**
 * @brief Standard Arduino application loop.
 * 
 * Performs a simple periodic on/off sequence.
 */
void loop() {
    light.on();    // Switch the output state to logical ON
    delay(1000);   // Maintain state for 1000 milliseconds
    
    light.off();   // Switch the output state to logical OFF
    delay(1000);   // Maintain state for 1000 milliseconds
}
