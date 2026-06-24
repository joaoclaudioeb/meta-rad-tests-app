/**
 * @file spi_linux.c
 * @brief Source file for... TBD.
 * @details TBD.
 * @author João Cláudio Elsen Barcellos
 * @version 0.0.1
 * @date 21/04/2026
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <include/libdac81408/gpio_linux.h>
#include <include/libdac81408/spi_linux.h>

int spi_txn_prepare(struct spi_txn *txn, unsigned int count) {
  if (count == 0 || count > SPI_TXN_MAX_TRANSFERS)
    return -EINVAL;

  memset(txn, 0, sizeof(*txn));
  txn->count = count;
  txn->bound = 0;

  return 0;
}

int spi_txn_bind_transfer(struct spi_txn *txn, unsigned int index,
                          const uint8_t *tx, uint8_t *rx, uint32_t len) {
  if (index >= txn->count || len == 0)
    return -EINVAL;

  txn->xfers[index].tx_buf = (unsigned long)tx;
  txn->xfers[index].rx_buf = (unsigned long)rx;
  txn->xfers[index].len = len;
  txn->xfers[index].cs_change = 0;
  txn->bound++;

  return 0;
}

int spi_txn_bind_write(struct spi_txn *txn, unsigned int index,
                       const uint8_t *tx, uint32_t len) {
  if (index >= txn->count || len == 0)
    return -EINVAL;

  txn->xfers[index].tx_buf = (unsigned long)tx;
  txn->xfers[index].rx_buf = (unsigned long)NULL;
  txn->xfers[index].len = len;
  txn->xfers[index].cs_change = 0;
  txn->bound++;

  return 0;
}

int spi_txn_bind_read(struct spi_txn *txn, unsigned int index, uint8_t *rx,
                      uint32_t len) {
  if (index >= txn->count || len == 0)
    return -EINVAL;

  txn->xfers[index].tx_buf = (unsigned long)NULL;
  txn->xfers[index].rx_buf = (unsigned long)rx;
  txn->xfers[index].len = len;
  txn->xfers[index].cs_change = 0;
  txn->bound++;

  return 0;
}

int spi_txn_set_speed(struct spi_txn *txn, unsigned int index,
                      uint32_t speed_hz) {
  if (index >= txn->count)
    return -EINVAL;

  txn->xfers[index].speed_hz = speed_hz;

  return 0;
}

int spi_txn_set_delay(struct spi_txn *txn, unsigned int index,
                      uint16_t delay_us) {
  if (index >= txn->count)
    return -EINVAL;

  txn->xfers[index].delay_usecs = delay_us;

  return 0;
}

int spi_txn_set_bits(struct spi_txn *txn, unsigned int index,
                     uint8_t bits_per_word) {
  if (index >= txn->count)
    return -EINVAL;

  txn->xfers[index].bits_per_word = bits_per_word;

  return 0;
}

int spi_txn_execute(struct spi_txn *txn, int fd) {
  int ret;

  if (txn->bound != txn->count)
    return -EINVAL;

  ret = ioctl(fd, SPI_IOC_MESSAGE(txn->count), txn->xfers);
  if (ret < 0)
    return -errno;

  return ret;
}

void spi_txn_finalize(struct spi_txn *txn) { memset(txn, 0, sizeof(*txn)); }

void spi_txn_reset(struct spi_txn *txn) {
  unsigned int count = txn->count;

  memset(txn->xfers, 0, sizeof(txn->xfers[0]) * count);
  txn->bound = 0;
}

int spi_txn_open(const char *path) {
  int fd;

  fd = open(path, O_RDWR);
  if (fd < 0)
    return -errno;
  uint8_t mode = SPI_MODE_1;
  ioctl(fd, SPI_IOC_WR_MODE, &mode);

  return fd;
}

int spi_txn_close(int fd) {
  if (close(fd) < 0)
    return -errno;

  return 0;
}
