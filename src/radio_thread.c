/*
 * Copyright (c) SP4SWD 2023
 */
#include "../inc/support.h"

LOG_MODULE_REGISTER(radio);

#define SLEEP_TIME_MS 1000
static const struct gpio_dt_spec pwr_synth =
    GPIO_DT_SPEC_GET(DT_ALIAS(pwrsynth), gpios);

bool radio_poweron(void) {
  int ret;
  /* Power pin configuration first */
  if (!gpio_is_ready_dt(&pwr_synth)) {
    LOG_INF("Radio pwr not ready");
    return false;
  }

  ret = gpio_pin_configure_dt(&pwr_synth, GPIO_OUTPUT_INACTIVE);
  if (ret < 0) {
    LOG_INF("Can`t configure radio pwr");
    return false;
  }

  ret = gpio_pin_set_dt(&pwr_synth, 1);
  if (ret < 0) {
    LOG_INF("Can`t turn on radio");
    return false;
  }
  LOG_INF("Radio started");
  k_msleep(10);
  return true;
}

void radio_thread(void) {

  LOG_INF("Radio starting");

  /* thread while loop */
  while (1) {
    k_msleep(SLEEP_TIME_MS);
    k_yield();
  }
}
