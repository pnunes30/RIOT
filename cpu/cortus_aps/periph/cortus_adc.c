/* * OSS-7 - An opensource implementation of the DASH7 Alliance Protocol for ultra
 * lowpower wireless sensor communication
 *
 * Copyright 2019 CORTUS
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cpu.h"
#include "mutex.h"
#include "periph/adc.h"
#include "machine/adc.h"

#define ADC_MAX_SAMPLING_RATE 1000   // 1 MSPS
#define ADC_MIN_SAMPLING_RATE   20   // 20 kSPS

/**
 * @brief	protection of the ADC device by a mutex
 *
 * Cortus_aps has a single ADC device
 *
 */
static mutex_t mutex_adc = MUTEX_INIT;


int adc_init (adc_t line)
{

	/* make sure the given line is valid */
	/* Adc device has only one line so far */
    if (line != ADC_NUMOF) {
        return -1;
    }

    /* Lock the adc device */
    mutex_lock(&mutex_adc);

    /* Initialise ADC clock */
    adc->clk_div = 0x5; // Default value : 10  (0101 in datasheet)

    /* Activate ADC */
    adc->enable = 1;

    /* Single conversion Mode only */
    adc->conv_md = 0;

    /*
     * To reduce current consumption it is recommended to use an option
     * COMP_MD = “0” for low sample rates (less than 250kSPS) or for a single
     * conversion at different sample rates
     */
    adc->comp_md = 0; // after conversion, comparator is in a sleep mode with no current consumption

    /* Enable reference signal buffer */
    adc->buf_en = 1;

    /* Choose required reference voltage signal input */
    /* internal reference buffer with high power without external capacitors */
    adc->buf_md = 1;

    /* Release adc device for now */
    mutex_unlock(&mutex_adc);

    return 0;
}

int adc_sample (adc_t line, adc_res_t res)
{
	uint32_t sample;

    /* check if resolution is applicable */
    if (res != 1) {
        return -1;
    }

	/* make sure the given line is valid */
	/* Adc device has only on line so far */
    if (line != ADC_NUMOF) {
        return -1;
    }

    /* Lock the adc device */
    mutex_lock(&mutex_adc);

    /* Start conversion and wait for results */
    adc->conv_exe = 1;
    while ( adc->status == 0 ){}
    /* Read result */
    sample = (int)adc->data;

    /* Release adc device */
    mutex_unlock(&mutex_adc);

    return sample;
}
