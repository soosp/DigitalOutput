/**
 * @file Pulses.ino
 * @brief Example sketch demonstrating the POSITIVE, NEGATIVE, and TOGGLE pulse features.
 * 
 * This sketch demonstrates the non-blocking timed pulse features of the DigitalOutput library.
 * It showcases how devices can be triggered for a specific duration and automatically
 * revert to their baseline states without using blocking delay() calls in the main process.
 */

#include <Arduino.h>
#include <DigitalOutput.h>

/** 
 * @brief DigitalOutput connected to pin 7 (e.g., a door latch relay or status LED).
 * Defaults to active LOW logic.
 */
DigitalOutput device(7);

/** @brief Tracks the current phase of the pulse demonstration. */
uint8_t demoPhase = 0;

/** @brief Timestamp to handle the macro-level transition between demo phases. */
uint32_t lastPhaseChange = 0;

/**
 * @brief Standard Arduino setup function.
 */
void setup() {
    Serial.begin(115200);
    while(!Serial); // Wait for serial connection
    Serial.println("DigitalOutput Pulse Demonstration Started.");

    // Initialize the device safely in the OFF state
    device.begin(DigitalOutput::State::OFF);
    
    lastPhaseChange = millis();
}

/**
 * @brief Standard Arduino application loop.
 */
void loop() {
    // CRITICAL: The update() method must be called constantly to refresh the internal timers!
    device.update();

    uint32_t currentTime = millis();

    // State machine to trigger different pulse types every 6 seconds
    if (currentTime - lastPhaseChange >= 6000) {
        lastPhaseChange = currentTime;
        demoPhase++;

        switch (demoPhase) {
            case 1:
                Serial.println("\n[Phase 1] Triggering a POSITIVE pulse (ON for 2.5 seconds)...");
                // Turns ON from an OFF baseline, then returns to OFF automatically
                device.pulse(2500, DigitalOutput::PulseType::POSITIVE);
                break;

            case 2:
                Serial.println("\n[Phase 2] Setting device permanently ON first...");
                device.on(); // Establish a steady ON baseline
                
                // We let it sit ON for a moment inside the state machine before the next phase
                break;

            case 3:
                Serial.println("\n[Phase 3] Triggering a NEGATIVE pulse (OFF for 3 seconds)...");
                // Drops to OFF from an ON baseline, then returns to ON automatically (e.g., modem reset)
                device.pulse(3000, DigitalOutput::PulseType::NEGATIVE);
                break;

            case 4:
                Serial.println("\n[Phase 4] Turning device permanently OFF again...");
                device.off(); // Restore baseline to OFF
                break;

            case 5:
                Serial.println("\n[Phase 5] Triggering a TOGGLE pulse from OFF state (ON for 2 seconds)...");
                // Inverts from OFF to ON, then reverts to OFF
                device.pulse(2000, DigitalOutput::PulseType::TOGGLE);
                break;

            case 6:
                Serial.println("\n[Phase 6] Setting device permanently ON again...");
                device.on(); // Invert baseline to ON
                break;

            case 7:
                Serial.println("\n[Phase 7] Triggering a TOGGLE pulse from ON state (OFF for 2 seconds)...");
                // Inverts from ON to OFF, then reverts to ON
                device.pulse(2000, DigitalOutput::PulseType::TOGGLE);
                break;

            default:
                Serial.println("\n--- Demonstration Cycle Finished. Resetting... ---");
                device.off();
                demoPhase = 0; // Restart the entire demo cycle
                break;
        }
    }
}
