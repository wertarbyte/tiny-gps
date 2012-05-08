#include <stdlib.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "nmea.h"
#include "usiTwiSlave.h"

#include "config.h"

#include <util/setbaud.h>

#define RX_BUF_SIZE 4
static volatile char rx_buf[RX_BUF_SIZE];
static volatile uint8_t rx_buf_r;
static volatile uint8_t rx_buf_w;

extern struct nmea_data_t nmea_data;
int main(void) {
	DDRD = (1<<PD5);
	UCSRB = (1<<RXEN);

	UCSRB = (1<<RXEN | 1<<RXCIE);
	UCSRC = (0<<USBS)|(3<<UCSZ0);

	UCSRA = (USE_2X<<U2X);
	UBRRL = UBRRL_VALUE;

	usiTwiSlaveInit(TWIADDRESS);
	usiTwiSetTransmitWindow( &nmea_data, sizeof(struct nmea_data_t) );

	rx_buf_r = 0;
	rx_buf_w = 0;

	PORTD &= ~(1<<PD5);

	sei();
	while (1) {
		/* read from the serial UART */
		while (rx_buf_w != rx_buf_r) {
			nmea_process_character(rx_buf[rx_buf_r]);
			rx_buf_r++;
			if (rx_buf_r >= RX_BUF_SIZE) {
				rx_buf_r = 0;
			}
		}

		/* toggle gps fix indicator */
		if (nmea_data.flags & 1<<NMEA_RMC_FLAGS_STATUS_OK) {
			PORTD |= (1<<PD5);
		} else {
			PORTD &= ~(1<<PD5);
		}
	}
}

ISR(USART_RX_vect) {
	rx_buf[rx_buf_w] = UDR;
	rx_buf_w++;
	if (rx_buf_w >= RX_BUF_SIZE) {
		rx_buf_w = 0;
	}
}
