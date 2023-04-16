/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "Chronos.hpp"

bool isPlayMode = false;

void Chronos::Init(IOHelper *ioh)
{
	io = ioh;
}

void Chronos::FastUpdate(uint32_t deltaMicros)
{
	
}

void Chronos::SetBPM(float exactBPM)
{
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
    b /= 64.0f; //convert from uS per 4th note to uS per 256th note
    printf("\tmicros Per 256th: %f\n", b);

    microsPerTimeGradation = uint32_t(floorf(b));
    printf("\tFINAL VALUE: %u\n", microsPerTimeGradation);
    printf("\tREAL QN TIME: %u\n", microsPerTimeGradation*64);
}

void Chronos::SlowUpdate(uint32_t deltaMicros)
{
	if(io->FLAG_PLAY)
    {
        io->FLAG_PLAY = false;
        isPlayMode = !isPlayMode;
        SetBPM(60);
        SetBPM(90);
        SetBPM(99.99f);
        SetBPM(120);
        SetBPM(240);
        SetBPM(1000);
    }

    io->SetLEDState(PanelLED::PlayButton, isPlayMode?LEDState::FADE_FAST:LEDState::FADE_MED);
}
