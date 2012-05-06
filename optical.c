#include "config.h"
#if USE_OPTICAL
/* optical flow sensor handler */
#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "optical.h"

#define OPTICAL_SCLK_PORT PORTA
#define OPTICAL_SDIO_PORT PORTA
#define OPTICAL_SCLK_DDR  DDRA
#define OPTICAL_SDIO_DDR  DDRA
#define OPTICAL_SCLK_PIN  PINA
#define OPTICAL_SDIO_PIN  PINA
#define OPTICAL_SCLK_BIT  PA0
#define OPTICAL_SDIO_BIT  PA1

#define IO_DELAY   4

#define ID_REG     0x00
#define MOTION_REG 0x02
#define DX_REG     0x03
#define DY_REG     0x04
#define QUAL_REG   0x05
#define RESET_REG  0x3a
#define CTRL1_REG  0x0d
#define CTRL2_REG  0x19

static void _spi_write(uint8_t d) {
	OPTICAL_SDIO_DDR |= 1<<OPTICAL_SDIO_BIT;
	for (uint8_t i=0; i<8; i++) {
		OPTICAL_SCLK_PORT &= ~(1<<OPTICAL_SDIO_BIT);
		if (d & 1<<i) {
			OPTICAL_SDIO_PORT |= 1<<OPTICAL_SDIO_BIT;
		} else {
			OPTICAL_SDIO_PORT &= ~(1<<OPTICAL_SDIO_BIT);
		}
		_delay_us(1);
		OPTICAL_SCLK_PORT |= 1<<OPTICAL_SDIO_BIT;
	}
}

static uint8_t optical_read(uint8_t addr) {
	_spi_write(addr);
	_delay_us(IO_DELAY);
	OPTICAL_SDIO_DDR &= ~(1<<OPTICAL_SDIO_BIT);
	uint8_t res = 0;
	for (uint8_t i=0; i<8; i++) {
		OPTICAL_SCLK_PORT &= ~(1<<OPTICAL_SDIO_BIT);
		_delay_us(1);
		OPTICAL_SCLK_PORT |= 1<<OPTICAL_SDIO_BIT;
		if (OPTICAL_SDIO_PIN & 1<<OPTICAL_SDIO_BIT) {
			res |= 1<<i;
		}
	}
	return res;
}

static void optical_write(uint8_t addr, uint8_t data) {
	_spi_write(addr);
	_spi_write(data);
}

void optical_init(void) {
	OPTICAL_SCLK_DDR |= 1<<OPTICAL_SCLK_BIT;
	OPTICAL_SDIO_DDR |= 1<<OPTICAL_SDIO_BIT;

	optical_write(RESET_REG, 0x5a);
	_delay_us(50);

	// start motion detection and reset buffers
	optical_write(MOTION_REG, 1);
}

void optical_query(struct optical_data_t *output) {
	int8_t dx;
	int8_t dy;
	if (optical_read(MOTION_REG)) {
		dx = optical_read(DX_REG);
		dy = optical_read(DY_REG);
		output->dx += dx;
		output->dy += dy;
	}
}
#endif
