/**
 * @file gpio_linux.h
 * @brief Header file for... TBD.
 * @details TBD.
 * @author João Cláudio Elsen Barcellos
 * @version 0.0.1
 * @date 21/04/2026
 */

#ifndef GPIO_LINUX_H_
#define GPIO_LINUX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <gpiod.h>
#include <stdint.h>

struct gpio {
  struct gpiod_chip *chip;
  struct gpiod_line *line;
};

int gpio_init(struct gpio *g, const char *chip_name, uint8_t pin,
              int default_value);

int gpio_set(struct gpio *g, int value);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_LINUX_H_ */
