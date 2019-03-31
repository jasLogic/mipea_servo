/*
 * servo.c
 * Copyright (C) 2018  jasLogic
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "servo.h"

#include <stdlib.h>

#include <mipea/gpio.h>
#include <mipea/pwm.h>
#include <mipea/dma.h>

int
servo_init(void)
{
    servos = (struct servos) {0};

    if (gpio_map() == NULL || pwm_map() == NULL || dma_map() == NULL) {
        return -1;
    }

    // ***** PWM ******
	pwm_channel_config_t config = {
		PWM_CHANNEL0,
		{{
            PWM_CTL_MODE_PWM,
		    PWM_RPTL_STOP,
		    PWM_SBIT_LOW,
		    PWM_POLA_DEFAULT,
		    PWM_USEF_FIFO,
		    PWM_MSEN_PWMALGORITHM
        }},
		50,
		10
	};
	pwm_configure(&config);
	PWM->DMAC = (1 << 31) | (15 << 8) | (15 << 0); // no idea what threshhold is
	pwm_enable(PWM_CHANNEL0);

    gpio_masks_data = (dma_phy_mem_blk_t) {0};
    cbs_data = (dma_phy_mem_blk_t) {0};

    return 1;
}

void
servo_uninit(void)
{
    free(servos.servo);

    // stop dma
    DMA_14->CS = 1 << 31; // reset signal + all other bits reset
    // stop pwm
    pwm_disable(PWM_CHANNEL0);

    dma_unmap();
    pwm_unmap();
    gpio_unmap();
}

void
servo_update_cb(void)
{
    DMA_14->CS = 1 << 31; // reset signal + all other bits reset

    if (gpio_masks_data.mem != NULL) {
        dma_free_phy_mem(&gpio_masks_data); // free previous allocs
    }
    if (cbs_data.mem != NULL) {
        dma_free_phy_mem(&cbs_data);
    }

    unsigned int num_samples = 20000;
    unsigned int num_cbs = num_samples + 2 * servos.length;
    // two times servos.length because one for turning the pin on and another off

    dma_alloc_phy_mem(&gpio_masks_data, servos.length * sizeof(uint32_t));
    uint32_t *gpio_masks = (uint32_t *)gpio_masks_data.mem;
    for (unsigned int i = 0; i < servos.length; ++i) {
        gpio_masks[i] = 1 << servos.servo[i].pin;
    }

    dma_alloc_phy_mem(&cbs_data, num_cbs * sizeof(dma_cb_t));
    dma_cb_t *cbs = (dma_cb_t *)cbs_data.mem;

    unsigned int cbi = 0;
    for (unsigned int i = 0; i < num_samples; ++i) {
        for (unsigned int j = 0; j < servos.length; ++j) {
            if (i == servos.servo[j].start) {
                // block that sets a pin
                cbs[cbi].TI = (1 << 26) | (5 << 16) | (1 << 6); // wait response??
    			cbs[cbi].SOURCE_AD = dma_virt_to_phy(&gpio_masks_data, &gpio_masks[j]);
    			cbs[cbi].DEST_AD = PERIPHERAL_BASE_PHY + GPIO_OFFSET + 0x1c; // set
    			cbs[cbi].TXFR_LEN = sizeof(uint32_t);
    			cbs[cbi].STRIDE = 0;
    			cbs[cbi].NEXTCONBK = dma_virt_to_phy(&cbs_data, &cbs[++cbi]);
            }
            if (i == servos.servo[j].stop) {
                // block that clrs a pin
                cbs[cbi].TI = (1 << 26) | (5 << 16) | (1 << 6); // wait response??
    			cbs[cbi].SOURCE_AD = dma_virt_to_phy(&gpio_masks_data, &gpio_masks[j]);
    			cbs[cbi].DEST_AD = PERIPHERAL_BASE_PHY + GPIO_OFFSET + 0x28; // clr
    			cbs[cbi].TXFR_LEN = sizeof(uint32_t);
    			cbs[cbi].STRIDE = 0;
    			cbs[cbi].NEXTCONBK = dma_virt_to_phy(&cbs_data, &cbs[++cbi]);
            }
        }

        // normal time keeping block
        cbs[cbi].TI = (1 << 26) | (5 << 16) | (1 << 6); // wait response??
        cbs[cbi].SOURCE_AD = dma_virt_to_phy(&gpio_masks_data, &gpio_masks[0]); // random data
        cbs[cbi].DEST_AD = PERIPHERAL_BASE_PHY + PWM_OFFSET + 0x18; // fifo
        cbs[cbi].TXFR_LEN = sizeof(uint32_t);
        cbs[cbi].STRIDE = 0;
        cbs[cbi].NEXTCONBK = dma_virt_to_phy(&cbs_data, &cbs[++cbi]);
    }
    // last block needs to be connected to the first one
    cbs[cbi - 1].NEXTCONBK = dma_virt_to_phy(&cbs_data, &cbs[0]);

    DMA_14->CS |= /*(1 << 31)*/(1 << 29) | (1 << 1); // disable debug and clear end flag
	DMA_14->CONBLK_AD = dma_virt_to_phy(&cbs_data, cbs);
    dma_enable(DMA_14);
}

int
servo_add(uint32_t pin)
{
    if (servos.servo == NULL) {
        servos.servo = malloc(sizeof(struct servo));
        if (servos.servo == NULL) {
            return -1;
        }
    } else {
        servos.servo = realloc(servos.servo, (servos.length+1) * sizeof(struct servo));
        if (servos.servo == NULL) {
            return -1;
        }

    }

    servos.servo[servos.length].pin = pin;
    gpio_out(pin);

    return ++servos.length;
}

int
servo_remove(uint32_t pin)
{
    for (unsigned int i = 0; i < servos.length; ++i) {
        if (servos.servo[i].pin == pin) {
            for (unsigned int j = i; j < servos.length - 1; ++j) {
                servos.servo[j] = servos.servo[j + 1];
            }
            servos.servo = realloc(servos.servo, (servos.length-1) * sizeof(struct servo));
            if (servos.servo == NULL) {
                return -1;
            }
            return --servos.length;
        }
    }
    return -1; // servo not found
}

int
servo_set(uint32_t pin, unsigned int pulsewidth)
{
    for (unsigned int i = 0; i < servos.length; ++i) {
        if (servos.servo[i].pin == pin) {
            servos.servo[i].start = 0;
            servos.servo[i].stop = pulsewidth;

            servo_update_cb();

            return pin;
        }
    }
    return -1; // servo not found
}
