#ifndef BUTTONS_H
#define BUTTONS_H

  #define BUTTON_A 5
  #define BUTTON_B 6

  void initialize_buttons(void);
  void send_clicked_display(uint);
  void toggle_irq(uint8_t, bool);

#endif
