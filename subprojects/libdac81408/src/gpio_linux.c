/**
 * @file gpio_linux.c
 * @brief Source file for... TBD.
 * @details TBD.
 * @author João Cláudio Elsen Barcellos
 * @version 0.0.1
 * @date 21/04/2026
 */

#include <stdio.h>

#include <include/libdac81408/gpio_linux.h>

int gpio_init(struct gpio *g, const char *chip_name, uint8_t pin,
              int default_value) {
  if (!g || !chip_name)
    return -1;

  g->chip = gpiod_chip_open_by_name(chip_name);
  if (!g->chip) {
    perror("open gpiod chip");
    return -1;
  }

  g->line = gpiod_chip_get_line(g->chip, pin);
  if (!g->line) {
    perror("get gpiod line");
    gpiod_chip_close(g->chip);
    g->chip = NULL;
    return -1;
  }

  if (gpiod_line_request_output(g->line, "dac-gpio", default_value) < 0) {
    perror("request gpiod line output");
    gpiod_chip_close(g->chip);
    g->line = NULL;
    g->chip = NULL;
    return -1;
  }

  return 0;
}

int gpio_set(struct gpio *g, int value) {
  if (!g || !g->chip || !g->line)
    return -1;

  return gpiod_line_set_value(g->line, value);
}
