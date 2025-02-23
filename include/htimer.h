#ifndef HTIMER_H
#define HTIMER_H

  typedef struct
  {
    datetime_t current_date;
    datetime_t selected_date;
    bool is_this_week;
    bool is_short;
    bool is_quiet;
  } alarm_struct;

  extern alarm_struct current_timer_struct;
  extern volatile bool alarm_triggered;

  void initialize_timer(void);
  void schedule_alarm_short(alarm_struct*);
  void schedule_alarm_long(alarm_struct*);

#endif
