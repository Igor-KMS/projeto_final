#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "htimer.h"
#include "display.h"
#include "joystick.h"
#include "buzzer.h"
#include "leds.h"
#include "buttons.h"

#define TIMER_ID 0

uint64_t microsseconds;
int64_t remaining_iterations = 0;

volatile bool alarm_triggered = false;

inline void schedule_timer()
{
  printf("%lld", (remaining_iterations > 0 ? UINT32_MAX : (uint32_t)microsseconds));
  uint32_t target = timer_hw->timerawl + (uint32_t)((remaining_iterations > 0) ? UINT32_MAX : microsseconds);
  timer_hw->alarm[TIMER_ID] = target;
}

void trigger_alarm()
{
  alarm_triggered = true;
  uint32_t current_time = to_ms_since_boot(get_absolute_time());

  while (1)
  {
    if (!current_timer_struct.is_quiet && to_ms_since_boot(get_absolute_time()) - current_time <= 30000)
      play_alarm();
    trigger_visuals();
    if (!gpio_get(BUTTON_A) || !gpio_get(BUTTON_B) || !gpio_get(JOYSTICK_SW))
    {
      next_triggered = false;
      prev_triggered = false;
      alarm_triggered = false;
      clear_visuals();
      return;
    };
    busy_wait_us_32(50000);
  }
}

void timer_irq_handler()
{
  hw_clear_bits(&timer_hw->intr, 1u << TIMER_ID);

  remaining_iterations--;

  if (remaining_iterations > 0)
    return schedule_timer();
  else
    return trigger_alarm();
}

static uint64_t to_microsseconds(datetime_t ts, bool is_this_week)
{
  uint64_t microseconds = 0;

  microseconds += (uint64_t)(is_this_week ? ts.day : (ts.day + 7)) * 86400 * 1000000LL;
  microseconds += (uint64_t)ts.hour * 3600 * 1000000LL;
  microseconds += (uint64_t)ts.min * 60 * 1000000LL;
  microseconds += (uint64_t)ts.sec * 1000000LL;

  return microseconds;
}

void schedule_alarm_short(alarm_struct *alarm)
{
  uint64_t selected_microsseconds = to_microsseconds(alarm->selected_date, true);
  remaining_iterations = selected_microsseconds / UINT32_MAX;
  microsseconds = selected_microsseconds - (remaining_iterations * (uint64_t)UINT32_MAX);
  if(microsseconds <= 0) microsseconds = 1000000;
  schedule_timer();
}

void schedule_alarm_long(alarm_struct *alarm)
{
  uint64_t current_microsseconds = to_microsseconds(alarm->current_date, true);
  uint64_t selected_microsseconds = to_microsseconds(alarm->selected_date, alarm->is_this_week);

  uint64_t diff = selected_microsseconds - current_microsseconds;

  if (diff <= 0LL)
    diff = to_microsseconds(alarm->selected_date, false) - current_microsseconds;

  remaining_iterations = diff / UINT32_MAX;
  microsseconds = diff - (remaining_iterations * (uint64_t)UINT32_MAX);

  schedule_timer();
}

void initialize_timer()
{
  hw_set_bits(&timer_hw->inte, 1u << TIMER_ID);
  irq_set_exclusive_handler(TIMER_IRQ_0, timer_irq_handler);
  irq_set_enabled(TIMER_IRQ_0, true);
  return;
}
