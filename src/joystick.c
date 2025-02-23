#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "joystick.h"

void initialize_joystick(){
  adc_init();
  adc_gpio_init(JOYSTICK_Y);

  gpio_init(JOYSTICK_SW);
  gpio_pull_up(JOYSTICK_SW);
  gpio_set_dir(JOYSTICK_SW, GPIO_IN);
  return;
}
