/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <stdio.h>
#include <cmath>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "MacroMath.h"

#define NUM_GATE_OUTS 6
#define NUM_LEDS 3

#define GPIO_CLK 0
#define GPIO_RST 1
#define GPIO_PLAY 14

#define GPIO_ADC 26

#define GPIO_TMULT_A 15
#define GPIO_TMULT_B 10


enum LEDState
{
    SOLID_ON,
    SOLID_OFF,
    SOLID_HALF,
    BLINK_SLOW,
    BLINK_MED,
    BLINK_FAST,
    FADE_SLOWEST,
    FADE_SLOW,
    FADE_MED,
    FADE_FAST,
    FADE_FASTER,
    FADE_FASTEST,
};

/// @brief Gate LEDs arent here, they're hardwired to the gate outs
enum PanelLED
{
    Reset,
    Clock,
    PlayButton
};

class IOHelper
{
    private:
        /// @brief Gate Out Pin numbers
        /// @note The pins appear in the following order:
        /// @note 0: Whole Note Out
        /// @note 1: Half Note Out
        /// @note 2: Quarter Note Out
        /// @note 3: Sixteenth Note Out
        /// @note 4: User Division Out
        /// @note 5: UD / 2 Out
        const uint8_t GATE_OUT_PINS[NUM_GATE_OUTS] = {4, 5, 6, 7, 8, 9};

        /// @brief LED Pin numbers
        /// @note Gate out LEDs are hardwired to the gate outs to save I/O pins.
        /// @note The pins appear in the following order:
        /// @note 0: Reset LED
        /// @note 1: Clock LED
        /// @note 2: Play Button LED
        const uint8_t LED_IO_PINS[NUM_LEDS] = {2, 3, 16};

        /// @brief Pins that connect to the address select lines of the ADC MUX
        const uint8_t MUX_ADDR_PINS[3] = {11, 12, 13};

        /// @brief To keep track of previous-update values; Used for debouncing
        bool WAS_PLAY  = false;
        /// @brief To keep track of previous-update values; Used for debouncing
        bool WAS_CLK   = false;
        /// @brief To keep track of previous-update values; Used for debouncing
        bool WAS_RST   = false;
        /// @brief To keep track of previous-update values; Used for debouncing
        uint8_t LAST_TM_SWITCH    = 0;
        
        
        uint16_t LEDCycle = 0;

        /// @brief Reads a value from the ADC through the MUX
        /// @param addr MUX address
        /// @return Raw ADC value, 0-4095
        /// @note Includes a 20 uS settling delay to avoid cross-talk
        int16_t ReadADC(uint8_t addr);

        /// @brief RETURNED VALUES: 
        /// @param adcValue raw value from the ADC
        /// @param zero the ADC's raw value when the CV in is set to 0V
        /// @param five the ADC's raw value when the CV in is set to +5V
        /// @return Unclamped, voltage-calibrated value. -5V = -2048, 0V = 0, 5V = 2048
        /// @note negative on bipolars only reads to -4.5V, while positive reads to about 5.5V
        int16_t GetCalibratedValue(uint16_t adcValue, uint16_t zero, uint16_t five);

        /// @brief Does Hysterises on an input
        /// @param var variable we're writing to, used to compare to new value 
        /// @param newValue newly read value
        /// @param hysteresisThreshold any newValue less than this amt away from the previous value will be ignored
        /// @return new value of var, with hysterises applied
        int16_t DoHysteresisWrite(int16_t var, int16_t newValue, int16_t hysteresisThreshold);

    public:

        //-------- Outs --------

        /// @brief RESET, CLOCK, PLAY
        LEDState OUT_LEDS[NUM_LEDS];

        /// @brief 1, 1/2, 1/4, 1/16, UD, UD/2
        bool OUT_GATES[NUM_GATE_OUTS];


        //-------- CV Inputs --------

        /// @brief User Division CV Value.
        int16_t CV_UD_Raw           = 0;
        /// @brief Scrub/Offset CV Value.
        int16_t CV_scrub_Raw        = 0;
        /// @brief Time Multiply CV Value.
        int16_t CV_timeMult_Raw     = 0;
        /// @brief Swing CV Value.
        int16_t CV_swing_Raw        = 0;
        /// @brief User Division CV Value.
        int16_t CV_UD           = 0;
        /// @brief Scrub/Offset CV Value.
        int16_t CV_scrub        = 0;
        /// @brief Time Multiply CV Value.
        int16_t CV_timeMult     = 0;
        /// @brief Swing CV Value.
        int16_t CV_swing        = 0;

        /// @brief User Division modifier
        int16_t CV_UD_Mult          = 0;


        //-------- Panel Knobs --------

        /// @brief Current state of the BPM Knob
        /// @note Analog value with hysterises compensation
        /// @note 0-4095
        int16_t IN_BPM_KNOB     = 0;

        /// @brief Current state of the Swing Knob
        /// @note Analog value with hysterises compensation
        /// @note 0-4095
        int16_t IN_SWING_KNOB   = 0;


        //-------- Panel Buttons and Switches --------

        /// @brief Current state of the Play Button
        bool    IN_PLAY_BTN     = false;

        /// @brief Current state of the TMULT switch.
        /// @note 0: up
        /// @note 1: center
        /// @note 2: down
        uint8_t IN_TMULT_SWITCH = 0;

        /// @brief Current state of the UD Knob
        /// @note Integer index, 0-6
        uint8_t IN_UD_INDEX     = 0;


        //-------- FLAGS --------

        /// @brief Set when PLAY button is pressed.
        bool FLAG_PLAY  = false;
        /// @brief Set when CLOCK IN gate goes high.
        bool FLAG_CLK   = false;
        /// @brief Set when RESET IN gate goes high.
        bool FLAG_RST   = false;

        /// @brief Set when the state of the TMULT switch is changed.
        bool FLAG_TMULT = false;
        

        //-------- METHODS --------

        /// @brief Initializes the IOHelper. Must be called before using.
        void Init();

        /// @brief "Audio rate" update. Should be called as often as reasonably possible, and at an even interval.
        /// @param dt the actual time in microseconds since the last time this was called
        void ReadFastInputs(long dt);

        /// @brief "CV rate" update. Should be called as fast as reasonable, but is not as time-critical as ReadFastInputs
        /// @param dt the actual time in microseconds since the last time this was called
        void ReadSlowInputs(long dt);

        /// @brief Pushes the output values to the hardware, doing necessary calculations. Should be called in "Audio rate" loop after setting output values.
        /// @param dt the actual time in microseconds since the last time this was called
        void WriteFastOutputs(long dt);

        /// @brief Pushes the output values to the hardware, doing necessary calculations. Should be called in "CV rate" loop after setting output values.
        /// @param dt the actual time in microseconds since the last time this was called
        void WriteSlowOutputs(long dt);

        /// @brief Checks if FLAG_RST is set, if so unsets it and returns true
        /// @return true if FLAG_RST was set, false otherwise
        bool ProcessResetFlag();
        
        /// @brief Checks if FLAG_CLK is set, if so unsets it and returns true
        /// @return true if FLAG_CLK was set, false otherwise
        bool ProcessClockFlag();
        
        /// @brief Checks if FLAG_PLAY is set, if so unsets it and returns true
        /// @return true if FLAG_PLAY was set, false otherwise
        bool ProcessPlayFlag();

        /// @brief Sets the display pattern/state of a panel LED.
        /// @param led The LED to set the pattern of
        /// @param state the pattern to set
        void SetLEDState(PanelLED led, LEDState state);
};