/* NMEA parser */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "nmea.h"

#define NMEA_FLAG_STATUS 0
#define NMEA_FLAG_LAT_N 1
#define NMEA_FLAG_LON_E 2

struct {
	/* flag bits (lsb to msb):
	 * 0 status (1 == OK, 0 == warning)
	 * 1 latitude alignment (1 == N, 0 == S)
	 * 2 longitude alignment (1 == E, 0 == W)
	 */
	uint8_t flags;
	struct coord lat;
	struct coord lon;
} nmea_pos = {0};

static enum {
	GP_UNKNOWN,
	GP_GPRMC,
} sentence = GP_UNKNOWN;

static uint8_t token_nr = 0;

#define TOKEN_BUFFER_SIZE 16

static char token_buffer[TOKEN_BUFFER_SIZE] = "";

static void parse_coord(struct coord *co) {
	/* clear the struct */
	memset(co, 0, sizeof(struct coord));
	/* find the decimal point */
	char *ptr = token_buffer;
	while (*ptr && *ptr != '.') ptr++;
	if (*ptr == '.') {
		/* we found the point */
		*ptr = '\0';
		ptr++;
	} else {
		/* something weird happened,
		 * there is no decimal point?!
		 */
		return;
	}
	/* two digits before the decimal point
	 * are the minutes
	 */
	co->min = atoi(ptr-3);
	*(ptr-3) = '\0';
	co->deg = atoi(token_buffer);

	/* now we BCD encode as many fractions
	 * of a minute as we can
	 */
	uint8_t i = 0;
	while (i<NMEA_MINUTE_FRACTS && ptr[i]) {
		uint8_t n = (ptr[i]-'0');
		if (i%2 == 0) {
			co->frac[i/2] |= (n << 4);
		} else {
			co->frac[i/2] |= (0x0F & n);
		}
		i++;
	}
}

static void process_gprmc_token(void) {
	switch (token_nr) {
		case 1:
			/* time
			 * HHMMSS(.sssss)
			 */
			break;
		case 2:
			/* status
			 * A OK
			 * V Warning
			 */
			if (strcmp(token_buffer, "A") == 0) {
				nmea_pos.flags |= (1<<NMEA_FLAG_STATUS);
			} else {
				nmea_pos.flags &= ~(1<<NMEA_FLAG_STATUS);
			}
			break;
		case 3:
			/* latitude
			 * BBBB.BBBB
			 */
			parse_coord(&nmea_pos.lat);
			break;
		case 4:
			/* orientation
			 * N north
			 * S south
			 */
			if (strcmp(token_buffer, "N") == 0) {
				nmea_pos.flags |= (1<<NMEA_FLAG_LAT_N);
			} else {
				nmea_pos.flags &= ~(1<<NMEA_FLAG_LAT_N);
			}
			break;
		case 5:
			/* longitude
			 * LLLLL.LLLL
			 */
			parse_coord(&nmea_pos.lon);
			break;
		case 6:
			/* orientation
			 * E east
			 * W west
			 */
			if (strcmp(token_buffer, "E") == 0) {
				nmea_pos.flags |= (1<<NMEA_FLAG_LON_E);
			} else {
				nmea_pos.flags &= ~(1<<NMEA_FLAG_LON_E);
			}

			break;
		case 7:
			/* speed
			 * GG.G
			 */
			break;
		case 8:
			/* course
			 * RR.R
			 */
			break;
		case 9:
			/* date
			 * DDMMYY
			 */
			break;
		case 10:
			/* magnetic declination
			 * M.M
			 */
			break;
		case 11:
			/* sign of declination
			 * E east
			 * W west
			 */
			break;
		case 12:
			/* signal integrity
			 * A autonomous mode
			 * D differential mode
			 * E estimated mode
			 * M manual input mode
			 * S simulated mode
			 * N data not valid
			 */
			break;
	}
}

static void sentence_started(void) {
	/* a new sentence has started, we do not know which yet */
	sentence = GP_UNKNOWN;
	/* clear token buffer */
	token_buffer[0] = '\0';
	token_nr = 0;
}

static void sentence_finished(void) {
	/* the entire sentence has been read */
}

static void token_finished(void) {
	/* a token has been completed, process the content in the buffer */
	switch (sentence) {
		case GP_UNKNOWN:
			if (token_nr == 0) {
				/* the first token defines the sentence type */
				if (strcmp(token_buffer, "GPRMC") == 0) {
					sentence = GP_GPRMC;
				}
			}
			break;
		case GP_GPRMC:
			/* process data of the minimal data set */
			process_gprmc_token();
			break;
		default:
			/* don't know what to do with it */
			break;
	}
	token_buffer[0] = '\0';
	token_nr++;
}

static void append_to_token(const char c) {
	uint8_t l = strlen(token_buffer);
	if (l < (TOKEN_BUFFER_SIZE-1)) {
		token_buffer[l] = c;
		token_buffer[l+1] = '\0';
	}
}

void nmea_process_character(char c) {
	switch (c) {
		case '$': /* a new sentence is starting */
			sentence_started();
			break;
		case ',':
		case '*': /* the checksum is following, but we ignore it */
			token_finished();
			break;
		case '\n':
			token_finished();
			sentence_finished();
			break;
		default:
			append_to_token(c);
	}
}
