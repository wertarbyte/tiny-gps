#include <stdlib.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "nmea.h"

int main(void) {
	DDRD |= 1<<PD5;

	char *input = "$GPRMC,125934.000,V,5123.1457,N,00645.0808,E,,,020811,,*13\r\n";
	while (input[0]) {
		nmea_process_character(input[0]);
		input++;
	}
	PORTD |= 1<<PD5;
	sei();
	while (1) {
	}
}
