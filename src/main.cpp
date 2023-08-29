/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "ResetFromBoot.hpp"

#include "Chronos.hpp"
#include "IO/IOHelper.hpp"

repeating_timer_t *audioRateTimer;

uint64_t frameLastMicros = 0;
uint64_t frameStartMicros = 0;
uint64_t deltaMicros = 0;

Chronos chronos;
IOHelper io;

void update()
{
    //--------Read CV Rate Inputs--------
    io.ReadSlowInputs(deltaMicros);
    //printf("%d\n", io.CV_scrub );

    //--------Call Helper Classes' CV Rate Updates--------
    chronos.SlowUpdate(deltaMicros);

    //--------Write CV Rate Outputs--------
    io.WriteSlowOutputs(deltaMicros);
    
    //--------Blink Heartbeat LED to confirm not frozen--------
    gpio_put(PICO_DEFAULT_LED_PIN, ((frameStartMicros/1'000) % 1'000) < 500);
}

uint64_t fastLastMicros = 0;
bool audio_rate_callback(struct repeating_timer *t)
{
    uint64_t now = time_us_64();
    uint64_t dt = now - fastLastMicros;
    //--------Call Helper Classes' Audio Rate Updates--------
    io.ReadFastInputs(dt);
    chronos.FastUpdate(dt);
    io.WriteFastOutputs(dt);
    fastLastMicros = now;
    return true; //keep doing this
}

int main(void)
{
    //--------Initialize Clock and StdIO--------
    stdio_init_all();
    busy_wait_ms(200);
    set_sys_clock_khz(280000, true);
    
    //--------Initialize Helper Classes--------
    io.Init();      //general I/O helper
    chronos.Init(&io); //timing handler
    chronos.SetBPM(165);

    //start chronos
    alarm_pool *chronos_pool = alarm_pool_create(1, 1);
    audioRateTimer = new repeating_timer_t();
    alarm_pool_add_repeating_timer_us(chronos_pool, -40, audio_rate_callback, NULL, audioRateTimer); //25kHz (40uS interval)
    
    io.SetLEDState(PanelLED::PlayButton, LEDState::BLINK_SLOW);
            
    while (true)
    {
        //--------Do Timing Variable Updates--------
        frameLastMicros = frameStartMicros;
        frameStartMicros = time_us_64();
        deltaMicros = frameStartMicros - frameLastMicros;

        //--------Run non-time-critical tasks--------
        update();

        //check if boot button is held, and enter boot mode if so
        check_for_reset();

        sleep_us(1000);
    }
}