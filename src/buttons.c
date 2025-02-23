#include <stdio.h>
#include "pico/stdlib.h"
#include "display.h"
#include "buttons.h"
#include "htimer.h"

inline void toggle_irq(uint8_t gpio, bool on)
{
  gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_FALL, on);
}

void send_clicked_display(uint gpio)
{
  if (on_loop || alarm_triggered)
  {
    if (gpio == 5)
      prev_triggered = true;
    else if (gpio == 6)
      next_triggered = true;
  }
  return;
}

static void debounce(uint gpio)
{
  toggle_irq(gpio, false);
  busy_wait_us_32(50000);
  if (!gpio_get(gpio))
  {
    send_clicked_display(gpio);
  }
  toggle_irq(gpio, true);
}

void gpio_irq_callback(uint gpio, uint32_t event_mask)
{
  debounce(gpio);
  return;
}

static void setup_button(uint8_t BUTTON)
{
  gpio_init(BUTTON);
  gpio_set_dir(BUTTON, GPIO_IN);
  gpio_pull_up(BUTTON);
  gpio_set_irq_enabled_with_callback(BUTTON, GPIO_IRQ_EDGE_FALL, true, gpio_irq_callback);
  return;
}

void initialize_buttons()
{
  setup_button(BUTTON_A);
  setup_button(BUTTON_B);
  return;
}
