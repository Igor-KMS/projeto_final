#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "buzzer.h"
#include "hardware/clocks.h"
#include "htimer.h"
#include <time.h>

#define BUZZER_A 21
#define BUZZER_B 10

const uint16_t MAX_WRAP = 4096;

void toggle_pwm(uint8_t on)
{
  unsigned int slice1 = pwm_gpio_to_slice_num(BUZZER_A);
  unsigned int slice2 = pwm_gpio_to_slice_num(BUZZER_B);
  pwm_set_enabled(slice1, on);
  pwm_set_enabled(slice2, on);
  return;
}

void set_frequency(uint8_t BUZZER, uint32_t FREQ)
{
  uint32_t divider = clock_get_hz(clk_sys) / (FREQ * (MAX_WRAP + 1));
  uint slice = pwm_gpio_to_slice_num(BUZZER);
  pwm_set_clkdiv(slice, divider);
  return;
}

void play_note(uint8_t BUZZER, uint32_t DURATION)
{
  pwm_set_gpio_level(BUZZER, 2000);
  busy_wait_us_32(DURATION);
  pwm_set_gpio_level(BUZZER, 0);
}

void play_alarm()
{
  toggle_pwm(true);

  play_note(BUZZER_A, 200000);
  busy_wait_us_32(100000);
  play_note(BUZZER_B, 200000);
  busy_wait_us_32(100000);

  toggle_pwm(false);

  return;
}

static void setup_pwm(uint8_t BUZZER, uint32_t FREQ)
{
  gpio_set_function(BUZZER, GPIO_FUNC_PWM);
  uint slice = pwm_gpio_to_slice_num(BUZZER);
  set_frequency(BUZZER, FREQ);
  pwm_set_wrap(slice, MAX_WRAP);
  pwm_set_enabled(slice, true);
  pwm_set_gpio_level(BUZZER, 0);
  return;
}

void initialize_buzzers()
{
  setup_pwm(BUZZER_A, 520);
  setup_pwm(BUZZER_B, 520);
  return;
}
