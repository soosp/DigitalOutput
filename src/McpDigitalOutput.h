#pragma once

#include "DigitalOutput.h"
#include <Adafruit_MCP23X08.h>
#include <Adafruit_MCP23X17.h>
#if !defined(ARDUINO_ARCH_AVR) && !defined(ARDUINO_ARCH_ESP8266)
#include <mutex>
#endif

/**
 * @class McpDigitalOutput
 * @brief Subclass of DigitalOutput designed to control a digital output via an MCP23X08/MCP23X17 I2C/SPI port expander.
 * 
 * This class inherits all state machine logic from the base DigitalOutput class, but overrides 
 * the hardware-level interface to communicate with an Adafruit_MCP23XXX base expander object.
 * 
 * @note Since the Adafruit MCP23017 Arduino Library and the underlying I2C Wire library are NOT
 *       thread-safe by default, this subclass utilizes an external shared timed mutex 
 *       to protect all hardware communication with the expander chip across multiple tasks.
 */
class McpDigitalOutput : public DigitalOutput {
public:
#if !defined(ARDUINO_ARCH_AVR) && !defined(ARDUINO_ARCH_ESP8266)
    /**
     * @brief Constructs a new McpDigitalOutput object using Adafruit_MCP23X08 expander object.
     * 
     * @param mcp Reference to the initialized Adafruit_MCP23X08 expander object.
     * @param pin The expander pin number (0 to 7) where the digital output device is connected.
     * @param shared_mutex Reference to a std::timed_mutex shared among ALL devices on this specific I2C bus.
     * @param active_low Set to true if the output triggers on LOW (default), or false for HIGH.
     */
    McpDigitalOutput(Adafruit_MCP23X08 &mcp, int pin, std::timed_mutex &shared_mutex, bool active_low = true) 
        : DigitalOutput(pin, active_low), _mcp(mcp), _mcpMutex(&shared_mutex) {}

    /**
     * @brief Constructs a new McpDigitalOutput object using Adafruit_MCP23X17 expander object.
     * 
     * @param mcp Reference to the initialized Adafruit_MCP23X17 expander object.
     * @param pin The expander pin number (0 to 15) where the digital output device is connected.
     * @param shared_mutex Reference to a std::timed_mutex shared among ALL devices on this specific I2C bus.
     * @param active_low Set to true if the output triggers on LOW (default), or false for HIGH.
     */
    McpDigitalOutput(Adafruit_MCP23X17 &mcp, int pin, std::timed_mutex &shared_mutex, bool active_low = true) 
        : DigitalOutput(pin, active_low), _mcp(mcp), _mcpMutex(&shared_mutex) {}
#else
    /**
     * @brief Constructs a new McpDigitalOutput object using Adafruit_MCP23X08 expander object.
     * 
     * @param mcp Reference to the initialized Adafruit_MCP23X08 expander object.
     * @param pin The expander pin number (0 to 7) where the digital output device is connected.
     * @param active_low Set to true if the output triggers on LOW (default), or false for HIGH.
     */
    McpDigitalOutput(Adafruit_MCP23X08 &mcp, int pin, bool active_low = true) 
        : DigitalOutput(pin, active_low), _mcp(mcp) {}

    /**
     * @brief Constructs a new McpDigitalOutput object using Adafruit_MCP23X17 expander object.
     * 
     * @param mcp Reference to the initialized Adafruit_MCP23X17 expander object.
     * @param pin The expander pin number (0 to 15) where the digital output device is connected.
     * @param active_low Set to true if the output triggers on LOW (default), or false for HIGH.
     */
    McpDigitalOutput(Adafruit_MCP23X17 &mcp, int pin, bool active_low = true) 
        : DigitalOutput(pin, active_low), _mcp(mcp) {}
#endif

    /**
     * @brief Destructor for McpDigitalOutput. 
     * @note The referenced MCP23X08/MCP23X17 object is managed outside this class and will not be destroyed.
     */
    ~McpDigitalOutput() override = default;

protected:
    /**
     * @brief Hardware-level initialization for the MCP23X08/MCP23X17 expander pin.
     * 
     * Overrides the base method to ensure a glitch-free initialization sequence on the expander 
     * by locking the shared hardware mutex, applying digitalWrite, and then setting the pin mode.
     */
    bool _init() override {
        if (_extLock()) {
            _mcp.digitalWrite(_pin, (_state == State::ON) ? _activeState : _inactiveState); // Write initial level first to prevent startup spikes
            _mcp.pinMode(_pin, OUTPUT);
            _extUnlock();
            return true;
        }
        return false; // Bus mutex timed out: report failure so the caller can roll back
    }

    /**
     * @brief Hardware-level operation that safely writes the logical state to the MCP23X08/MCP23X17 expander pin.
     * 
     * Wraps the I2C/SPI transaction inside the external shared mutex to prevent collisions with other 
     * tasks accessing the same expander chip.
     */
    bool _operate() override {
        if (_extLock()) {
            _mcp.digitalWrite(_pin, (_state == State::ON) ? _activeState : _inactiveState);
            _extUnlock();
            return true;
        }
        return false; // Bus mutex timed out: report failure so the caller can roll back
    }

private:
    /** 
     * @brief Reference to the base expander class.
     * 
     * By using the Adafruit_MCP23XXX base class reference, this variable can polymorphically 
     * hold and operate on both Adafruit_MCP23X08 and Adafruit_MCP23X17 objects.
     */
    Adafruit_MCP23XXX &_mcp; 
    
#if !defined(ARDUINO_ARCH_AVR) && !defined(ARDUINO_ARCH_ESP8266)
    std::timed_mutex *_mcpMutex; /**< Pointer to the shared hardware timed mutex. */

    /**
     * @brief Internal helper to lock the external shared expander mutex.
     * @return true If the lock was successfully acquired.
     * @return false If the operation timed out.
     */
    bool _extLock() {
        if (!_mcpMutex) return true;
        return _mcpMutex->try_lock_for(std::chrono::milliseconds(DIGITAL_OUTPUT_MUTEX_TIMEOUT));
    }
    
    /**
     * @brief Internal helper to unlock the external shared expander mutex.
     */
    void _extUnlock() {
        if (_mcpMutex) _mcpMutex->unlock();
    }
#else
    // Fallback stubs for legacy single-core architectures with no shared hardware locking required
    bool _extLock()   { return true; }
    void _extUnlock() {}
#endif
};