/**
 * @file spi_linux.h
 * @brief Header file for... TBD.
 * @details TBD.
 * @author João Cláudio Elsen Barcellos
 * @version 0.0.1
 * @date 21/04/2026
 */

#ifndef SPI_LINUX_H_
#define SPI_LINUX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/spi/spidev.h>
#include <stdint.h>

#define SPI_TXN_MAX_TRANSFERS 16

struct spi_txn {
  struct spi_ioc_transfer xfers[SPI_TXN_MAX_TRANSFERS];
  unsigned int count;
  unsigned int bound;
};

int spi_txn_prepare(struct spi_txn *txn, unsigned int count);

int spi_txn_bind_transfer(struct spi_txn *txn, unsigned int index,
                          const uint8_t *tx, uint8_t *rx, uint32_t len);

int spi_txn_bind_write(struct spi_txn *txn, unsigned int index,
                       const uint8_t *tx, uint32_t len);

int spi_txn_bind_read(struct spi_txn *txn, unsigned int index, uint8_t *rx,
                      uint32_t len);

int spi_txn_set_speed(struct spi_txn *txn, unsigned int index,
                      uint32_t speed_hz);

int spi_txn_set_delay(struct spi_txn *txn, unsigned int index,
                      uint16_t delay_us);

int spi_txn_set_bits(struct spi_txn *txn, unsigned int index,
                     uint8_t bits_per_word);

int spi_txn_execute(struct spi_txn *txn, int fd);

void spi_txn_finalize(struct spi_txn *txn);
void spi_txn_reset(struct spi_txn *txn);

int spi_txn_open(const char *path);
int spi_txn_close(int fd);

#ifdef __cplusplus
}
#endif

#endif /* SPI_LINUX_H_ */
