/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "Chronos.hpp"


void Chronos::Init(IOHelper *ioh)
{
	io = ioh;
    
    for(int i = 0; i < CLOCKIN_BUFFER_SIZE; i++)
    {
        clockInDiffBuffer[i] = 0;
    }
}

bool Chronos::CalcGate(uint32_t thisBeatTime, uint16_t divisor, uint16_t gateLen)
{
    //TODO: change gate generation method to something with better hysterisis prevention
    return (thisBeatTime%divisor)*1024 < divisor*gateLen;
}

void Chronos::AddBeatToBPMEstimate()
{
    //add the length of this pulse to the pulse length buffer
    uint64_t currentTimeUS = time_us_64();
    clockInDiffBuffer[clockInDiffBufferIterator] = currentTimeUS - lastClockTime;
    lastClockTime = currentTimeUS;
    
    //increase clockInDiffBufferIterator by one
    clockInDiffBufferIterator++;
    if(clockInDiffBufferIterator >= CLOCKIN_BUFFER_SIZE) clockInDiffBufferIterator = 0;

    //average clockInDiffBuffer time differences and convert to seconds
    int elementsAveraged = 0;
    float average = 0;
    for(int i = 0; i < CLOCKIN_BUFFER_SIZE; i++)
    {
        if(clockInDiffBuffer[i] == 0) continue;
        average += clockInDiffBuffer[i]/1'000'000.0f; //seconds
        elementsAveraged++;
    }
    average /= (double)elementsAveraged;
	debug("Average:\t%f\n", average);
    //Convert to BPM;
    estimatedBPM = (1.0f/average) * (60.0f/float(clockPPQN))*0.9999f; //better to be slightly under than over to help prevent double-triggering or weirdness
    debug("Estimated BPM:\t%f\n", estimatedBPM);
}
void Chronos::AddBeatToBPMEstimateLastOnly()
{
    //add the length of this pulse to the pulse length buffer
    uint64_t currentTimeUS = time_us_64();
    lastClockTime = currentTimeUS;
}

void Chronos::CalculateSwing()
{
    uint16_t swing = io->IN_SWING_KNOB;
    if(io->CV_swing > 300) //using swing cv
    {
        swing += uint16_t(io->CV_swing); //later clamped to 4096
        if(swing > 4096)
        {
            swing = 4096;
        }
    }
    debug("%i\n", io->CV_UD);
    if(swing > 300) //don't burden the processor with this while swing isn't even on
    {
        uint32_t barTime = (beatTimeFinal%512)*128; //bar time from 0-65535
        //this swing formula was calculated experimentally using the following formula on desmos. N = swings per bar, X and Y range from 0-65535
        //y=x\ +\ \cos\left(\frac{x}{\left(\frac{65535}{2\pi\ \cdot\ n}\right)}\right)\cdot\left(\frac{9300}{n}\right)-\left(\frac{9300}{n}\right)
        uint32_t swingTime = beatTimeFinal + (cos(barTime / (65535/(2*3.141592*swingsPerBar)))*(9300/swingsPerBar) - (9300/swingsPerBar))/128;
        beatTimeFinal = lerp((double)beatTimeFinal, (double)swingTime, (double)swing/4096.0);
    }

    beatTimeFinal += io->CV_scrub;
}

void Chronos::FastUpdate(uint32_t deltaMicros)
{
    if(isFollowMode)
    {
        //Check for clock pulse
        if(io->ProcessClockFlag())
        {
            //Update running BPM estimate and reset ext clock keepalive
            AddBeatToBPMEstimate();
            externalClockKeepaliveCountdown = max(microsPerTimeGradation * CLOCKIN_WAIT_MULT, CLOCKIN_MIN_WAIT);
            clockPulseCounter++;
            isPlayMode = true;
        }

        //Check for reset pulse (after clock, so a full stop doesn't start at an advanced time)
        if(io->ProcessResetFlag())
        {
            clockPulseCounter = 0;
            last_beatTime = 0;
            beatTime = 0;
        }

        if(isPlayMode)
        {
            //Advance time
            timeInThisGradation += deltaMicros;
            if(timeInThisGradation >= microsPerTimeGradation)
            {
                timeInThisGradation -= microsPerTimeGradation;
                if      (io->IN_TMULT_SWITCH == 0) beatTime += 1;
                else if (io->IN_TMULT_SWITCH == 1) beatTime += 2;
                else if (io->IN_TMULT_SWITCH == 2) beatTime += 4;
            }

            //Full quarter note elapsed. snap time to quarter
            if(clockPulseCounter >= uint16_t(clockPPQN))
            {
                clockPulseCounter -= uint16_t(clockPPQN);
                beatTime = floor(round(beatTime/512.0)*512);
            }
            beatTimeFinal = beatTime; //TODO: ADD OFFSET CV HERE
            CalculateSwing();
        }


        //Process the keepalive timer. If it goes below zero, exit follow mode and stop play mode
        externalClockKeepaliveCountdown -= deltaMicros;
        if(externalClockKeepaliveCountdown < 0)
        {
            isFollowMode = false;
            isPlayMode = false; //don't keep running if master clock stops!
        }
    }
    else if(isPlayMode)
    {
        if(io->ProcessClockFlag())
        {
            AddBeatToBPMEstimateLastOnly();
            isFollowMode = true;
            isPlayMode = true;
            externalClockKeepaliveCountdown = max(microsPerTimeGradation * CLOCKIN_WAIT_MULT, CLOCKIN_MIN_WAIT);
        }

        //stop if set to 0BPM, or else it's actually 0.00767988281 BPM, which might spook someone in 2.17 hours!!
        if(microsPerTimeGradation != UINT16_MAX)
        {
            //Advance time
            timeInThisGradation += deltaMicros;
            if(timeInThisGradation >= microsPerTimeGradation)
            {
                timeInThisGradation -= microsPerTimeGradation;
                if      (io->IN_TMULT_SWITCH == 0) beatTime += 1;
                else if (io->IN_TMULT_SWITCH == 1) beatTime += 2;
                else if (io->IN_TMULT_SWITCH == 2) beatTime += 4;
            }
            beatTimeFinal = beatTime; //TODO: ADD OFFSET CV HERE
            CalculateSwing();
        }

        //reset on the beat
        if((CalcGate(64, gateLen) && !CalcGate(last_beatTime, 64, gateLen)) && io->ProcessResetFlag())
        {
            beatTime = 0;
        }

    }
    else
    {
        if(io->ProcessClockFlag())
        {
            AddBeatToBPMEstimateLastOnly();
            isFollowMode = true;
            isPlayMode = true;
            externalClockKeepaliveCountdown = max(microsPerTimeGradation * CLOCKIN_WAIT_MULT, CLOCKIN_MIN_WAIT);
        }
        beatTime = 0;
    }
    
    //TEMPORARY IMPLEMENTATION FOR TESTING, PROBABLY VERY BAD
    io->OUT_GATES[0] = CalcGate(512, gateLen); //512 512th notes
    io->OUT_GATES[1] = CalcGate(256, gateLen);
    io->OUT_GATES[2] = CalcGate(128, gateLen);
    io->OUT_GATES[3] = CalcGate(64 , gateLen);

    io->OUT_GATES[4] = CalcGate(8 <<clamp((7-io->IN_UD_INDEX) - io->CV_UD_Mult, 1, 7), gateLen);
    io->OUT_GATES[5] = CalcGate(16<<clamp((7-io->IN_UD_INDEX) - io->CV_UD_Mult, 1, 7), gateLen);
    last_beatTime = beatTime;
}

//We sacrifice a little bit of accuracy here in exchange for faster calculation and better precision.
void Chronos::SetBPM(float exactBPM)
{
    if(exactBPM == currentExactBPM) return;
    currentExactBPM = exactBPM;
    debug("CALCULATING VALUES FOR BPM: %f\n", exactBPM);
    if(exactBPM == 0)
    {
        microsPerTimeGradation = UINT16_MAX; //0BPM
        return;
    }
    float b = exactBPM/60.0f;
    debug("\tBPS: %f\n", b);
    b = 1/b;
    debug("\tSPB: %f\n", b);
    b *= 1'000'000.0f;
    debug("\tmicros Per beat: %f\n", b);
    b /= 128.0f; //convert from uS per 4th note to uS per 512th note
    debug("\tmicros Per 512th: %f\n", b);

    microsPerTimeGradation = uint32_t(floorf(b));
    debug("\tFINAL VALUE: %u\n", microsPerTimeGradation);
    debug("\tREAL QN TIME: %u\n", microsPerTimeGradation*64);
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
        SetBPM(estimatedBPM);
    }
    else if(isPlayMode)
    {
        // -------- Set BPM From Knob --------
        float newBPM = io->IN_BPM_KNOB; //0-4096
        newBPM /= 4096.0f;
        newBPM *= 200.0f; //scale BPM knob to 0-200 BPM
        float bpmMod = io->CV_timeMult;
        bpmMod /= 2048.0f;
        bpmMod *= 5;
        bpmMod += 1;
        SetBPM(newBPM*bpmMod);
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
