#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/rtc.h"
#include "htimer.h"
#include "display.h"
#include "joystick.h"
#include "buzzer.h"
#include "leds.h"
#include "buttons.h"

#define TIMER_ID 0

volatile bool alarm_triggered = false;

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
    busy_wait_us_32(10000);
  }
}

void _rtc_irq_handler()
{
  rtc_disable_alarm();
  trigger_alarm();
}

static uint64_t to_microsseconds(datetime_t ts)
{
  uint64_t microseconds = 0;

  microseconds += (uint64_t)ts.day * 86400 * 1000000LL;
  microseconds += (uint64_t)ts.hour * 3600 * 1000000LL;
  microseconds += (uint64_t)ts.min * 60 * 1000000LL;
  microseconds += (uint64_t)ts.sec * 1000000LL;

  return microseconds;
}

void schedule_alarm(alarm_struct *alarm)
{
  if (!rtc_set_datetime(&alarm->current_date))
    return;
  rtc_disable_alarm();
  datetime_t t = alarm->selected_date;
  if (!alarm->is_short && to_microsseconds(alarm->current_date) >= to_microsseconds(alarm->selected_date))
    t.day += 7;

  rtc_hw->irq_setup_0 = ((((uint32_t)t.day) << RTC_IRQ_SETUP_0_DAY_LSB));
  rtc_hw->irq_setup_1 = ((((uint32_t)t.hour) << RTC_IRQ_SETUP_1_HOUR_LSB)) |
                        ((((uint32_t)t.min) << RTC_IRQ_SETUP_1_MIN_LSB)) |
                        ((((uint32_t)t.sec) << RTC_IRQ_SETUP_1_SEC_LSB));

  if (t.day >= 0)
    hw_set_bits(&rtc_hw->irq_setup_0, RTC_IRQ_SETUP_0_DAY_ENA_BITS);
  if (t.hour >= 0)
    hw_set_bits(&rtc_hw->irq_setup_1, RTC_IRQ_SETUP_1_HOUR_ENA_BITS);
  if (t.min >= 0)
    hw_set_bits(&rtc_hw->irq_setup_1, RTC_IRQ_SETUP_1_MIN_ENA_BITS);
  if (t.sec >= 0)
    hw_set_bits(&rtc_hw->irq_setup_1, RTC_IRQ_SETUP_1_SEC_ENA_BITS);

  irq_set_exclusive_handler(RTC_IRQ, _rtc_irq_handler);
  rtc_hw->inte = RTC_INTE_RTC_BITS;
  irq_set_enabled(RTC_IRQ, true);
  rtc_enable_alarm();
}

void initialize_timer()
{
  rtc_init();
  return;
}
