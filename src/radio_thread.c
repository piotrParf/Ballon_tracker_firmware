/*
 * Copyright (c) SP4SWD 2023
 */
#include "../inc/support.h"
#include "drivers/Si5351/si5351.h"
#include "drivers/WSPR.h"

LOG_MODULE_REGISTER(radio);

#define SLEEP_TIME_MS 8900
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
  int ret;

  LOG_INF("Radio starting");

  radio_poweron();

  ret = SI5351_init();
  if (ret) {
    LOG_INF("SI5351_init failed: %d\n", ret);
    return;
  }

  SI5351_frequency(SI5351_FREQUENCY);

  /* thread while loop */
  while (1) {

    /* CONSTRUCT WSPR EXTENDED MESSAGE */
    WSPR_encode_msg_extended(10.0f, 10.0f, 1000, WSPR_CALLSIGN);
    WSPR_create_tones();

    /* transmit */
    WSPR_transmit();

    k_msleep(SLEEP_TIME_MS);
    k_yield();
  }
}
