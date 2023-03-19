#include "IOHelper.hpp"

void IOHelper::Init()
{
    //set up outputs
    for(int i = 0; i < NUM_GATE_OUTS; i++)
    {
        gpio_init(GATE_IO_PINS[i]);
        gpio_set_dir(GATE_IO_PINS[i], true);
    }

    for(int i = 0; i < NUM_LEDS; i++)
    {

        gpio_init(LED_IO_PINS[i]);
        gpio_set_dir(LED_IO_PINS[i], true);
    }
    for(int i = 0; i < 3; i++)
    {
        gpio_init(MUX_ADDR_PINS[i]);
        gpio_set_dir(MUX_ADDR_PINS[i], true);
    }
    //set up ADC
    adc_init();
    adc_gpio_init(GPIO_ADC);
    adc_select_input(0);
}

void IOHelper::ReadFastInputs(long dt)
{
    if(!FLAG_PLAY && gpio_get(GPIO_PLAY)) {     FLAG_PLAY = true;   }
    if(!FLAG_CLK  && gpio_get(GPIO_CLK))  {     FLAG_CLK = true;    }
    if(!FLAG_RST  && gpio_get(GPIO_RST))  {     FLAG_RST = true;    }
}

void IOHelper::ReadSlowInputs(long dt)
{
    //Increment LED Cycle
    LEDCycle += dt/10'000;

    //Read Knobs
    IN_SWING_KNOB = ReadADC(0);
    IN_BPM_KNOB = ReadADC(1);
    int16_t IN_UD_KNOB = ReadADC(2);
    CV_UD = GetCalibratedValue(ReadADC(3), 2300, 340);
    CV_scrub = GetCalibratedValue(ReadADC(4), 2300, 340);
    CV_timeMult = GetCalibratedValue(ReadADC(5), 2300, 340);
    CV_swing = GetCalibratedValue(ReadADC(6), 36, 3010);

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

///RETURNED VALUES: -5V = -2048, 0V = 0, 5V = 2048
///reminder: negative on bipolars only reads to -4.5V, while positive reads to abour 5.5V..
int16_t IOHelper::GetCalibratedValue(uint16_t adcValue, uint16_t zero, uint16_t five)
{
    //input value is from 0-4096
    long adcValueReal = long(adcValue)*256L;

    //center 0
    adcValueReal -= long(zero)*256L;

    //bring maximum to scaled 5V
    adcValueReal *= (long((five - zero)));

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
    sleep_us(10);
    uint16_t adcVal = adc_read();
    return adcVal;
}

void IOHelper::WriteOutputs()
{
    //Set Gates
    for(int i = 0; i < NUM_GATE_OUTS; i++)
    {
        gpio_put(GATE_IO_PINS[i], OUT_GATES[i]);
    }
    //Set LEDs
    for(int i = 0; i < NUM_LEDS; i++)
    {
        bool thisLedState = false;
        switch(OUT_LEDS[i])
        {
            case LEDState::OFF:
                thisLedState = false;
                break;
            case LEDState::ON:
                thisLedState = true;
                break;
            case LEDState::SLOW_BLINK:
                thisLedState = (LEDCycle > 128);
                break;
            case LEDState::FAST_BLINK:
                thisLedState = (LEDCycle%128 > 64);
                break;
        }
        gpio_put(LED_IO_PINS[i], thisLedState);
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