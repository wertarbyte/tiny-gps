#include "config.h"
#if USE_SONAR
/* sonar handler */
#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "sonar.h"

#define SONAR_TRIGGER_PORT PORTD
#define SONAR_TRIGGER_DDR DDRD
#define SONAR_TRIGGER_BIT PD2

static volatile int16_t sonar_pong = 0;

static volatile enum {
	SONAR_READY,
	SONAR_PING,
	SONAR_PONG,
} sonar_state = SONAR_READY;

void sonar_init(void) {
	/* configure trigger output pin */
	SONAR_TRIGGER_DDR |= 1<<SONAR_TRIGGER_BIT;
	
	/* configure timer for measurement of echo time */

	/* - clock scaling /8 (yielding ticks of 0,000001s)
	 * - input capture on rising edge
	 */
	TCCR1B = 1<<CS11 | 1<<ICES1;
	/* enable capture interrupt */
	TIMSK = (1<<ICIE1 | 1<<TOIE1);
}

uint8_t sonar_ready(void) {
	return sonar_state == SONAR_READY;
}

void sonar_ping(void) {
	cli();
	sonar_state = SONAR_PING;
	SONAR_TRIGGER_PORT |= 1<<SONAR_TRIGGER_BIT;
	TCNT1 = 0;
	sei();
	_delay_us(10);
	SONAR_TRIGGER_PORT &= ~(1<<SONAR_TRIGGER_BIT);
}

uint16_t sonar_last_pong(void) {
	return sonar_pong/58;
}

ISR(TIMER1_CAPT_vect) {
	if (sonar_state == SONAR_PING) {
		// reset timer
		TCNT1 = 0;
		// now we wait for the falling edge
		TCCR1B &= ~(1<<ICES1);
		sonar_state = SONAR_PONG;
	} else if (sonar_state == SONAR_PONG) {
		sonar_pong = ICR1;
		TCCR1B |= (1<<ICES1);
	}
}

ISR(TIMER1_OVF_vect) {
	static uint8_t overflows = 0;
	overflows++;
	if (overflows > 5) {
		if (sonar_state == SONAR_PING) {
			// we are still waiting for a reply? Impossible!
			sonar_pong = -1;
		}
		overflows = 0;
		sonar_state = SONAR_READY;
	}
	TCCR1B |= (1<<ICES1);
}
#endif
