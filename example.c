/*
 * servo.h
 * Copyright (C) 2019  jasLogic
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

#include <stdio.h>
#include <unistd.h>

#include "src/servo.h"

int
main(void)
{
    servo_init();

    servo_add(14);

    servo_set(14, 1000);
    servo_update_cbs();
    sleep(1);
    
    servo_set(14, 2000);
    servo_update_cbs();
    sleep(2);

    servo_uninit();
    return 0;
}
