/**
 * PulseProgress.ino — Render a live pulse countdown as a text progress bar
 * over Serial, driven by getStatus().
 *
 * The carriage-return trick ('\r') rewrites the same Serial line in place,
 * so the bar animates without scrolling. It needs no extra hardware and
 * works in the plain Arduino IDE Serial Monitor and most terminals.
 *
 * getStatus() is used (instead of separate isPulsing()/remaining() calls)
 * so the "still pulsing?" flag and the "time left" value are guaranteed to
 * come from the same instant — see the library README for why that matters.
 */

#include <Arduino.h>
#include <DigitalOutput.h>

DigitalOutput relay(7); // Active LOW by default; drives an LED or relay

const uint32_t PULSE_MS  = 4000; // duration of each pulse we trigger & track
const uint32_t IDLE_MS   = 2000; // pause between pulses, to show the idle gap
const uint8_t  BAR_WIDTH = 20;   // progress bar width, in characters

bool     wasPulsing  = false; // edge detector for the pulsing -> idle change
uint32_t nextTrigger = 0;     // when to start the next pulse

/**
 * @brief Draws one frame of the progress bar on the current Serial line.
 *
 * @param remaining Milliseconds left in the pulse (from getStatus()).
 */
void renderBar(uint32_t remaining) {
    // remaining is already clamped to <= PULSE_MS by the library, but guard
    // anyway so the math stays correct even if PULSE_MS is ever changed.
    uint32_t elapsed = (remaining > PULSE_MS) ? PULSE_MS : (PULSE_MS - remaining);
    uint8_t  filled  = (uint8_t)((uint32_t)BAR_WIDTH * elapsed / PULSE_MS);
    uint8_t  pct     = (uint8_t)((uint32_t)100 * elapsed / PULSE_MS);

    Serial.print('\r'); // jump to start of line without advancing
    Serial.print("Pulse [");
    for (uint8_t i = 0; i < BAR_WIDTH; i++) {
        Serial.print(i < filled ? '#' : '-');
    }
    Serial.print("] ");
    if (pct < 10) Serial.print(' '); // keep the percentage right-aligned
    Serial.print(pct);
    Serial.print("%  ");
    Serial.print(remaining);
    Serial.print(" ms left   "); // trailing pad overwrites any leftover chars
}

void setup() {
    Serial.begin(115200);
    relay.begin();
    Serial.println("DigitalOutput pulse progress demo");
    nextTrigger = millis() + 500; // first pulse shortly after boot
}

void loop() {
    relay.update(); // retire an expired pulse *before* reading its status

    DigitalOutput::Status status = relay.getStatus();

    if (status.pulsing) {
        renderBar(status.remaining);
        wasPulsing = true;
    } else {
        if (wasPulsing) {
            // The pulse just finished: draw a final full bar, then break the
            // line so the completed bar is not overwritten by the next one.
            renderBar(0);
            Serial.println();
            Serial.println("Pulse complete.");
            wasPulsing = false;
            nextTrigger = millis() + IDLE_MS;
        }

        // Start the next pulse once the idle gap has elapsed.
        if ((int32_t)(millis() - nextTrigger) >= 0) {
            relay.pulse(PULSE_MS, DigitalOutput::PulseType::POSITIVE);
        }
    }
}
