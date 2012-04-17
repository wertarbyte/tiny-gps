#include <stdlib.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "nmea.h"
#include "usiTwiSlave.h"

#include "config.h"

#include <util/setbaud.h>

extern struct nmea_data_t nmea_data;
int main(void) {
	DDRD = (1<<PD5);
	UCSRB = (1<<RXEN);
	UCSRC = (0<<USBS)|(3<<UCSZ0);

	UCSRA = (USE_2X<<U2X);
	UBRRL = UBRRL_VALUE;

	usiTwiSlaveInit(TWIADDRESS);
	usiTwiSetTransmitWindow( &nmea_data, sizeof(struct nmea_data_t) );

	PORTD &= ~(1<<PD5);

	sei();
	while (1) {
		/* read from the serial UART */
		while(!(UCSRA & (1<<RXC))) {}
		char c = UDR;
		nmea_process_character(c);

		/* toggle gps fix indicator */
		if (nmea_data.flags & 1<<NMEA_RMC_FLAGS_STATUS_OK) {
			PORTD |= (1<<PD5);
		} else {
			PORTD &= ~(1<<PD5);
		}
	}
}
