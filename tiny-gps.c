#include <stdlib.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "nmea.h"
#include "sonar.h"
#include "optical.h"
#include "nav_structs.h"
#include "usiTwiSlave.h"

#include "config.h"


#if USE_GPS
#define RX_BUF_SIZE 4
static volatile char rx_buf[RX_BUF_SIZE];
static volatile uint8_t rx_buf_r = 0;
static volatile uint8_t rx_buf_w = 0;
#endif

struct nav_data_t nav_data = {{0}};

#if USE_OPTICAL
static void window_trap(void) {
	/* did we read the last byte of the data? then reset the counters! */
	memset(&nav_data.optical, 0, sizeof(nav_data.optical));
}
#define TRAP_ADDR &window_trap
#else
#define TRAP_ADDR NULL
#endif

#if USE_GPS
static void init_gps_unit(void) {
	/* enable RX pin and interrupt */
	UCSRB = ( 1<<RXEN | 1<<RXCIE
#ifdef GPS_INIT_STRING
	/* also enable the TX pin */
	| 1<<TXEN
#endif
	);

	UCSRC = (0<<USBS)|(3<<UCSZ0);

#ifdef GPS_INIT_BAUD
	/* set initial baud rate for initialization */
	#define BAUD GPS_INIT_BAUD
	#include <util/setbaud.h>
	UCSRA = (USE_2X<<U2X);
	UBRRL = UBRRL_VALUE;
	#undef BAUD
#endif

#ifdef GPS_INIT_STRING
	/* transmit configuration commands */
	PGM_P init = PSTR(GPS_INIT_STRING);
	const char *p = init;
	char c;
	while ((c = pgm_read_byte(p++)) != '\0') {
		UDR = c;
		/* wait for transmission to complete */
		while(!(UCSRA & (1<<UDRE)));
	}
#endif

	/* now set final baud rate */
	#define BAUD GPS_BAUD
	#include <util/setbaud.h>
	UCSRA = (USE_2X<<U2X);
	UBRRL = UBRRL_VALUE;
}
#endif

int main(void) {
#if USE_GPS
	init_gps_unit();
	nmea_init(&nav_data.gps);
#endif

#if USE_SONAR
	sonar_init();
#endif
#if USE_OPTICAL
	optical_init();
#endif

	usiTwiSlaveInit(TWIADDRESS);
	usiTwiSetTransmitWindow( &nav_data, sizeof(nav_data) );
#if USE_OPTICAL
	usiTwiSlaveSetTrap(TRAP_ADDR);
#endif

#if LED_FIX_INDICATOR
	DDRD = (1<<PD5);
	PORTD &= ~(1<<PD5);
#endif

	sei();
	while (1) {
#if USE_GPS
		/* read from the serial UART */
		while (rx_buf_w != rx_buf_r) {
			nmea_process_character(rx_buf[rx_buf_r]);
			rx_buf_r++;
			if (rx_buf_r >= RX_BUF_SIZE) {
				rx_buf_r = 0;
			}
		}
#endif
		/* toggle gps fix indicator */
#if LED_FIX_INDICATOR
		if (nav_data.gps.flags & 1<<NMEA_RMC_FLAGS_STATUS_OK) {
			PORTD |= (1<<PD5);
		} else {
			PORTD &= ~(1<<PD5);
		}
#endif
#if USE_SONAR
		nav_data.sonar.distance = sonar_last_pong();
		if (sonar_ready()) {
			sonar_ping();
		}
#endif
#if USE_OPTICAL
		optical_query(&nav_data.optical);
#endif
	}
}

#if USE_GPS
ISR(USART_RX_vect) {
	rx_buf[rx_buf_w] = UDR;
	rx_buf_w++;
	if (rx_buf_w >= RX_BUF_SIZE) {
		rx_buf_w = 0;
	}
}
#endif
