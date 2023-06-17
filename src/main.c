/*
 * Copyright (c) SP4SWD 2023
 */

#include "../inc/support.h"

LOG_MODULE_REGISTER(baloon);

#define RADIO_THREAD_STACK_SIZE (2048)
#define RADIO_THREAD_PRIORITY (7)
K_THREAD_STACK_DEFINE(radio_thread_stack, RADIO_THREAD_STACK_SIZE);
struct k_thread radio_thread_data;
K_THREAD_DEFINE(radio_thread_id, RADIO_THREAD_STACK_SIZE, radio_thread, NULL,
                NULL, NULL, RADIO_THREAD_PRIORITY, 0, 0);

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS (1000)
#define LED0_NODE DT_ALIAS(myled0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* shell */
const struct shell *sh;

void main(void) {
  int ret;
  LOG_INF("Tracker start");

  radio_poweron();

  sh = shell_backend_uart_get_ptr();
  if (sh == NULL) {
    LOG_INF("Shell on usb not ready");
  }

  if (!gpio_is_ready_dt(&led)) {
    LOG_INF("LED0 not ready");
    return;
  }

  ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    LOG_INF("Can`t configure LED0");
    return;
  }

  usb_enable(NULL);

  while (1) {
    k_msleep(SLEEP_TIME_MS);

    ret = gpio_pin_toggle_dt(&led);
    if (ret < 0) {
      return;
    }
  }
}
