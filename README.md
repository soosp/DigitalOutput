# DigitalOutput

A lightweight, multi-platform, thread-safe, and glitch-free digital output
(relay, LED, valve, buzzer) control library for Arduino.
It features an extensible object-oriented architecture allowing you to
seamlessly mixed local microcontroller GPIO pins and external I2C/SPI port
expanders (like the MCP23X17) under a single, unified polymorphic interface.

## Features

* Thread-Safe Architecture: Uses a platform-aware `std::timed_mutex`
implementation on multi-core/RTOS platforms (e.g., ESP32, ARM) while remaining
lightweight and fully compatible with legacy single-core, single-thread
architectures (AVR,ESP8266).
* Glitch-Free Startup: Enforces output voltage level initialization before
switching pin modes to output, eliminating unwanted startup spikes or
accidental relay triggers.
* Extensible Subclassing: Protected core hardware interfaces (`_init()` and
`_operate()`) enable swift integration with alternative hardware interfaces
(e.g., I2C expanders, SPI shift registers, or MQTT virtual pins).
* Universal Timed Pulses: Supports non-blocking POSITIVE (ON-OFF), NEGATIVE
(OFF-ON), and TOGGLE timed operations using a precise `millis()` state machine.
* Manual Override Safety: Any explicit hardware call (`on()`, `off()`,
`toggle()`) able to automatically terminate active background pulse timers
safely.

## Installation

### Arduino IDE

**Install via Library Manager:**

Search for **DigitalOutput** in the Library Manager
(Sketch → Include Library → Manage Libraries).

**Install manually:**

1. Download this repository as a ZIP file
2. In Arduino IDE: **Sketch → Include Library → Add .ZIP Library**

### PlatformIO

**Install via `platformio.ini`:**

Add the library to your `platformio.ini`:

```ini
lib_deps =
    adafruit/Adafruit MCP23017 Arduino Library
    soosp/DigitalOutput
```

**Install manually:**

1. Download this repository as a ZIP file
2. Extract it into your project's `lib/` directory.

### Manual

Place the header files (`*.h`) from the `src/` directory of this library
inside your sketch folder.

## API Reference

### Constructor (`DigitalOutput`)

```cpp
DigitalOutput(int pin, bool active_low = true)
```

Creates a local-GPIO output bound to `pin`. The constructor only stores
configuration; no pin mode or hardware level is applied until `begin()` is
called. `active_low` selects the electrical polarity of the "ON" logical
state:

* `active_low = true` (default) — `State::ON` drives the pin `LOW`. This
  matches most common relay and opto-isolator modules.
* `active_low = false` — `State::ON` drives the pin `HIGH`, e.g. for a
  directly-driven LED or an active-high solid-state relay.

### Core Methods (`DigitalOutput`)

* **`bool begin(State state = State::OFF)`**
  Initializes the hardware safely: the output level is written *before* the
  pin mode is switched to `OUTPUT`, so no relay chatter or LED flash occurs
  on boot. Also clears any leftover pulse state. Returns `false` only if the
  mutex could not be acquired within `DIGITAL_OUTPUT_MUTEX_TIMEOUT`.

* **`bool on(bool force = true)`** / **`bool off(bool force = true)`** /
  **`bool toggle(bool force = true)`**
  Immediately drive the output to the requested state, bypassing any timed
  pulse in progress. `force` controls what happens if a `pulse()` is
  currently active:
  * `force = true` (default) — the active pulse is cancelled and the new
    state is applied immediately.
  * `force = false` — the call is rejected (no hardware change) while a
    pulse is running; it only takes effect once the pulse has finished.
  All three return `true` on success and `false` if rejected by `force`
  *or* if the mutex timed out — the two cases are not distinguishable from
  the return value alone.

* **`bool pulse(uint32_t interval, PulseType type = PulseType::POSITIVE,`**
  **`bool force = true)`**
  Starts a non-blocking timed pulse of `interval` milliseconds. `force`
  works the same way as above, but applies to an already-running pulse
  instead of a steady state: with `force = false`, calling `pulse()` again
  while one is still active is rejected and the original pulse keeps
  running untouched. This "poll until accepted" pattern (see the `Morse`
  example) is a convenient way to serialize consecutive pulses without
  manual timing code. When a `TOGGLE` pulse is triggered while another
  pulse is already in progress, it inverts from the *original* baseline
  state (the one saved before the first pulse started), not from the
  transient in-pulse state — so back-to-back pulses never drift away from
  the intended resting state.

* **`void update()`**
  Must be called periodically (typically once per `loop()` iteration) for
  `pulse()` to take effect — it is the only place where an expired pulse is
  detected and reverted to its baseline state. Cheap to call when no pulse
  is active (a lock-free flag check short-circuits immediately). See
  "When to use update()" below for when it can be omitted entirely.

* **`State getState()`**
  Returns the *current* logical state, which is `State::ON` or
  `State::OFF`. Note that while a pulse is active, this reflects the
  transient pulsed state, not the baseline it will return to — e.g. during
  a `NEGATIVE` pulse, `getState()` returns `State::OFF` even though the
  output will revert to `State::ON` once the pulse expires. If you need
  the resting baseline instead, use `getBaseline()`. If the mutex times
  out, the last cached value is returned without waiting, which may be
  momentarily stale under heavy contention.

* **`State getBaseline()`**
  Returns the resting state the output will settle into once any active
  pulse expires, unaffected by a pulse currently in progress. When no
  pulse is active, `getBaseline()` always equals `getState()`. Useful for
  UI/status reporting (e.g. MQTT or Home Assistant state topics) and
  persisting state across reboots, where a brief pulse shouldn't be
  reported or saved as a real state change. Subject to the same mutex
  timeout caveat as `getState()`.

### McpDigitalOutput

`McpDigitalOutput` inherits every method above unchanged — `on()`, `off()`,
`toggle()`, `pulse()`, `update()`, and `getState()` all behave identically
regardless of whether the output is a local pin or an expander pin. Only
construction and the internal `_init()`/`_operate()` hardware calls differ.

```cpp
// Multi-core/RTOS platforms (ESP32, ARM, ...)
McpDigitalOutput(Adafruit_MCP23X17 &mcp, int pin,
                  std::timed_mutex &shared_mutex, bool active_low = true)

// Single-core legacy platforms (AVR, ESP8266)
McpDigitalOutput(Adafruit_MCP23X17 &mcp, int pin, bool active_low = true)
```

`pin` is the expander pin number (0–15), not a microcontroller GPIO. On
RTOS platforms, `shared_mutex` must be a `std::timed_mutex` shared by
*every* `McpDigitalOutput` on the same **physical bus** — not merely the
same chip. The actual non-thread-safe resource is the underlying
`Wire`/`TwoWire` (or `SPI`) instance itself: its transaction sequence
(`beginTransmission()` / `write()` / `endTransmission()`) is not atomic
across concurrent callers, so two chips sharing one I2C bus at different
addresses can still corrupt each other's transactions if they use separate
mutexes. Only chips on genuinely independent hardware buses (e.g. an
ESP32's `Wire` and `Wire1`, backed by separate I2C peripherals) may use
separate mutex instances, since those buses can run fully in parallel.

```cpp
std::timed_mutex bus0_mutex; // shared by ALL chips on Wire (bus 0)
std::timed_mutex bus1_mutex; // shared by ALL chips on Wire1 (bus 1), if used

Adafruit_MCP23X17 mcpA, mcpB; // both on Wire, address 0x20 and 0x21
McpDigitalOutput out1(mcpA, 0, bus0_mutex, true);
McpDigitalOutput out2(mcpB, 0, bus0_mutex, true); // same mutex as out1!
```

## Configuration Enums

```cpp
enum class State : uint8_t { ON, OFF };
enum class PulseType : uint8_t {
    POSITIVE, // Forces ON, reverts to OFF after expiration
    NEGATIVE, // Forces OFF, reverts to ON after expiration
    TOGGLE    // Inverts state, reverts to original baseline after expiration
};
```

## ⚠️ Important Configuration Guidelines

### 1. When to use update()

The `update()` method is ONLY required if your code uses the timed `pulse()`
function. If your application only relies on immediate `on()`, `off()`, and
`toggle()` methods, you can completely omit calling `update()` in your main
loop to save valuable CPU cycles.

### 2. Multi-Threading & Mutex Timeout

On RTOS-supported platforms, a fallback macro handles mutex lock recovery to
prevent thread starvation if the hardware bus hangs:

```cpp
#define DIGITAL_OUTPUT_MUTEX_TIMEOUT 1000 // Timeout in milliseconds
```

### 3. Interrupt Service Routines (ISR)

NEVER call state-changing methods (`on()`, `off()`, `toggle()` and `pulse()`)
directly inside an Interrupt Service Routine (ISR). Doing so will bypass thread
safety and will cause a deadlock/system crash on RTOS. Always use a deferred
flag pattern (see example below).

### 4. Custom Subclasses (`_init()` / `_operate()` Overrides)

The internal `std::timed_mutex` is **not** recursive. `_init()` and
`_operate()` are always invoked while this mutex is already held by the
calling public method (`begin()`, `on()`, `off()`, `toggle()`, or
`pulse()`). If a custom override of `_init()` or `_operate()` calls back
into any state-changing method on the *same* object, the lock attempt will
simply time out after `DIGITAL_OUTPUT_MUTEX_TIMEOUT` and fail silently
(returning `false`) rather than deadlock — but this is almost never the
intended behavior. Keep custom `_init()`/`_operate()` overrides limited to
direct hardware I/O, as `McpDigitalOutput` does.

## Usage Examples

### 1. Basic Toggle (`LightToggle.ino`)

A straightforward implementation for single, localized hardware control.

```cpp
#include <Arduino.h>
#include <DigitalOutput.h>

DigitalOutput light(7); // Pin 7, Active LOW by default

void setup() {
    light.begin(); // Initializes as OFF
}

void loop() {
    light.on();
    delay(1000);
    light.off();
    delay(1000);
}
```

### 2. Safe Hardware Interrupt (`LightSwitch.ino`)

The recommended way to process asynchronous input triggers (e.g., buttons,
sensors) safely alongside the library.

```cpp
#include <Arduino.h>
#include <DigitalOutput.h>

const int buttonPin = 2;
DigitalOutput light(7);

volatile bool buttonPressed = false;

void IRAM_ATTR buttonISR() {
    buttonPressed = true; // Minimum footprint ISR
}

void setup() {
    light.begin();
    pinMode(buttonPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, FALLING);
}

void loop() {
    if (buttonPressed) {
        buttonPressed = false; // Clear flag outside ISR context
        light.toggle();        // Safe execution
    }
}
```

### 3. Mixed Hardware via Polymorphism (`MixedRelays.ino`)

Iterating through a synchronized pool containing both native microcontroller
pins and an I2C MCP23017 expander chip.

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <DigitalOutput.h>
#include <McpDigitalOutput.h>

Adafruit_MCP23X17 mcp;

#if !defined(ARDUINO_ARCH_AVR) && !defined(ARDUINO_ARCH_ESP8266)
std::timed_mutex bus0_mutex; // Shared per-bus mutex (Wire), not per-chip
#endif

DigitalOutput localRelay(7, true);

#if !defined(ARDUINO_ARCH_AVR) && !defined(ARDUINO_ARCH_ESP8266)
McpDigitalOutput mcpRelay1(mcp, 0, bus0_mutex, true); // Expander pin 0
McpDigitalOutput mcpRelay2(mcp, 1, bus0_mutex, true); // Expander pin 1
#else
McpDigitalOutput mcpRelay1(mcp, 0, true);
McpDigitalOutput mcpRelay2(mcp, 1, true);
#endif

const uint8_t DEVICE_COUNT = 3;

DigitalOutput* outputSystem[DEVICE_COUNT] = {
    &localRelay,
    &mcpRelay1,
    &mcpRelay2
};

void setup() {
    Wire.begin();
    mcp.begin_I2C(0x20);

    for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
        outputSystem[i]->begin(DigitalOutput::State::OFF);
    }
}

void loop() {
    // Efficiently update background pulse timers for all registered devices
    for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
        outputSystem[i]->update();
    }
    
    // Triggering a non-blocking pulse on an I2C device
    static uint32_t lastTrigger = 0;
    if (millis() - lastTrigger > 10000) {
        lastTrigger = millis();
        mcpRelay1.pulse(3000, DigitalOutput::PulseType::POSITIVE); // ON for 3s
    }
}
```

## License

MIT
