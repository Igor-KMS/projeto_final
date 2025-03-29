#include <stdio.h>
#include "pico/stdlib.h"
#include "buzzer.h"
#include "htimer.h"
#include "display.h"
#include "leds.h"
#include "joystick.h"
#include "buttons.h"
#include "hardware/sync.h"

int main()
{
  stdio_init_all();
  initialize_timer();
  initialize_display();
  initialize_buzzers();
  initialize_leds();
  initialize_joystick();
  initialize_buttons();

  const display_state order[] = {D_WELCOME, D_ALARM_TYPE_SELECT, D_SHORT_TIME_SELECT, D_SET_CURRENT_DATE, D_LONG_WEEK_SELECT, D_LONG_TIME_SELECT, D_QUIET_SELECT};
  int i = 0;
  while (1)
  {
    i += change_display_state(order[i]);

    if (i < 0)
      i = 0;
    else if (i >= sizeof(order))
      break;
  }
  hide_display();
  setup_struct();
  toggle_irq(BUTTON_A, false);
  toggle_irq(BUTTON_B, false);
  schedule_alarm(&current_timer_struct);
  __wfi();
}
