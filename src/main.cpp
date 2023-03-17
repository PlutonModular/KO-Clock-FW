/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "ResetFromBoot.h"

#include "chronos.h"

repeating_timer_t *audioRateTimer;


uint64_t frameLastMicros = 0;
uint64_t frameStartMicros = 0;
uint64_t deltaMicros = 0;

Chronos chronos;

int main(void)
{  
    stdio_init_all();
    sleep_ms(1000);
    set_sys_clock_khz(210000, true);
    printf("\n\nHello World\n");
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    chronos.Init();

    //50kHz
    add_repeating_timer_us(-20, audio_rate_callback, NULL, audioRateTimer);
    
    while (true)
    {
        //Do timing updates
        frameLastMicros = frameStartMicros;
        frameStartMicros = time_us_64();
        deltaMicros = frameStartMicros - frameLastMicros;

        //Run non-time-critical tasks
        update();
        //check if boot button is held, and enter boot mode if so
        check_for_reset();
    }
}

void update()
{
    //Blink LED to confirm not frozen
    gpio_put(PICO_DEFAULT_LED_PIN, (frameStartMicros/1'000'000) % 1'000'000 < 500'000);
}


bool audio_rate_callback(struct repeating_timer *t)
{
    chronos.FastUpdate(20);
    return true; //keep doing this.
}