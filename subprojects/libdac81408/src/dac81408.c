/**
 * @file dac81408.c
 * @brief Source file for controlling TI's 16-bit DAC, the DAC81408.
 * @details This code was based on FelipeJuliano24's one
 * (https://github.com/FelipeJuliano24/DAC81408_lib/blob/main/dac81408.c).
 * @author João Cláudio Elsen Barcellos
 * @version 0.0.1
 * @date 21/04/2026
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <include/libdac81408/dac81408.h>
#include <include/libdac81408/spi_linux.h>

static void dac81408_sleep_ms(uint32_t ms) {
  struct timespec ts;

  ts.tv_sec = 0U;
  ts.tv_nsec = ms * 1000000UL;

  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
}

static void dac81408_sleep_us(uint32_t us) {
  struct timespec ts;

  ts.tv_sec = 0U;
  ts.tv_nsec = us * 1000UL;

  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
}

static inline void dac81408_tcsh_delay(void) {
  dac81408_sleep_us(1); /* minimum tCSH from datasheet */
}

static inline void dac81408_tldac_delay(void) {
  dac81408_sleep_us(1); /* minimum tLDAC from datasheet */
}

void dac81408_init(dac81408_t *dev, const char *spidev_path,
                   const char *gpiochip_name, uint8_t rst_pin,
                   uint8_t ldac_pin) {

  dev->spidev_path = spidev_path;
  dev->gpiochip_name = gpiochip_name;
  dev->rst_pin = rst_pin;
  dev->ldac_pin = ldac_pin;

  /* Init GPIOs only if defined */
  if (dev->rst_pin != DAC81408_PIN_UNUSED) {
    gpio_init(&dev->rst_gpio, dev->gpiochip_name, dev->rst_pin, 1);
  }

  if (dev->ldac_pin != DAC81408_PIN_UNUSED) {
    gpio_init(&dev->ldac_gpio, dev->gpiochip_name, dev->ldac_pin, 1);
  }

  /* Defaults */
  dev->dacrange0_cache = 0x0000;
  dev->dacrange1_cache = 0x0000;
  dev->spiconfig_cache = 0x0AA4;
  dev->genconfig_cache = 0x7F00;
  dev->syncconfig_cache = 0x0000;
  dev->dacpwdwn_cache = 0xFFFF;
}

int dac81408_config(dac81408_t *dev) {
  if (dev->rst_pin != DAC81408_PIN_UNUSED) {
    gpio_set(&dev->rst_gpio, 0);
    dac81408_sleep_ms(150);

    gpio_set(&dev->rst_gpio, 1);
    dac81408_sleep_ms(150);
  }

  uint16_t spiconfig = DAC81408_TEMPALM_EN(0) | DAC81408_DACBUSY_EN(0) |
                       DAC81408_CRCALM_EN(0) | DAC81408_SOFTTOGGLE_EN(0) |
                       DAC81408_DEV_PWDWN(0) | DAC81408_CRC_EN(0) |
                       DAC81408_STR_EN(0) | DAC81408_SDO_EN(1) |
                       DAC81408_FSDO(0) | ((0x01) << 7);
  dac81408_write_register(dev, DAC81408_REG_SPICONFIG, spiconfig);

  dev->spiconfig_cache = dac81408_read_register(dev, DAC81408_REG_SPICONFIG);

  return (dev->spiconfig_cache == spiconfig) ? 0 : -1;
}

void dac81408_write_register(dac81408_t *dev, uint8_t reg, uint16_t wdata) {
  uint8_t cmd = (DAC81408_WREG << 7) | (reg & 0x3F);
  uint8_t msb = (wdata >> 8) & 0xFF;
  uint8_t lsb = wdata & 0xFF;

  struct spi_txn txn;
  uint8_t idx = 0;

  spi_txn_prepare(&txn, 3);

  spi_txn_bind_write(&txn, idx, &cmd, 1);
  ++idx;

  spi_txn_bind_write(&txn, idx, &msb, 1);
  ++idx;

  spi_txn_bind_write(&txn, idx, &lsb, 1);
  ++idx;

  int fd = spi_txn_open(dev->spidev_path);
  if (fd < 0) {
    perror("Failed to open spidev path!");
    return;
  }

  if (spi_txn_execute(&txn, fd) < 0) {
    fprintf(stderr, "Failed to execute transfer!");
    return;
  }

  spi_txn_close(fd);

  dac81408_tcsh_delay();
}

uint16_t dac81408_read_register(dac81408_t *dev, uint8_t reg) {
  uint8_t cmd = (DAC81408_RREG << 7) | (reg & 0x3F);
  uint8_t dummy = 0x00;

  struct spi_txn txn;
  uint8_t idx = 0;

  spi_txn_prepare(&txn, 3);

  spi_txn_bind_write(&txn, idx, &cmd, 1);
  ++idx;

  spi_txn_bind_write(&txn, idx, &dummy, 1);
  ++idx;

  spi_txn_bind_write(&txn, idx, &dummy, 1);
  ++idx;

  int fd = spi_txn_open(dev->spidev_path);
  if (fd < 0) {
    perror("Failed to open spidev path!");
    exit(1);
  }

  if (spi_txn_execute(&txn, fd) < 0) {
    fprintf(stderr, "Failed to execute transfer!");
    exit(1);
  }

  spi_txn_close(fd);

  dac81408_tcsh_delay();

  spi_txn_reset(&txn);

  idx = 0;

  uint8_t buf[3];

  spi_txn_bind_read(&txn, idx, &buf[0], 1);
  ++idx;

  spi_txn_bind_read(&txn, idx, &buf[1], 1);
  ++idx;

  spi_txn_bind_read(&txn, idx, &buf[2], 1);
  ++idx;

  fd = spi_txn_open(dev->spidev_path);
  if (fd < 0) {
    perror("Failed to open spidev path!");
    exit(1);
  }

  if (spi_txn_execute(&txn, fd) < 0) {
    fprintf(stderr, "Failed to execute transfer!");
    exit(1);
  }

  spi_txn_close(fd);

  dac81408_tcsh_delay();

  return ((buf[1] << 8) | buf[2]);
}

void dac81408_set_ch_enabled(dac81408_t *dev, int ch, bool state) {
  if (ch < 0 || ch > 7)
    return;

  uint16_t dacpwdwn = dac81408_read_register(dev, DAC81408_REG_DACPWDWN);
  uint16_t mask = 1 << (ch + 4);

  if (state) {
    dac81408_write_register(dev, DAC81408_REG_DACPWDWN,
                            dacpwdwn & ~mask); /* power up (bit = 0) */
  } else {
    dac81408_write_register(dev, DAC81408_REG_DACPWDWN,
                            dacpwdwn | mask); /* power down (bit = 1) */
  }

  dev->dacpwdwn_cache = dac81408_read_register(dev, DAC81408_REG_DACPWDWN);
}

bool dac81408_get_ch_enabled(dac81408_t *dev, int ch) {
  if (ch < 0 || ch > 7)
    return -1;

  uint16_t dacpwdwn = dac81408_read_register(dev, DAC81408_REG_DACPWDWN);

  return !((dacpwdwn >> (ch + 4)) & 1);
}

void dac81408_set_range(dac81408_t *dev, int ch, dac81408_range_t range) {
  if (ch < 0 || ch > 7)
    return;

  uint8_t dacrange_ch = ch & 0x03;
  uint16_t shift = 4 * dacrange_ch;
  uint16_t mask = 0xF << shift;

  if (ch < 4) {
    dev->dacrange0_cache =
        (dev->dacrange0_cache & ~mask) | ((range << shift) & mask);
    dac81408_write_register(dev, DAC81408_REG_DACRANGE0, dev->dacrange0_cache);
  } else {
    dev->dacrange1_cache =
        (dev->dacrange1_cache & ~mask) | ((range << shift) & mask);
    dac81408_write_register(dev, DAC81408_REG_DACRANGE1, dev->dacrange1_cache);
  }
}

int dac81408_get_range(dac81408_t *dev, int ch) {
  if (ch < 0 || ch > 7)
    return -1;

  uint8_t dacrange_ch = ch & 0x3;
  uint16_t shift = 4 * dacrange_ch;

  if (ch < 4) {
    return ((dev->dacrange0_cache >> shift) & 0xF);
  } else {
    return ((dev->dacrange1_cache >> shift) & 0xF);
  }
}

void dac81408_set_out(dac81408_t *dev, int ch, uint16_t val) {
  if (ch < 0 || ch > 7)
    return;

  uint8_t ch_reg = DAC81408_REG_DAC0 + ch;

  dac81408_write_register(dev, ch_reg, val);
}

uint16_t dac81408_get_out(dac81408_t *dev, uint8_t reg) {
  uint16_t val = dac81408_read_register(dev, reg);

  return val;
}

/* Functions that do not work or were not tested yet */
void dac81408_set_int_reference(dac81408_t *dev, dac81408_ref_state_t state) {
  uint16_t mask_preserve = 0x3C7F;
  uint16_t ref_bit = (state == DAC81408_REF_OFF) ? (1 << 14) : 0;

  dev->genconfig_cache = (dev->genconfig_cache & mask_preserve) | ref_bit;
  dac81408_write_register(dev, DAC81408_REG_GENCONFIG, dev->genconfig_cache);
}

int dac81408_get_int_reference(dac81408_t *dev) {
  uint16_t res = dac81408_read_register(dev, DAC81408_REG_GENCONFIG);
  return (res & (1 << 14)) ? DAC81408_REF_OFF : DAC81408_REF_ON;
}

void dac81408_set_sync(dac81408_t *dev, int ch, dac81408_sync_t mode) {
  if (ch < 0 || ch > 7)
    return;

  if (mode == DAC81408_SYNC_LDAC) {
    dev->syncconfig_cache |= (1UL << (ch + 4));
  } else {
    dev->syncconfig_cache &= ~(1UL << (ch + 4));
  }
  dac81408_write_register(dev, DAC81408_REG_SYNCCONFIG, dev->syncconfig_cache);
}

void dac81408_trigger_ldac(dac81408_t *dev) {
  if (dev->ldac_pin == DAC81408_PIN_UNUSED)
    return;

  gpio_set(&dev->ldac_gpio, 0);
  dac81408_tldac_delay();
  gpio_set(&dev->ldac_gpio, 1);
}
