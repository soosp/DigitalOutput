/**
 * @file LightSwitch.ino
 * @brief Example demonstrating hardware interrupt handling using a deferred flag pattern.
 * 
 * This sketch showcases the recommended best practice for controlling a digital output 
 * via an external hardware interrupt (ISR). To prevent deadlocks with the class mutexes, 
 * the ISR only sets a volatile flag, leaving the actual hardware toggle to the main loop.
 */

#include <Arduino.h>
#include <DigitalOutput.h>

/** @brief GPIO pin connected to the external momentary push button. */
const int buttonPin = 2;

/** @brief GPIO pin connected to the digital output device (e.g., light relay). */
const int outputPin = 7;

/** @brief Create a digital output object using active LOW logic by default. */
DigitalOutput light(outputPin);

/** 
 * @brief Volatile flag updated by the ISR to signal a button press event.
 * @note Marked as volatile to prevent the compiler from optimizing out checks in the loop.
 */
volatile bool buttonPressed = false;

/**
 * @brief Interrupt Service Routine (ISR) triggered by the button press.
 * 
 * Keeps execution time to a strict minimum by only modifying a memory flag.
 * Uses the IRAM_ATTR attribute to ensure the function resides in internal RAM for speed.
 */
void IRAM_ATTR buttonISR() {
    buttonPressed = true; 
}

/**
 * @brief Standard Arduino setup function.
 */
void setup() {
    // Safely initialize the digital output hardware
    light.begin();
    
    // Configure the button pin with internal pull-up resistor
    pinMode(buttonPin, INPUT_PULLUP);
    
    // Attach the interrupt to look for a FALLING edge (button pressed to GND)
    attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, FALLING);
}

/**
 * @brief Standard Arduino application loop.
 */
void loop() {
    // Process the deferred interrupt flag safely outside the ISR context
    if (buttonPressed) {
        buttonPressed = false; // Reset the flag immediately
        light.toggle();        // Execute the thread-safe state toggle
    }
}
