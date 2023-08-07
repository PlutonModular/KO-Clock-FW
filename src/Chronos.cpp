/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "Chronos.hpp"


void Chronos::Init(IOHelper *ioh)
{
	io = ioh;
}

bool Chronos::CalcGate(uint32_t thisBeatTime, uint16_t divisor, uint16_t gateLen)
{
    //TODO: change gate generation method to something with better hysterisis prevention
    return (thisBeatTime%divisor)*1024 < divisor*gateLen;
}

void Chronos::FastUpdate(uint32_t deltaMicros)
{
    if(isFollowMode)
    {

    }
    else if(isPlayMode)
    {
        //stop if set to 0BPM, or else it's actually 0.00767988281 BPM, which might spook someone in 2.17 hours!!
        if(microsPerTimeGradation != UINT16_MAX)
        {
            timeInThisGradation += deltaMicros;
            if(timeInThisGradation > microsPerTimeGradation)
            {
                timeInThisGradation -= microsPerTimeGradation;
                if      (io->IN_TMULT_SWITCH == 0) beatTime += 1;
                else if (io->IN_TMULT_SWITCH == 1) beatTime += 2;
                else if (io->IN_TMULT_SWITCH == 2) beatTime += 4;
            }
        }

        //reset on the beat
        if((CalcGate(64, gateLen) && !CalcGate(last_beatTime, 64, gateLen)) && io->ProcessResetFlag())
        {
            beatTime = 0;
        }

        //TEMPORARY IMPLEMENTATION FOR TESTING, PROBABLY VERY BAD
        io->OUT_GATES[0] = CalcGate(512, gateLen); //512 512th notes
        io->OUT_GATES[1] = CalcGate(256, gateLen);
        io->OUT_GATES[2] = CalcGate(128, gateLen);
        io->OUT_GATES[3] = CalcGate(64 , gateLen);

    }
    else
    {
        beatTime = 0;
        io->OUT_GATES[0] = false;
        io->OUT_GATES[1] = false;
        io->OUT_GATES[2] = false;
        io->OUT_GATES[3] = false;
    }
    last_beatTime = beatTime;
}

//We sacrifice a little bit of accuracy here in exchange for faster calculation and better precision.
void Chronos::SetBPM(float exactBPM)
{
    if(exactBPM == currentExactBPM) return;
    currentExactBPM = exactBPM;
    printf("CALCULATING VALUES FOR BPM: %f\n", exactBPM);
    if(exactBPM == 0)
    {
        microsPerTimeGradation = UINT16_MAX; //0BPM
        return;
    }
    float b = exactBPM/60.0f;
    printf("\tBPS: %f\n", b);
    b = 1/b;
    printf("\tSPB: %f\n", b);
    b *= 1'000'000.0f;
    printf("\tmicros Per beat: %f\n", b);
    b /= 128.0f; //convert from uS per 4th note to uS per 512th note
    printf("\tmicros Per 512th: %f\n", b);

    microsPerTimeGradation = uint32_t(floorf(b));
    printf("\tFINAL VALUE: %u\n", microsPerTimeGradation);
    printf("\tREAL QN TIME: %u\n", microsPerTimeGradation*64);
}

void Chronos::SlowUpdate(uint32_t deltaMicros)
{


	if(io->ProcessPlayFlag())
    {
        isPlayMode = !isPlayMode;
    }

    if(isFollowMode)
    {
        bool isClockLEDOn = beatTime % 64 < 32;
        io->SetLEDState(PanelLED::PlayButton, isClockLEDOn?LEDState::SOLID_ON:LEDState::SOLID_HALF);
        io->SetLEDState(PanelLED::Reset, io->FLAG_RST?LEDState::FADE_FASTEST:LEDState::SOLID_OFF);
    }
    else if(isPlayMode)
    {
        // -------- Set BPM From Knob --------
        float newBPM = io->IN_BPM_KNOB; //0-4096
        newBPM /= 4096.0f;
        newBPM *= 200.0f; //scale BPM knob to 0-200 BPM
        SetBPM(newBPM);
        // -------- Set LEDs --------
        bool isClockLEDOn = beatTime % 64 < 32;
        io->SetLEDState(PanelLED::PlayButton, isClockLEDOn?LEDState::SOLID_ON:LEDState::SOLID_HALF);
        io->SetLEDState(PanelLED::Clock, LEDState::SOLID_OFF);
        io->SetLEDState(PanelLED::Reset, io->FLAG_RST?LEDState::FADE_FASTEST:LEDState::SOLID_OFF);
    }
    else
    {
        io->SetLEDState(PanelLED::PlayButton, LEDState::FADE_SLOW);
        io->SetLEDState(PanelLED::Reset, io->FLAG_RST?LEDState::FADE_FASTEST:LEDState::SOLID_OFF);
    }
}
