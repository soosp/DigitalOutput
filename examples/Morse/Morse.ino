/**
 * @file Morse.ino
 * @brief Example sketch to emit Morse code on the built-in LED.
 * 
 * It demonstrates how it works when a pulse cannot be triggered again until
 * the previous one has ended using DigitalOutput library.
 * This sketch uses blocking functions tho emit Morse signs and spaces.
 * It's not a typical use of the library, but it was fun to make. :)
 */

#include <Arduino.h>
#include <DigitalOutput.h>
#include <cctype>

/** 
 * @brief DigitalOutput constructor.
 *
 * LED is connected to GPIO pin 46 with active HIGH logic.
 * Change it to match with your board's specification.
 */
DigitalOutput led(46, false);

/**
 * @brief Duration of a time unit in miliseconds.
 *
 * The duration of the signals and spaces is a multiple of this.
 * You can adjust it based on your own level of proficiency in Morse code. :)
 */
const uint32_t time_unit = 100;

/**
 * @brief Duration of dit signal in miliseconds.
 *
 * It is one time unit long.
 */
const uint32_t dit_len = time_unit;

/** 
 * @brief Duration of dah signal in miliseconds.
 * 
 * It is three time units long.
 */
const uint32_t dah_len = 3 * time_unit;

/**
 * @brief Duration of space between dits and dahs in miliseconds.
 *
 * It is equal to the dit's duration, one time unit long.
 */
const uint32_t space_len = time_unit;

/**
 * @brief Duration of space between letters in miliseconds.
 * 
 * It is three time units long.
 */
const uint32_t letter_space_len = 3 * time_unit;

/**
 * @brief Duration of space between words in miliseconds.
 * 
 * It is seven time units long.
 */
const uint32_t word_space_len = 7 * time_unit;

/**
 * @brief Blocking function for a single dit.
 */
void dit(void) {
    while (!led.pulse(dit_len, DigitalOutput::PulseType::POSITIVE, false)) { led.update(); }
}

/**
 * @brief Blocking function for a single dah.
 */
void dah(void) {
    while (!led.pulse(dah_len, DigitalOutput::PulseType::POSITIVE, false)) { led.update(); }
}

/**
 * @brief Blocking function for a space between signals.
 */
void space(void) {
    while (!led.pulse(space_len, DigitalOutput::PulseType::NEGATIVE, false)) { led.update(); }
}

/**
 * @brief Blocking function for a space between letters.
 */
void letter_space(void) {
    while (!led.pulse(letter_space_len, DigitalOutput::PulseType::NEGATIVE, false)) { led.update(); }
}

/**
 * @brief Blocking function for a space between words.
 */
void word_space(void) {
    while (!led.pulse(word_space_len, DigitalOutput::PulseType::NEGATIVE, false)) { led.update(); }
}

/**
 * @brief Structure for mapping letters to Morse code symbols.
 */
struct MorseMapping {
    char character;
    const char* pattern;
};

/**
 * @brief Structure to map a character to it's Morse code representation.
 */
const MorseMapping morse_table[] = {
    // --- Alphabet (A-Z) ---
    {'A', ".-"},   {'B', "-..."}, {'C', "-.-."}, {'D', "-.."},  {'E', "."},
    {'F', "..-."}, {'G', "--."},  {'H', "...."}, {'I', ".."},   {'J', ".---"},
    {'K', "-.-"},  {'L', ".-.."}, {'M', "--"},   {'N', "-."},   {'O', "---"},
    {'P', ".--."}, {'Q', "--.-"}, {'R', ".-."},  {'S', "..."},  {'T', "-"},
    {'U', "..-"},  {'V', "...-"}, {'W', ".--"},  {'X', "-..-"}, {'Y', "-.--"},
    {'Z', "--.."},
    
    // --- Numbers (0-9) ---
    {'1', ".----"}, {'2', "..---"}, {'3', "...--"}, {'4', "....-"}, {'5', "....."},
    {'6', "-...."}, {'7', "--..."}, {'8', "---.."}, {'9', "----."}, {'0', "-----"},
    
    // --- Punctuation and Symbols ---
    {'.', ".-.-.-"},   // Period / Full stop
    {',', "--..--"},   // Comma
    {'?', "..--.."},   // Question mark
    {'!', "-.-.--"},   // Exclamation mark
    {':', "---..."},   // Colon
    {';', "-.-.-."},   // Semicolon
    {'-', "-....-"},   // Hyphen / Minus
    {'/', "-..-."},    // Slash / Division
    {'_', "..--.-"},   // Underscore
    {'"', ".-..-."},   // Quotation mark
    {'\'', ".----."},  // Apostrophe
    {'(', "-.--."},    // Open parenthesis
    {')', "-.--.-"},   // Close parenthesis
    {'=', "-...-"},    // Equals sign / "Break" (BT prosign)
    {'+', ".-.-."},    // Plus sign / "End of message" (AR prosign)
    {'@', ".--.-."},   // At sign
    {'&', ".-..."},    // Ampersand / "Wait" (AS prosign)
    {'$', "...-..-"},  // Dollar sign
    
    // --- Procedural Signals (Prosigns) ---
    {'*', "........"}, // Error signal (8 dits)
    {'%', "...-.-"}    // End of work / Sign-off (SK prosign)
};

/**
 * @brief Total number of elements in the morse_table.
 */
const int morse_table_size = sizeof(morse_table) / sizeof(MorseMapping);

/** 
 * @brief Converts a letter to Morse code signals.
 * @param letter The character to convert.
 */
void morse_letter(const char letter) {
    char upper_letter = std::toupper(letter); 
    
    for (int i = 0; i < morse_table_size; i++) {
        if (morse_table[i].character == upper_letter) {
            const char* pattern = morse_table[i].pattern;
            int j = 0;
            while (pattern[j] != '\0') {
                if (pattern[j] == '.') dit();
                else if (pattern[j] == '-') dah();
                // Do not emit space after the last signal
                if (pattern[j + 1] != '\0') space();
                j++;
            }
            return;
        }
    }
}

/** 
 * @brief Converts a string to Morse code signals.
 * @param str The character buffer to convert.
 */
void morse_string(const char* str) {
    int i = 0;
    while (str[i] != '\0') {
        
        if (str[i] == ' ') {
            word_space();
        } else {
            morse_letter(str[i]);
            char next_char = str[i + 1];
            if (next_char != '\0' && next_char != ' ') {
                letter_space();
            }
        }
        i++;
    }
    word_space();
}

/**
 * @brief Standard Arduino setup function.
 */
void setup() {
    Serial.begin(115200);
    if (Serial) // Is ESP32 CDC port is connected? Always true on UART.
        Serial.println("DigitalOutput Morse Demonstration Started.");

    // Initialize the device safely in the OFF state
    led.begin(DigitalOutput::State::OFF);
}

/**
 * @brief Standard Arduino application loop.
 */
void loop() {
    // Actually, update isn't necessary here because each function use its own
    // update loop.
    // led.update();

    const char* message = "DigitalOutput library Morse demonstration.";
    // const char* message = "SOS";

    if (Serial) { // Is ESP32 CDC port is connected? Always true on UART.
        Serial.print("Starting message: ");
        Serial.println(message);
    }
    morse_string(message);
}
