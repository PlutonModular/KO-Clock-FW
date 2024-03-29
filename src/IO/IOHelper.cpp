/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "IOHelper.hpp"

void IOHelper::Init()
{
    //--------Set up Inputs--------

    //Play Button
    gpio_init(GPIO_PLAY);
    gpio_set_dir(GPIO_PLAY, GPIO_IN);
    gpio_set_pulls(GPIO_PLAY, true, false);
    //Clock and Reset Gate Ins
    gpio_init(GPIO_CLK);
    gpio_set_dir(GPIO_CLK, GPIO_IN);
    gpio_set_pulls(GPIO_CLK, true, false);
    gpio_init(GPIO_RST);
    gpio_set_dir(GPIO_RST, GPIO_IN);
    gpio_set_pulls(GPIO_RST, true, false);
    //Time Mult Switch
    gpio_init(GPIO_TMULT_A);
    gpio_set_dir(GPIO_TMULT_A, GPIO_IN);
    gpio_set_pulls(GPIO_TMULT_A, true, false);
    gpio_init(GPIO_TMULT_B);
    gpio_set_dir(GPIO_TMULT_B, GPIO_IN);
    gpio_set_pulls(GPIO_TMULT_B, true, false);

    //--------Set up Outputs--------

    //Heartbeat LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    //Gate Outs
    for(int i = 0; i < NUM_GATE_OUTS; i++)
    {
        gpio_init(GATE_OUT_PINS[i]);
        gpio_set_dir(GATE_OUT_PINS[i], GPIO_OUT);
    }
    //LED Outs
    for(int i = 0; i < NUM_LEDS; i++)
    {

        gpio_init(LED_IO_PINS[i]);
        gpio_set_dir(LED_IO_PINS[i], GPIO_OUT);
    }
    //MUX Address Lines
    for(int i = 0; i < 3; i++)
    {
        gpio_init(MUX_ADDR_PINS[i]);
        gpio_set_dir(MUX_ADDR_PINS[i], GPIO_OUT);
    }

    //--------Set up ADC--------

    adc_init();
    adc_gpio_init(GPIO_ADC);
    adc_select_input(0);
}

void IOHelper::ReadFastInputs(long dt)
{
    //sets input flags, so they can be processed at any speed
    bool TMP_CLK  = !gpio_get(GPIO_CLK); //these are active low
    bool TMP_RST  = !gpio_get(GPIO_RST);
    if(!WAS_CLK  && TMP_CLK)  {     FLAG_CLK = true;    }
    if(!WAS_RST  && TMP_RST)  {     FLAG_RST = true;    }
    WAS_CLK  = TMP_CLK;
    WAS_RST  = TMP_RST;
}

int16_t IOHelper::DoHysteresisWrite(int16_t var, int16_t newValue, int16_t hysteresisThreshold)
{
    if(abs(newValue - var) > hysteresisThreshold)
    {
        return newValue;
    }
    return var;
}

void IOHelper::ReadSlowInputs(long dt)
{
    //--------Read Play Button--------

    IN_PLAY_BTN = !gpio_get(GPIO_PLAY); //active low
    if(!WAS_PLAY && IN_PLAY_BTN) {     FLAG_PLAY = true;   }
    WAS_PLAY = IN_PLAY_BTN;

    //--------Read Time Mult Switch--------
    
    //read value
    IN_TMULT_SWITCH = 1; //centered
    if     (!gpio_get(GPIO_TMULT_A)) IN_TMULT_SWITCH = 0; //up      (these are both active low)
    else if(!gpio_get(GPIO_TMULT_B)) IN_TMULT_SWITCH = 2; //down
    //set flag
    if(LAST_TM_SWITCH != IN_TMULT_SWITCH) FLAG_TMULT = true; //change flag
    LAST_TM_SWITCH = IN_TMULT_SWITCH;

    //--------Read Knobs--------
    IN_SWING_KNOB   = DoHysteresisWrite(IN_SWING_KNOB,  ReadADC(0), 50);
    IN_BPM_KNOB     = DoHysteresisWrite(IN_BPM_KNOB,    ReadADC(1), 30);
    int16_t IN_UD_KNOB = ReadADC(2);

    //TODO: better hysterises method, and proper smoothing. This is fine for the demo but kind of cringe tbh
    CV_UD_Raw +=        CV_UD_Raw       + CV_UD_Raw       + CV_UD_Raw       + CV_UD_Raw       + CV_UD_Raw       + CV_UD_Raw       + ReadADC(3);
    CV_scrub_Raw +=     CV_scrub_Raw    + CV_scrub_Raw    + CV_scrub_Raw    + CV_scrub_Raw    + CV_scrub_Raw    + CV_scrub_Raw    + ReadADC(4);
    CV_timeMult_Raw +=  CV_timeMult_Raw + CV_timeMult_Raw + CV_timeMult_Raw + CV_timeMult_Raw + CV_timeMult_Raw + CV_timeMult_Raw + ReadADC(5);
    CV_swing_Raw +=     CV_swing_Raw    + CV_swing_Raw    + CV_swing_Raw    + CV_swing_Raw    + CV_swing_Raw    + CV_swing_Raw    + ReadADC(6);
    CV_UD_Raw /= 8;
    CV_scrub_Raw /= 8;
    CV_timeMult_Raw /= 8;
    CV_swing_Raw /= 8;

    CV_UD       = DoHysteresisWrite(CV_UD      , GetCalibratedValue(CV_UD_Raw      , 2300, 340), 30);
    CV_scrub    = DoHysteresisWrite(CV_scrub   , GetCalibratedValue(CV_scrub_Raw   , 2300, 340), 60);
    CV_timeMult = DoHysteresisWrite(CV_timeMult, GetCalibratedValue(CV_timeMult_Raw, 2300, 340), 30);
    CV_swing    = DoHysteresisWrite(CV_swing   , max(min(CV_swing_Raw, 4096), 36)-36, 30); //TODO: make this work with getCalibratedValue

    if(abs(CV_UD) < 20)       CV_UD = 0;
    if(abs(CV_scrub) < 40)    CV_scrub = 0;
    if(abs(CV_timeMult) < 20) CV_timeMult = 0;
    if(abs(CV_swing) < 20)    CV_swing = 0;

    CV_UD_Mult = (CV_UD/800.0);

    //Set UD index (only if not grounded; if grounded, UD selector is between positions)
    if(IN_UD_KNOB > 128)
    {
        IN_UD_INDEX = IN_UD_KNOB/(4095/7);
    }
}

//INPUT     0V      5V      -4.5V   -5V (projected)
//scrub     2305    350     4095    4550
//tmult     2300    330     4095    4550
//UD        2300    340     4095    4550
//swing     36      3010

int16_t IOHelper::GetCalibratedValue(uint16_t adcValue, uint16_t zero, uint16_t five)
{
    //input value is from 0-4096
    long adcValueReal = long(adcValue)*256L;

    //center 0
    adcValueReal -= long(zero)*256L;

    //bring maximum to scaled 5V
    adcValueReal *= long((five - zero));

    //adcValueReal *= 4L;

    adcValueReal /= 4096L;

    adcValueReal -= long(zero);

    adcValueReal /= 128L;

    if(adcValueReal >= INT16_MAX)
    {
        adcValueReal = INT16_MAX - 1;
    }
    else if(adcValueReal <= INT16_MIN)
    {
        adcValueReal = INT16_MIN + 1;
    }

    return adcValueReal;
}

int16_t IOHelper::ReadADC(uint8_t addr)
{
    //write address to addr bus
    for(int i = 0; i < 3; i++)
    {
        gpio_put(MUX_ADDR_PINS[i], (addr & (0b00000001 << i)) > 0);
        //printf("TEST %u\t%d\n", i, (addr & (0b00000001 << i)) > 0);
    }
    //allow MUX and ADC to settle
    sleep_us(100);
    uint16_t adcVal = adc_read();
    return adcVal;
}

void IOHelper::WriteSlowOutputs(long dt)
{
    //Increment LED Cycle; used for "blinking" LED states
    LEDCycle += dt/100;
    //Set LEDs
    for(int i = 0; i < NUM_LEDS; i++)
    {
        bool thisLedState = false;
        switch(OUT_LEDS[i])
        {
            //-------- SOLID --------
            case LEDState::SOLID_OFF:
                thisLedState = false;
                break;
            case LEDState::SOLID_ON:
                thisLedState = true;
                break;
            case LEDState::SOLID_HALF:
                if(i == PanelLED::PlayButton)
                    thisLedState = LEDCycle%32 > 26; //gives a better "50% brightness" on the button LED
                else
                    thisLedState = LEDCycle%32 > 28; //gives a better "50% brightness" on the rect LEDs
                break;
            //-------- BLINK --------
            case LEDState::BLINK_SLOW:
                thisLedState = LEDCycle%8192 > 4096;
                break;
            case LEDState::BLINK_MED:
                thisLedState = LEDCycle%4096 > 2048;
                break;
            case LEDState::BLINK_FAST:
                thisLedState = LEDCycle%2048 > 1024;
                break;
            //-------- FADE --------
            case LEDState::FADE_SLOWEST:
            {
                int fadeBrightness = abs((LEDCycle) - 32'768); //0-32'768 (no %, clamped by uint16 limit)
                thisLedState = LEDCycle%64 > fadeBrightness/512;
                break;
            }
            case LEDState::FADE_SLOW:
            {
                int fadeBrightness = abs((LEDCycle%32'768) - 16'384); //0-16'384
                thisLedState = LEDCycle%64 > fadeBrightness/256;
                break;
            }
            case LEDState::FADE_MED:
            {
                int fadeBrightness = abs((LEDCycle%16'384) - 8'192); //0-8192
                thisLedState = LEDCycle%32 > fadeBrightness/256;
                break;
            }
            case LEDState::FADE_FAST:
            {
                int fadeBrightness = abs((LEDCycle%8'192) - 4'096); //0-4096
                thisLedState = LEDCycle%16 > fadeBrightness/256;
                break;
            }
            case LEDState::FADE_FASTER:
            {
                int fadeBrightness = abs((LEDCycle%2'048) - 1'024); //0-1024
                thisLedState = LEDCycle%16 > fadeBrightness/64;
                break;
            }
            case LEDState::FADE_FASTEST:
            {
                int fadeBrightness = abs((LEDCycle%1'024) - 512); //0-512
                thisLedState = LEDCycle%16 > fadeBrightness/32;
                break;
            }
        }
        gpio_put(LED_IO_PINS[i], thisLedState);
    }
}
void IOHelper::WriteFastOutputs(long dt)
{
    //Set Gates
    for(int i = 0; i < NUM_GATE_OUTS; i++)
    {
        gpio_put(GATE_OUT_PINS[i], OUT_GATES[i]);
    }
}

bool IOHelper::ProcessPlayFlag()
{
    if(FLAG_PLAY) {  FLAG_PLAY = false;   return true;  }
    return false;
}

bool IOHelper::ProcessClockFlag()
{
    if(FLAG_CLK) {  FLAG_CLK = false;   return true;  }
    return false;
}

bool IOHelper::ProcessResetFlag()
{
    if(FLAG_RST) {  FLAG_RST = false;   return true;  }
    return false;
}

void IOHelper::SetLEDState(PanelLED led, LEDState state)
{
    OUT_LEDS[(uint8_t)led] = state;
}