#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "display.h"
#include "joystick.h"
#include "htimer.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

#define I2C_SDA 14
#define I2C_SCL 15
#define I2C_PORT i2c1
#define OLED_ADDR 0x3C
#define DEFAULT_SCALE 1

const struct
{
  display_state state;
  const int rules[2];
} rule_set[] = {
    {D_WELCOME, {0, 0}},
    {D_SET_CURRENT_DATE, {2, 7}},
    {D_ALARM_TYPE_SELECT, {2, 0}},
    {D_SHORT_TIME_SELECT, {3, 0}},
    {D_LONG_WEEK_SELECT, {2, 0}},
    {D_LONG_TIME_SELECT, {2, 7}},
    {D_QUIET_SELECT, {2, 0}},
};

ssd1306_t display;
display_state current_display_state;
alarm_struct current_timer_struct;

volatile bool next_triggered = false;
volatile bool prev_triggered = false;

bool on_loop;

int current_hour, current_minute, current_weekday;
int short_hours, short_minutes, short_seconds;
int long_hour, long_minute, long_weekday;

void setup_struct()
{
  if (!current_timer_struct.is_short)
  {
    current_timer_struct.current_date.day = current_weekday;
    current_timer_struct.current_date.hour = current_hour;
    current_timer_struct.current_date.min = current_minute;

    current_timer_struct.selected_date.day = long_weekday;
    current_timer_struct.selected_date.hour = long_hour;
    current_timer_struct.selected_date.min = long_minute;
  }
  else
  {
    current_timer_struct.selected_date.day = 0;
    current_timer_struct.selected_date.hour = short_hours;
    current_timer_struct.selected_date.min = short_minutes;
    current_timer_struct.selected_date.sec = short_seconds;
  }
}

void hide_display()
{
  ssd1306_clear(&display);
  ssd1306_show(&display);
}

void clamp_inclusive(int *value, int lower, int upper)
{
  if (*value > upper)
    *value = upper;
  else if (*value < lower)
    *value = lower;
}

static void format_time_selection(char *buffer, int selection, int *hours, int *minutes, int increase)
{
  switch (selection)
  {
  case 0:
    (*hours) += increase;
    clamp_inclusive(hours, 0, 24);
    snprintf(buffer, 12, "[%02d]:%02d", *hours, *minutes);
    break;
  case 1:
    (*minutes) += increase;
    clamp_inclusive(minutes, 0, 59);
    snprintf(buffer, 12, "%02d:[%02d]", *hours, *minutes);
    break;
  default:
    snprintf(buffer, 12, "%02d:%02d", *hours, *minutes);
  }
}

static void format_chronometer_selection(char *buffer, int selection, int increase)
{
  switch (selection)
  {
  case 0:
    short_hours += increase;
    clamp_inclusive(&short_hours, 0, 24);
    snprintf(buffer, 14, "[%02d]h %02dm %02ds", short_hours, short_minutes, short_seconds);
    break;
  case 1:
    short_minutes += increase;
    clamp_inclusive(&short_minutes, 0, 59);
    snprintf(buffer, 14, "%02dh [%02d]m %02ds", short_hours, short_minutes, short_seconds);
    break;
  case 2:
    short_seconds += increase;
    clamp_inclusive(&short_seconds, 0, 59);
    snprintf(buffer, 14, "%02dh %02dm [%02d]s", short_hours, short_minutes, short_seconds);
    break;
  default:
    snprintf(buffer, 14, "%02dh %02dm %02ds", short_hours, short_minutes, short_seconds);
    break;
  }
}

static void format_weekday_selection(char *buffer, int selection, int selected)
{
  static const char weekdays[] = "D S T Q Q S S";
  if (selection >= 0 && selection < 7)
  {
    strcpy(buffer, weekdays);
    int pos = selection * 2;
    memmove(&buffer[pos + 1], &buffer[pos], 13 - pos);
    buffer[pos] = '[';
    if (selected > 0)
      buffer[pos + 1] = 'X';
    buffer[pos + 2] = ']';
  }
}

static int format_option_selection(char *buffer, int selection, char *option1, char *option2, size_t size, int increase)
{
  switch (selection)
  {
  case 0:
    if (increase > 0)
    {
      snprintf(buffer, size, "[X] - %s", option2);
      return 1;
    }
    snprintf(buffer, size, "[%s] - %s", option1, option2);
    break;
  case 1:
    if (increase > 0)
    {
      snprintf(buffer, size, "%s - [X]", option1);
      return 0;
    }
    snprintf(buffer, size, "%s - [%s]", option1, option2);
    break;
  default:
    snprintf(buffer, size, "%s - %s", option1, option2);
  }
  return -1;
}

inline static void write_to_display(char *to_write, uint32_t x, uint32_t y)
{
  ssd1306_draw_string(&display, x, y, DEFAULT_SCALE, to_write);
}

static void update_navigation(char *A, char *B)
{
  ssd1306_clear_square(&display, 0, 55, 128, 12);
  ssd1306_draw_string(&display, 0, 55, DEFAULT_SCALE, A);
  ssd1306_draw_string(&display, 110, 55, DEFAULT_SCALE, B);
  ssd1306_show(&display);
}

static void show_set_date(int selection, int increase, bool is_current_date)
{
  char time_buffer[6];
  char week_buffer[17] = "D S T Q Q S S";

  int *h = &long_hour;
  int *m = &long_minute;

  if (is_current_date)
  {
    h = &current_hour;
    m = &current_minute;
  }

  ssd1306_clear(&display);

  is_current_date ? write_to_display("Hora ATUAL:", 30, 0) : write_to_display("Hora do Alarme:", 20, 0);
  if (selection < 2)
    format_time_selection(time_buffer, selection, h, m, increase);
  else
    format_time_selection(time_buffer, -1, h, m, increase);

  write_to_display(time_buffer, 35, 12);

  write_to_display("Dia da Semana:", 20, 30);
  if (selection >= 2)
  {
    format_weekday_selection(week_buffer, selection - 2, increase);
    if (increase > 0)
    {
      if (is_current_date)
        current_weekday = selection - 2;
      else
        long_weekday = selection - 2;
      clamp_inclusive(&current_weekday, 0, 6);
      clamp_inclusive(&long_weekday, 0, 6);
    }
  }

  write_to_display(week_buffer, 22, 42);
  ssd1306_show(&display);
}

static void show_welcome()
{
  ssd1306_clear(&display);
  write_to_display("Ola!", 55, 25);
  write_to_display("- B para Iniciar -", 10, 35);
  ssd1306_show(&display);
  return;
}

static void show_alarm_select(int selection, int increase)
{
  char buffer[16];
  ssd1306_clear(&display);
  write_to_display("Tipo de alarme: ", 20, 20);
  int selection_value = format_option_selection(buffer, selection, "Curto", "Longo", sizeof(buffer), increase);
  write_to_display(buffer, 20, 32);
  ssd1306_show(&display);
  if (selection_value != -1)
    current_timer_struct.is_short = selection_value;
  return;
}

static void show_setup_short(int selection, int increase)
{
  char chrono_buffer[15];
  ssd1306_clear(&display);
  write_to_display("Cronometro:", 30, 20);
  format_chronometer_selection(chrono_buffer, selection, increase);
  write_to_display(chrono_buffer, 20, 32);
  ssd1306_show(&display);
  return;
}

static void show_select_week(int selection, int increase)
{
  char buffer[17];
  ssd1306_clear(&display);
  write_to_display("Qual semana:", 20, 20);
  int selection_value = format_option_selection(buffer, selection, "Esta", "Proxima", sizeof(buffer), increase);
  write_to_display(buffer, 20, 32);
  ssd1306_show(&display);
  if (selection_value != -1)
    current_timer_struct.is_this_week = selection_value;
  return;
}

static void show_quiet_select(int selection, int increase)
{
  char buffer[12];
  ssd1306_clear(&display);
  write_to_display("Silencioso?", 20, 20);
  int selection_value = format_option_selection(buffer, selection, "Sim", "Nao", sizeof(buffer), increase);
  write_to_display(buffer, 20, 32);
  ssd1306_show(&display);
  if (selection_value != -1)
    current_timer_struct.is_quiet = selection_value;
  return;
}

static void update_by_rule(const int *rules, int current_selection)
{
  if (rules[1] != 0 && current_selection == rules[0] - 1 && current_display_state != D_WELCOME)
    update_navigation("< A", "B v");

  else if (current_selection == rules[0] && current_display_state != D_WELCOME)
    update_navigation("^ A", "B >");

  else if (current_display_state != D_WELCOME)
    update_navigation("< A", "B >");
}

static bool check_triggered(const int *rules, int *current_selection)
{
  if (next_triggered)
  {
    next_triggered = false;
    prev_triggered = false;
    if ((*current_selection) >= (rules[0] + rules[1]) - 1)
      return false;
    (*current_selection)++;
  }
  else if (prev_triggered)
  {
    next_triggered = false;
    prev_triggered = false;
    if (*current_selection == 0)
      return false;
    (*current_selection)--;
  }
  return true;
}

static void read_joystick(int *increase)
{
  adc_select_input(JOYSTICK_ADC_CHANNEL);
  uint16_t value = adc_read();
  if (value > 3000 || !gpio_get(JOYSTICK_SW))
    (*increase)++;
  else if (value < 1000)
    (*increase)--;
}

static int start_input_loop()
{
  on_loop = true;
  const int *rules = rule_set[current_display_state].rules;
  int current_selection = 0;
  int prev_selection = -1;
  int increase = 0;

  while (1)
  {
    if (!check_triggered(rules, &current_selection))
      break;
    update_by_rule(rules, current_selection);
    read_joystick(&increase);

    if ((current_selection != prev_selection) || (increase != 0))
    {
      prev_selection = current_selection;

      switch (current_display_state)
      {
      case D_WELCOME:
        show_welcome();
        break;

      case D_ALARM_TYPE_SELECT:
        show_alarm_select(current_selection, increase);
        break;

      case D_SHORT_TIME_SELECT:
        show_setup_short(current_selection, increase);
        break;

      case D_LONG_WEEK_SELECT:
        show_select_week(current_selection, increase);
        break;

      case D_SET_CURRENT_DATE:
        show_set_date(current_selection, increase, true);
        break;
      case D_LONG_TIME_SELECT:
        show_set_date(current_selection, increase, false);
        break;

      case D_QUIET_SELECT:
        show_quiet_select(current_selection, increase);
        break;
      }
    }
    increase = 0;
  }
  on_loop = false;

  if (current_selection >= (rules[0] + rules[1]) - 1)
  {
    if (current_display_state == D_ALARM_TYPE_SELECT && !current_timer_struct.is_short)
      return 2;
    else if (current_display_state == D_SHORT_TIME_SELECT)
      return 4;
    return 1;
  }
  if (current_selection == 0)
  {
    if (current_display_state == D_SET_CURRENT_DATE)
      return -2;
    else if (current_display_state == D_QUIET_SELECT && current_timer_struct.is_short)
      return -4;
    return -1;
  }
}

inline int change_display_state(display_state new_state)
{
  current_display_state = new_state;
  return start_input_loop();
}

static bool setup_gpio()
{
  if (!i2c_init(I2C_PORT, 100 * 1000))
    return false;
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
  return true;
}

static void reset_state()
{
  current_timer_struct.current_date = (datetime_t){-1, 0, -1, 0, 0, 0, 0};
  current_timer_struct.selected_date = (datetime_t){-1, 0, -1, 0, 0, 0, 0};
  current_timer_struct.is_quiet = false;
  current_timer_struct.is_this_week = true;
  current_timer_struct.is_short = true;

  short_hours = 0;
  short_minutes = 0;
  short_seconds = 0;

  long_hour = 0;
  long_minute = 0;
  long_weekday = 0;

  current_hour = 0;
  current_minute = 0;
  current_weekday = 0;
}

void initialize_display()
{
  if (!setup_gpio())
    printf("    DEBUG: DISPLAY GPIO SETUP FAILED!");
  display.width = 128;
  display.height = 64;
  display.external_vcc = false;
  ssd1306_init(&display, display.width, display.height, OLED_ADDR, I2C_PORT);
  ssd1306_contrast(&display, 100);
  ssd1306_clear(&display);
  ssd1306_show(&display);
  reset_state();
  return;
}
