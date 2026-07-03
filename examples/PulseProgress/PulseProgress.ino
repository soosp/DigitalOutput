/**
 * PulseProgress.ino — Render a live pulse countdown as a text progress bar
 * over Serial, driven by getStatus().
 *
 * The bar starts full and drains as the remaining time counts down from
 * 100% to 0%. It redraws only when the bar actually changes, so it never
 * floods the serial port regardless of how fast loop() runs.
 *
 * getStatus() is used (instead of separate isPulsing()/remaining() calls)
 * so the "still pulsing?" flag and the "time left" value are guaranteed to
 * come from the same instant — see the library README for why that matters.
 */

#include <Arduino.h>
#include <DigitalOutput.h>

// ---- Output style ---------------------------------------------------------
// true  : carriage return ('\r') — animates in place on a single line.
//         Requires a real terminal (PlatformIO monitor, VS Code serial
//         terminal, PuTTY, screen). The Arduino IDE Serial Monitor does
//         NOT support this: it treats '\r' as a newline and the bar will
//         scroll instead of updating in place.
// false : newline ('\n') — prints each bar step on its own line. Readable
//         everywhere, including the Arduino IDE Serial Monitor.
const bool USE_CARRIAGE_RETURN = true;
// ---------------------------------------------------------------------------

DigitalOutput relay(7); // Active LOW by default; drives an LED or relay

const uint32_t PULSE_MS  = 4000; // duration of each pulse we trigger & track
const uint32_t IDLE_MS   = 2000; // pause between pulses, to show the idle gap
const uint8_t  BAR_WIDTH = 20;   // progress bar width, in characters

bool    wasPulsing = false; // edge detector for the pulsing -> idle change
int16_t lastFilled = -1;    // last drawn fill level; -1 means "nothing yet"
uint32_t nextTrigger = 0;   // when to start the next pulse

/**
 * @brief Draws one frame of the countdown bar.
 *
 * The bar and percentage both reflect the time *remaining*: full at the
 * start of the pulse, empty when it expires.
 *
 * @param remaining Milliseconds left in the pulse (from getStatus()).
 */
void renderBar(uint32_t remaining) {
    // remaining is already clamped to <= PULSE_MS by the library, but guard
    // anyway so the math stays correct even if PULSE_MS is ever changed.
    uint32_t rem    = (remaining > PULSE_MS) ? PULSE_MS : remaining;
    uint8_t  filled = (uint8_t)((uint32_t)BAR_WIDTH * rem / PULSE_MS);
    uint8_t  pct    = (uint8_t)((uint32_t)100 * rem / PULSE_MS);

    Serial.print(USE_CARRIAGE_RETURN ? '\r' : '\n');
    Serial.print("Pulse [");
    for (uint8_t i = 0; i < BAR_WIDTH; i++) {
        Serial.print(i < filled ? '#' : '-');
    }
    Serial.print("] ");
    if (pct < 100) Serial.print(' '); // right-align the percentage to 3 cols
    if (pct < 10)  Serial.print(' ');
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
        // Only redraw when the bar visibly changes. This keeps '\r' mode
        // from spamming the port, and keeps '\n' mode to ~BAR_WIDTH lines
        // per pulse instead of thousands.
        uint32_t rem = (status.remaining > PULSE_MS) ? PULSE_MS : status.remaining;
        int16_t filled = (int16_t)((uint32_t)BAR_WIDTH * rem / PULSE_MS);
        if (filled != lastFilled) {
            lastFilled = filled;
            renderBar(status.remaining);
        }
        wasPulsing = true;
    } else {
        if (wasPulsing) {
            // The pulse just finished: draw a final empty bar (0%), then
            // break the line so it is not overwritten by the next pulse.
            renderBar(0);
            Serial.println();
            Serial.println("Pulse complete.");
            wasPulsing = false;
            lastFilled = -1;
            nextTrigger = millis() + IDLE_MS;
        }

        // Start the next pulse once the idle gap has elapsed.
        if ((int32_t)(millis() - nextTrigger) >= 0) {
            relay.pulse(PULSE_MS, DigitalOutput::PulseType::POSITIVE);
        }
    }
}
