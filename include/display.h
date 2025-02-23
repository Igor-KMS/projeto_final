#ifndef DISPLAY_H
#define DISPLAY_H

  typedef enum {
    D_WELCOME,
    D_SET_CURRENT_DATE,
    D_ALARM_TYPE_SELECT,
    D_SHORT_TIME_SELECT,
    D_LONG_WEEK_SELECT,
    D_LONG_TIME_SELECT,
    D_QUIET_SELECT,
  } display_state;

  extern bool on_loop;
  extern volatile bool next_triggered;
  extern volatile bool prev_triggered;

  extern int current_hour, current_minute, current_weekday, short_hours, short_minutes, short_seconds, long_hour, long_minute, long_weekday;

  void initialize_display(void);
  int change_display_state(display_state);
  void clamp_inclusive(int *, int, int);
  void hide_display(void);
  void setup_struct(void);

#endif
