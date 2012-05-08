#include "config.h"
#if USE_GPS
/* NMEA parser */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "nmea.h"

#if __AVR__
#include <util/atomic.h>
#define ATOMIC(t) ATOMIC_BLOCK(t)
#else
#define ATOMIC(t)
#endif

/* these structs are used as a construction site
 * during the parsing process; once a sentence has
 * been received completely, we transfer the useful
 * data to the output struct
 */

static union {
#if PARSE_GPS_NMEA_RMC
	struct nmea_rmc_t rmc;
#endif
#if PARSE_GPS_NMEA_GGA
	struct nmea_gga_t gga;
#endif
} nmea_wip;

/* this is the data we will be offering */
static struct nmea_data_t *nmea_data = NULL;

static enum {
	GP_UNKNOWN,
	GP_RMC,
	GP_GGA,
} sentence = GP_UNKNOWN;

static enum {
	CS_UNKNOWN, /* no checksum is available */
	CS_CALC, /* calculating checksum */
	CS_READ, /* reading transmitted checksum */
	CS_VALID, /* transmitted checksum did not match the calculated one */
	CS_INVALID, /* the transmitted checksum did not match the calculated one */
} checksum_state = CS_UNKNOWN;

static uint8_t checksum = 0;

static uint8_t token_nr = 0;

#define TOKEN_BUFFER_SIZE 11

static char token_buffer[TOKEN_BUFFER_SIZE] = "";


static void parse_to_bcd(char *s, uint8_t *b, uint8_t max) {
	uint8_t i = 0;
	while (i<max && s[i]) {
		uint8_t n = (s[i]-'0');
		if (i%2 == 1) {
			b[i/2] |= (n << 4);
		} else {
			b[i/2] |= (0x0F & n);
		}
		i++;
	}
}

static void parse_coord(struct coord *co) {
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
	parse_to_bcd(ptr, co->frac, NMEA_MINUTE_FRACTS);
}

#if PARSE_GPS_ALTITUDE
static void parse_altitude(struct altitude_t *a) {
	/* find the decimal point */
	char *p = token_buffer;
	while (*p && *p != '.') p++;
	if (*p == '.') {
		/* we found the point */
		*p = '\0';
		/* now we BCD encode as many fractions
		 * as we can
		 */
		parse_to_bcd(p+1, a->frac, NMEA_ALTITUDE_FRACTS);
	}
	/* parse the integer part */
	a->m = atoi(token_buffer);

}
#endif

#if PARSE_GPS_TIME
static void parse_clock(struct clock_t *cl) {
	token_buffer[6] = '\0';
	cl->second = atoi(&token_buffer[4]);
	token_buffer[4] = '\0';
	cl->minute = atoi(&token_buffer[2]);
	token_buffer[2] = '\0';
	cl->hour = atoi(&token_buffer[0]);
}

static void parse_date(struct date_t *d) {
	token_buffer[6] = '\0';
	d->year = atoi(&token_buffer[4]);
	token_buffer[4] = '\0';
	d->month = atoi(&token_buffer[2]);
	token_buffer[2] = '\0';
	d->day = atoi(&token_buffer[0]);
}
#endif

#if PARSE_GPS_NMEA_RMC
static void process_gprmc_token(void) {
	switch (token_nr) {
#if !PARSE_GPS_NMEA_GGA /* avoid duplicate parsing code */
#if PARSE_GPS_TIME
		case 1:
			/* time
			 * HHMMSS(.sssss)
			 */
			parse_clock(&nmea_wip.rmc.clock);
			break;
#endif
		case 2:
			/* status
			 * A OK
			 * V Warning
			 */
			if (strcmp(token_buffer, "A") == 0) {
				nmea_wip.rmc.flags |= (1<<NMEA_RMC_FLAGS_STATUS_OK);
			} else {
				nmea_wip.rmc.flags &= ~(1<<NMEA_RMC_FLAGS_STATUS_OK);
			}
			break;
		case 3:
			/* latitude
			 * BBBB.BBBB
			 */
			parse_coord(&nmea_wip.rmc.lat);
			break;
		case 4:
			/* orientation
			 * N north
			 * S south
			 */
			if (strcmp(token_buffer, "N") == 0) {
				nmea_wip.rmc.flags |= (1<<NMEA_RMC_FLAGS_LAT_NORTH);
			} else {
				nmea_wip.rmc.flags &= ~(1<<NMEA_RMC_FLAGS_LAT_NORTH);
			}
			break;
		case 5:
			/* longitude
			 * LLLLL.LLLL
			 */
			parse_coord(&nmea_wip.rmc.lon);
			break;
		case 6:
			/* orientation
			 * E east
			 * W west
			 */
			if (strcmp(token_buffer, "E") == 0) {
				nmea_wip.rmc.flags |= (1<<NMEA_RMC_FLAGS_LON_EAST);
			} else {
				nmea_wip.rmc.flags &= ~(1<<NMEA_RMC_FLAGS_LON_EAST);
			}

			break;
#endif
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
#if PARSE_GPS_TIME
		case 9:
			/* date
			 * DDMMYY
			 */
			parse_date(&nmea_wip.rmc.date);
			break;
#endif
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
#endif

#if PARSE_GPS_NMEA_GGA
static void process_gpgga_token(void) {
	switch (token_nr) {
#if PARSE_GPS_TIME
		case 1:
			/* time
			 * HHMMSS(.sssss)
			 */
			parse_clock(&nmea_wip.gga.clock);
			break;
#endif
		case 2:
			/* latitude
			 * BBBB.BBBB
			 */
			parse_coord(&nmea_wip.gga.lat);
			break;
		case 3:
			/* orientation
			 * N north
			 * S south
			 */
			if (strcmp(token_buffer, "N") == 0) {
				nmea_wip.gga.flags |= (1<<NMEA_RMC_FLAGS_LAT_NORTH);
			} else {
				nmea_wip.gga.flags &= ~(1<<NMEA_RMC_FLAGS_LAT_NORTH);
			}
			break;
		case 4:
			/* longitude
			 * LLLLL.LLLL
			 */
			parse_coord(&nmea_wip.gga.lon);
			break;
		case 5:
			/* orientation
			 * E east
			 * W west
			 */
			if (strcmp(token_buffer, "E") == 0) {
				nmea_wip.gga.flags |= (1<<NMEA_RMC_FLAGS_LON_EAST);
			} else {
				nmea_wip.gga.flags &= ~(1<<NMEA_RMC_FLAGS_LON_EAST);
			}

			break;
		case 6:
			/* signal quality */
			nmea_wip.gga.quality = atoi(token_buffer);
			if (nmea_wip.gga.quality) {
				nmea_wip.gga.flags |= (1<<NMEA_RMC_FLAGS_STATUS_OK);
			} else {
				nmea_wip.gga.flags &= ~(1<<NMEA_RMC_FLAGS_STATUS_OK);
			}
			break;
		case 7:
			/* number of used satellites */
			nmea_wip.gga.sats = atoi(token_buffer);
			break;
#if PARSE_GPS_ALTITUDE
		case 9:
			/* altitude */
			parse_altitude(&nmea_wip.gga.alt);
			break;
#endif
		default:
			/* nothing to do */
			break;
	}
}
#endif

static void sentence_started(void) {
	/* a new sentence has started, we do not know which yet */
	sentence = GP_UNKNOWN;
	/* clear token buffer */
	token_buffer[0] = '\0';
	token_nr = 0;
}

static void sentence_finished(void) {
	/* the entire sentence has been read;
	 * now copy the constructed data to the ouput struct
	 * if the checksum matches.
	 */
	if (checksum_state == CS_INVALID) {
		return;
	}
	switch (sentence) {
#if PARSE_GPS_NMEA_RMC
		case GP_RMC:
			/* copy date, time and location */
#if PARSE_GPS_TIME
			memcpy(&nmea_data->date, &nmea_wip.rmc.date, sizeof(nmea_wip.rmc.date));
			memcpy(&nmea_data->clock, &nmea_wip.rmc.clock, sizeof(nmea_wip.rmc.clock));
#endif
#if !PARSE_GPS_NMEA_GGA /* avoid duplicate parsing code */
			nmea_data->flags = nmea_wip.rmc.flags;
			memcpy(&nmea_data->lon, &nmea_wip.rmc.lon, sizeof(nmea_wip.rmc.lon));
			memcpy(&nmea_data->lat, &nmea_wip.rmc.lat, sizeof(nmea_wip.rmc.lat));
#endif
			break;
#endif
#if PARSE_GPS_NMEA_GGA
		case GP_GGA:
#if PARSE_GPS_TIME
			memcpy(&nmea_data->clock, &nmea_wip.gga.clock, sizeof(nmea_wip.gga.clock));
#endif
			nmea_data->flags = nmea_wip.gga.flags;
			memcpy(&nmea_data->lon, &nmea_wip.gga.lon, sizeof(nmea_wip.gga.lon));
			memcpy(&nmea_data->lat, &nmea_wip.gga.lat, sizeof(nmea_wip.gga.lat));
			/* copy quality and number of satellites */
			nmea_data->quality = nmea_wip.gga.quality;
			nmea_data->sats = nmea_wip.gga.sats;
#if PARSE_GPS_ALTITUDE
			/* copy altitude */
			memcpy(&nmea_data->alt, &nmea_wip.gga.alt, sizeof(nmea_wip.gga.alt));
#endif
			break;
#endif
		default:
			break;
	}
}

static void gp_token_finished(void) {
	/* a token of the nmea sentence has been completed */
	switch (sentence) {
		case GP_UNKNOWN:
			if (token_nr == 0) {
				/* clear the building site */
				memset(&nmea_wip, 0, sizeof(nmea_wip));
				/* the first token defines the sentence type */
#if PARSE_GPS_NMEA_RMC
				if (strcmp(token_buffer, "GPRMC") == 0) {
					sentence = GP_RMC;
				} else
#endif
#if PARSE_GPS_NMEA_GGA
				if (strcmp(token_buffer, "GPGGA") == 0) {
					sentence = GP_GGA;
				}
#else
				{}
#endif
			}
			break;
#if PARSE_GPS_NMEA_RMC
		case GP_RMC:
			/* process data of the minimal data set */
			process_gprmc_token();
			break;
#endif
#if PARSE_GPS_NMEA_GGA
		case GP_GGA:
			process_gpgga_token();
			break;
#endif
		default:
			/* don't know what to do with it */
			break;
	}
	token_buffer[0] = '\0';
	token_nr++;
}

static uint8_t hex_digit(char c) {
	if (c >= '0' && c <= '9') {
		return (c - '0');
	} else if (c >= 'A' && c <= 'F') {
		return (10+(c - 'A'));
	} else if (c >= 'a' && c <= 'f') {
		return (10+(c - 'a'));
	} else {
		return 0;
	}
}

static uint8_t hex_to_short(char *s) {
	uint8_t r = 0;
	uint8_t i = 1;
	char *w = s;
	while (*w) w++; /* now *w == '\0' */
	while (w > s) {
		w--;
		r += i*hex_digit(*w);
		i *= 16;
	}
	return r;
}

static void token_finished(void) {
	/* a token has been completed, process the content in the buffer */
	if (checksum_state != CS_READ) {
		/* it was a normal token and not the checksum */
		gp_token_finished();
	} else {
		/* did we receive a checksum? */
		if (token_buffer[0] == '\0') {
			/* there is no checksum */
			checksum_state = CS_UNKNOWN;
		} else if ( hex_to_short(token_buffer) == checksum ) {
			/* the received checksum does match our calculated one */
			checksum_state = CS_VALID;
		} else {
			/* something strange happened */
			checksum_state = CS_INVALID;
		}
	}
}

static void append_to_token(const char c) {
	uint8_t l = strlen(token_buffer);
	if (l < (TOKEN_BUFFER_SIZE-1)) {
		token_buffer[l] = c;
		token_buffer[l+1] = '\0';
	}
}

static void add_to_checksum(const char c) {
	checksum ^= c;
}

void nmea_init(struct nmea_data_t *output) {
	nmea_data = output;
}

void nmea_process_character(char c) {
	switch (c) {
		case '$': /* a new sentence is starting */
			sentence_started();
			/* reset and enable checksum calculation */
			checksum = 0;
			checksum_state = CS_CALC;
			break;
		case ',':
			token_finished();
			break;
		case '*': /* checksum is following */
			token_finished();
			checksum_state = CS_READ;
			break;
		case '\r':
			/* \n is following soon, we ignore this */
			break;
		case '\n':
			token_finished();
			ATOMIC(ATOMIC_RESTORESTATE) {
				sentence_finished();
			}
			checksum_state = CS_UNKNOWN;
			break;
		default:
			append_to_token(c);
	}
	if (checksum_state == CS_CALC && c != '$') {
		add_to_checksum(c);
	}
}
#endif
