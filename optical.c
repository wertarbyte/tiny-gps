#include "config.h"
#if USE_OPTICAL
/* optical flow sensor handler */
#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "optical.h"

#if __AVR__
#include <util/atomic.h>
#define ATOMIC(t) ATOMIC_BLOCK(t)
#else
#define ATOMIC(t)
#endif

#define OPTICAL_SCLK_PORT PORTA
#define OPTICAL_SDIO_PORT PORTA
#define OPTICAL_CSEL_PORT PORTB
#define OPTICAL_SCLK_DDR  DDRA
#define OPTICAL_SDIO_DDR  DDRA
#define OPTICAL_CSEL_DDR  DDRB
#define OPTICAL_SCLK_PIN  PINA
#define OPTICAL_SDIO_PIN  PINA
#define OPTICAL_CSEL_PIN  PINB
#define OPTICAL_SCLK_BIT  PA0
#define OPTICAL_SDIO_BIT  PA1
#define OPTICAL_CSEL_BIT  PB6

#define IO_DELAY   4

/* values for ADNS 5050 */
#define ID_REG     0x00
#define MOTION_REG 0x02
#define DX_REG     0x03
#define DY_REG     0x04
#define QUAL_REG   0x05
#define RESET_REG  0x3a
#define RESET_VAL  0x5a
#define CTRL1_REG  0x0d
#define CTRL2_REG  0x19

static void _spi_write(uint8_t d) {
	OPTICAL_SDIO_DDR |= 1<<OPTICAL_SDIO_BIT;
	for (int8_t i=7; i>=0; i--) {
		OPTICAL_SCLK_PORT &= ~(1<<OPTICAL_SCLK_BIT);
		_delay_us(1);
		if (d & 1<<i) {
			OPTICAL_SDIO_PORT |= 1<<OPTICAL_SDIO_BIT;
		} else {
			OPTICAL_SDIO_PORT &= ~(1<<OPTICAL_SDIO_BIT);
		}
		_delay_us(1);
		OPTICAL_SCLK_PORT |= 1<<OPTICAL_SCLK_BIT;
		_delay_us(1);
	}
}

static uint8_t optical_read(uint8_t addr) {
	OPTICAL_CSEL_PORT &= ~(1<<OPTICAL_CSEL_BIT);
	_spi_write(addr);
	OPTICAL_SDIO_DDR &= ~(1<<OPTICAL_SDIO_BIT);
	_delay_us(IO_DELAY);
	uint8_t res = 0;
	for (int8_t i=7; i>=0; i--) {
		OPTICAL_SCLK_PORT &= ~(1<<OPTICAL_SCLK_BIT);
		_delay_us(1);
		OPTICAL_SCLK_PORT |= 1<<OPTICAL_SCLK_BIT;
		_delay_us(1);
		if (OPTICAL_SDIO_PIN & 1<<OPTICAL_SDIO_BIT) {
			res |= 1<<i;
		}
	}
	OPTICAL_CSEL_PORT |= (1<<OPTICAL_CSEL_BIT);
	return res;
}

static void optical_write(uint8_t addr, uint8_t data) {
	OPTICAL_CSEL_PORT &= ~(1<<OPTICAL_CSEL_BIT);
	_delay_us(IO_DELAY);
	_spi_write(addr | 1<<7);
	_delay_us(IO_DELAY);
	_spi_write(data);
	_delay_us(IO_DELAY);
	OPTICAL_CSEL_PORT |= 1<<OPTICAL_CSEL_BIT;
}

void optical_init(void) {
	OPTICAL_SCLK_DDR |= 1<<OPTICAL_SCLK_BIT;
	OPTICAL_SDIO_DDR &= ~(1<<OPTICAL_SDIO_BIT);
	OPTICAL_CSEL_DDR |= 1<<OPTICAL_CSEL_BIT;

	// toggle chip select to reset sensor
	OPTICAL_CSEL_PORT |= 1<<OPTICAL_CSEL_BIT;
	_delay_us(100);
	OPTICAL_CSEL_PORT &= ~(1<<OPTICAL_CSEL_BIT);

	// raise CLK
	OPTICAL_SCLK_PORT |= 1<<OPTICAL_SDIO_BIT;
	_delay_us(100);

	optical_write(RESET_REG, RESET_VAL);
	_delay_us(50);
}

void optical_query(struct optical_data_t *output) {
	if (optical_read(MOTION_REG)) {
		int8_t dy = optical_read(DY_REG);
		int8_t dx = optical_read(DX_REG);
		ATOMIC(ATOMIC_FORCEON) {
			output->dx += dx;
			output->dy += dy;
		}
	}
}
#endif
