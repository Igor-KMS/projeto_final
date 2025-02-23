#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "leds.pio.h"
#include "htimer.h"

#define MATRIX_PIN 7

PIO pio;
uint sm;
uint offset;

uint t = 0;

void put_pixel(uint32_t pixel_grb)
{
  pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

uint32_t make_pixel(uint8_t r, uint8_t g, uint8_t b)
{
  return ((uint32_t)(r) << 8) |
         ((uint32_t)(g) << 16) |
         (uint32_t)(b);
}

void pattern_firework(uint len, uint t)
{
  uint max_radius = 3;
  uint max_brightness = 7;
  t %= max_radius * 3;

  for (uint y = 0; y < 5; ++y)
  {
    for (uint x = 0; x < 5; ++x)
    {
      int dist = abs(x - 2) + abs(y - 2);
      if (dist == t / 3)
      {
        uint r = (rand() % 5 + 5) * 2;
        uint g = (rand() % 5) * 2;
        uint b = (rand() % 5) * 2;
        put_pixel(make_pixel(r, g, b));
      }
      else
      {
        put_pixel(make_pixel(0, 0, 0));
      }
    }
  }

  if (t / 3 == max_radius - 1)
  {
    put_pixel(make_pixel(10, 5, 2));
  }
}

void trigger_visuals()
{
  pattern_firework(25, t);
  busy_wait_us_32(100000);
  t++;
}

void clear_visuals()
{
  for (int i = 0; i < 25; i++)
    put_pixel(make_pixel(0, 0, 0));
}

void initialize_leds()
{
  hard_assert(pio_claim_free_sm_and_add_program_for_gpio_range(&led_matrix_program, &pio, &sm, &offset, MATRIX_PIN, 1, true));
  led_matrix_program_init(pio, sm, offset, MATRIX_PIN, 800000); // 1/800kHZ -> 1,25 us -> data transfer time of ws2812b (TH + TL)
  return;
}
