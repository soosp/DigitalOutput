#pragma once

#include <Arduino.h>
#if !defined(ARDUINO_ARCH_AVR) && !defined(ARDUINO_ARCH_ESP8266)
#include <mutex>
#endif

#ifndef DIGITAL_OUTPUT_MUTEX_TIMEOUT
#define DIGITAL_OUTPUT_MUTEX_TIMEOUT 1000
#endif

/**
 * @class DigitalOutput
 * @brief Base class for thread-safe digital output control, extensible for different hardware interfaces.
 */
class DigitalOutput {
public:
    enum class State : uint8_t { ON, OFF };

    /**
     * @enum PulseType
     * @brief Defines the behavior of a timed pulse operation.
     */
    enum class PulseType : uint8_t {
        POSITIVE, /**< Forces ON state, then automatically reverts to OFF */
        NEGATIVE, /**< Forces OFF state, then automatically reverts to ON */
        TOGGLE    /**< Inverts current state, then automatically reverts to the pre-pulse baseline state */
    };

    DigitalOutput(int pin, bool active_low = true) 
        : _pin(pin), _state(State::OFF), _savedState(State::OFF), _isPulsing(false), _pulseStartTime(0), _pulseInterval(0) {
        if (active_low) {
            _activeState = LOW;
            _inactiveState = HIGH;
        } else {
            _activeState = HIGH;
            _inactiveState = LOW;
        }
    }

    /**
     * @brief Virtual destructor ensuring proper cleanup in derived subclasses.
     */
    virtual ~DigitalOutput() = default;

    /**
     * @brief Thread-safe initialization. Calls the protected _init() method.
     */
    bool begin(State state = State::OFF) {
        if (!_lock()) return false;
        _isPulsing = false; // Reset pulse tracker on initialization
        _state = state;
        _init(); // Hardware-specific setup
        _unlock();
        return true;
    }

    /**
     * @brief Turns the output permanently ON.
     * @param force Interrupts active pulses if true.
     * @return true On success.
     * @return false If a non-overridable pulse is in progress or on mutex timeout.
     */
    bool on(bool force = true) {
        if (!_lock()) return false;
        bool retval = (!_isPulsing || force);
        if (retval) {
            _isPulsing = false; // Manual override terminates any active timed pulse
            _state = State::ON;
            _operate();
        }
        _unlock();
        return retval;
    }

    /**
     * @brief Turns the output permanently OFF.
     * @param force Interrupts active pulses if true.
     * @return true On success.
     * @return false If a non-overridable pulse is in progress or on mutex timeout.
     */
    bool off(bool force = true) {
        if (!_lock()) return false;
        bool retval = (!_isPulsing || force);
        if (retval) {
            _isPulsing = false; // Manual override terminates any active timed pulse
            _state = State::OFF;
            _operate();
        }
        _unlock();
        return retval;
    }

    /**
     * @brief Toggles the output.
     * @param force Interrupts active pulses if true.
     * @return true On success.
     * @return false If a non-overridable pulse is in progress or on mutex timeout.
     */
    bool toggle(bool force = true) {
        if (!_lock()) return false;
        bool retval = (!_isPulsing || force);
        if (retval) {
            // If a pulse is active, we toggle based on the actual transient state
            // but stop the pulse tracker
            _isPulsing = false; // Manual override terminates any active timed pulse
            _state = (_state == State::ON) ? State::OFF : State::ON;
            _operate();
        }
        _unlock();
        return retval;
    }

    /**
     * @brief Triggers a non-blocking timed pulse operation.
     * 
     * Generates either a positive (ON), negative (OFF), or inverted (TOGGLE) pulse for a 
     * specified duration. Once the timer expires, the output automatically reverts to its 
     * designated resting state.
     * 
     * @param interval Duration of the pulse in milliseconds.
     * @param type The behavior of the pulse (POSITIVE, NEGATIVE, or TOGGLE).
     *             Defaults to POSITIVE.
     * @param force If true, new pulse call overrides the previous one
     * @return true If the pulse was successfully initiated.
     * @return false If the mutex acquisition timed out or if force mode disabled
     *         and a pulse is in progress.
     */
    bool pulse(uint32_t interval, PulseType type = PulseType::POSITIVE, bool force = true) {
        if (!_lock()) return false;
        bool retval = (force || !_isPulsing);
        if (retval) {
            // Set values just for eliminating compiler warnings.
            State targetState = _state;
            State recoveryState = _savedState;

            switch(type) {
                case PulseType::POSITIVE: {
                    targetState = State::ON;
                    recoveryState = State::OFF;
                    break;
                }
                case PulseType::NEGATIVE: {
                    targetState = State::OFF;
                    recoveryState = State::ON;
                    break;
                }
                case PulseType::TOGGLE: {
                    // If a pulse is already active, we use the already saved baseline state to invert from.
                    State currentBaseline = _isPulsing ? _savedState : _state;
                    targetState = (currentBaseline == State::ON) ? State::OFF : State::ON;
                    recoveryState = currentBaseline;
                    break;
                }
            }

            _state = targetState;
            _savedState = recoveryState;
            _pulseStartTime = millis();
            _pulseInterval = interval;
            _isPulsing = true;

            _operate();
        }
        _unlock();
        return retval;
    }

    /**
     * @brief Non-blocking state refresher. Must be called periodically within the main loop.
     * 
     * Checks if an active pulse interval has expired and gracefully returns the output 
     * to its designated baseline recovery state.
     */
    void update() {
        // Quick lock-free check to avoid CPU overhead when no pulse is active
        if (!_isPulsing) return; 

        if (!_lock()) return;
        // Re-verify inside the protected context
        if (_isPulsing && (millis() - _pulseStartTime >= _pulseInterval)) {
            _state = _savedState; // Revert to the dynamically saved recovery state
            _isPulsing = false;
            _operate();
        }
        _unlock();
    }

    /**
     * @brief Returns the current logical state of the output.
     *
     * @note While a timed pulse (see pulse()) is active, this reflects the
     *       transient pulsed state, not the baseline the output will
     *       revert to once the pulse expires. Use getBaseline() if you
     *       need the resting state instead.
     * @return The current State (ON or OFF). If the mutex times out, the
     *         last cached value is returned without waiting, which may be
     *         momentarily stale under heavy contention.
     */
    State getState() const {
        if (!_lock()) return _state;
        State current = _state;
        _unlock();
        return current;
    }

    /**
     * @brief Returns the resting ("baseline") state the output will settle
     *        into once any active pulse expires.
     *
     * @note Unlike getState(), this is unaffected by a currently running
     *       pulse. When no pulse is active, getBaseline() and getState()
     *       always return the same value.
     * @return The baseline State (ON or OFF). Subject to the same mutex
     *         timeout caveat as getState().
     */
    State getBaseline() const {
        if (!_lock()) return _isPulsing ? _savedState : _state;
        State baseline = _isPulsing ? _savedState : _state;
        _unlock();
        return baseline;
    }

protected:
    int _pin;               /**< Assigned identifier (GPIO pin, I2C channel, etc.). */
    bool _activeState;      /**< Hardware level representing ON. */
    bool _inactiveState;    /**< Hardware level representing OFF. */
    State _state;           /**< Current cached logical state. */

    // Pulse tracking variables protected for potential subclass usage
    State _savedState;      /**< The state the output must return to after the pulse expires. */
    bool _isPulsing;        /**< True if a timed pulse operation is currently active. */
    uint32_t _pulseStartTime;  /**< Timestamp (in ms) when the active pulse started. */
    uint32_t _pulseInterval;   /**< Configured duration (in ms) of the active pulse. */

    /**
     * @brief Default hardware-level initialization for local GPIO pins.
     * @note Overridable by subclasses to implement alternative hardware (e.g., I2C expanders).
     */
    virtual void _init() {
        // Enforce glitch-free sequence: write level before setting output mode
        digitalWrite(_pin, (_state == State::ON) ? _activeState : _inactiveState);
        pinMode(_pin, OUTPUT);
    }

    /**
     * @brief Default hardware-level operation for local GPIO pins.
     * @note Overridable by subclasses to write to alternative interfaces.
     */
    virtual void _operate() {
        digitalWrite(_pin, (_state == State::ON) ? _activeState : _inactiveState);
    }

private:
#if !defined(ARDUINO_ARCH_AVR) && !defined(ARDUINO_ARCH_ESP8266)
    mutable std::timed_mutex _mutex;
    bool _lock() const { return _mutex.try_lock_for(std::chrono::milliseconds(DIGITAL_OUTPUT_MUTEX_TIMEOUT)); }
    void _unlock() const { _mutex.unlock(); }
#else
    bool _lock()   const { return true; }
    void _unlock() const {}
#endif
};
