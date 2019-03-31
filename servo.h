/*
 * servo.h
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

#ifndef _SERVO_H_
#define _SERVO_H_

#include <stdint.h>
#include <mipea/dma.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

struct servo {
    int pin;
    unsigned int start; // in us
    unsigned int stop; // in us
};

struct servos {
    struct servo *servo;
    unsigned int length;
} servos;

dma_phy_mem_blk_t gpio_masks_data;
dma_phy_mem_blk_t cbs_data;

int     servo_init(void);
void    servo_uninit(void);

void    servo_update_cb(void);

int servo_add(uint32_t pin);
int servo_remove(uint32_t pin);

int servo_set(uint32_t pin, unsigned int pulsewidth); // in us

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//_SERVO_H_
