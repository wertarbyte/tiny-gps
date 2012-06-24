#include "config.h"
#if USE_SONAR
/* sonar handler */
#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include "sonar.h"

#define SONAR_TRIGGER_PORT PORTD
#define SONAR_TRIGGER_DDR DDRD
#define SONAR_TRIGGER_BIT PD2

#if __AVR__
#include <util/atomic.h>
#define ATOMIC(t) ATOMIC_BLOCK(t)
#else
#define ATOMIC(t)
#endif

static uint8_t sonar_pong_i = 0;
static volatile int16_t sonar_pong[SONAR_AVG_WINDOW_SIZE] = {-1};
#if SONAR_AVG_WINDOW_SIZE <= 0
	#error "SONAR_AVG_WINDOW_SIZE has to be >= 1"
#endif

static volatile enum {
	SONAR_READY,
	SONAR_PING,
	SONAR_PONG,
} sonar_state = SONAR_READY;

void sonar_init(void) {
#if SONAR_AVG_WINDOW_SIZE > 1
	memset((int16_t *)sonar_pong, ~0, sizeof(sonar_pong));
#endif
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
	ATOMIC(ATOMIC_FORCEON) {
		sonar_state = SONAR_PING;
		SONAR_TRIGGER_PORT |= 1<<SONAR_TRIGGER_BIT;
		TCNT1 = 0;
	}
	_delay_us(10);
	SONAR_TRIGGER_PORT &= ~(1<<SONAR_TRIGGER_BIT);
}

static int16_t sonar_avg_pong(void) {
#if SONAR_AVG_WINDOW_SIZE > 1
	uint8_t invalid = 0;
	uint16_t sum = 0;
	for (uint8_t i=0; i<SONAR_AVG_WINDOW_SIZE; i++) {
		int16_t v;
		ATOMIC(ATOMIC_FORCEON) {
			v = sonar_pong[i];
		}
		if (v<0) {
			invalid++;
		} else {
			sum += v;
		}
	}
	if (invalid > SONAR_AVG_WINDOW_SIZE/2) {
		return -1;
	} else {
		return sum/SONAR_AVG_WINDOW_SIZE;
	}
#else
	return sonar_pong[0];
#endif
}

int16_t sonar_last_pong(void) {
	int16_t pong = sonar_avg_pong();
	if (pong < 0) return -1;
#if SLOPPY_SONAR_CONVERSION
	return pong>>6;
#else
	return pong/58;
#endif
}

ISR(TIMER1_CAPT_vect) {
	if (sonar_state == SONAR_PING) {
		// reset timer
		TCNT1 = 0;
		// now we wait for the falling edge
		sonar_state = SONAR_PONG;
	} else if (sonar_state == SONAR_PONG) {
		sonar_pong[sonar_pong_i] = ICR1;
#if SONAR_AVG_WINDOW_SIZE > 1
		sonar_pong_i++;
		if (sonar_pong_i == SONAR_AVG_WINDOW_SIZE) sonar_pong_i = 0;
#endif
	}
	TCCR1B ^= (1<<ICES1);
}

ISR(TIMER1_OVF_vect) {
	if (sonar_state == SONAR_PING) {
		// we are still waiting for a reply? Impossible!
		sonar_pong[sonar_pong_i] = -1;
#if SONAR_AVG_WINDOW_SIZE > 1
		sonar_pong_i++;
		if (sonar_pong_i == SONAR_AVG_WINDOW_SIZE) sonar_pong_i = 0;
#endif
	}
	sonar_state = SONAR_READY;
	TCCR1B |= (1<<ICES1);
}
#endif
