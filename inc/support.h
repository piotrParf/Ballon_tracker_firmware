/*
 *  Copyright (c) Piotr Parfeniuk 2023
 */
#ifndef _SUPPORT_H_
#define _SUPPORT_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/usb/usb_device.h>

void radio_thread(void);
bool radio_poweron(void);

#endif /* _SUPPORT_H_ */
