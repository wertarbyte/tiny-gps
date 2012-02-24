#include <stdlib.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "nmea.h"
#include "usiTwiSlave.h"

#define TWIADDRESS 0x11

extern struct nmea_rmc_t nmea_rmc;

int main(void) {
	UCSRB = (1<<RXEN);
	UCSRC = (0<<USBS)|(3<<UCSZ0);
	UCSRA = (0<<U2X);
	UBRRL = 12;

	usiTwiSlaveInit(TWIADDRESS);
	usiTwiSetTransmitWindow( &nmea_rmc, sizeof(struct nmea_rmc_t) );

	sei();
	while (1) {
		/* read from the serial UART */
		while(!(UCSRA & (1<<RXC))) {}
		char c = UDR;
		nmea_process_character(c);
	}
}
