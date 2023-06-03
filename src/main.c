/*
 * Copyright (c) Piotr Parfeniuk 2023
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/usb/usb_device.h>

LOG_MODULE_REGISTER(baloon);

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* shell */
const struct shell *sh;

void main(void) {
  sh = shell_backend_uart_get_ptr();

  shell_print(sh, "Tracker start");
  LOG_INF("Tracker start");

  /* main while loop */
  while (1) {
    k_msleep(SLEEP_TIME_MS);
  }
}

