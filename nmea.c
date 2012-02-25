/* NMEA parser */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "nmea.h"

static struct nmea_rmc_t nmea_rmc = {0};
static struct nmea_gga_t nmea_gga = {0};

struct nmea_data_t nmea_data = {
	{0},
	{0},
};

static enum {
	GP_UNKNOWN,
	GP_RMC,
	GP_GGA,
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

static void parse_clock(struct clock_t *cl) {
	memset(cl, 0, sizeof(struct clock_t));
	token_buffer[6] = '\0';
	cl->second = atoi(&token_buffer[4]);
	token_buffer[4] = '\0';
	cl->minute = atoi(&token_buffer[2]);
	token_buffer[2] = '\0';
	cl->hour = atoi(&token_buffer[0]);
}

static void parse_date(struct date_t *d) {
	memset(d, 0, sizeof(struct date_t));
	token_buffer[6] = '\0';
	d->year = atoi(&token_buffer[4]);
	token_buffer[4] = '\0';
	d->month = atoi(&token_buffer[2]);
	token_buffer[2] = '\0';
	d->day = atoi(&token_buffer[0]);
}

static void process_gprmc_token(void) {
	switch (token_nr) {
		case 1:
			/* time
			 * HHMMSS(.sssss)
			 */
			parse_clock(&nmea_rmc.clock);
			break;
		case 2:
			/* status
			 * A OK
			 * V Warning
			 */
			if (strcmp(token_buffer, "A") == 0) {
				nmea_rmc.flags |= (1<<NMEA_RMC_FLAGS_STATUS_OK);
			} else {
				nmea_rmc.flags &= ~(1<<NMEA_RMC_FLAGS_STATUS_OK);
			}
			break;
		case 3:
			/* latitude
			 * BBBB.BBBB
			 */
			parse_coord(&nmea_rmc.lat);
			break;
		case 4:
			/* orientation
			 * N north
			 * S south
			 */
			if (strcmp(token_buffer, "N") == 0) {
				nmea_rmc.flags |= (1<<NMEA_RMC_FLAGS_LAT_NORTH);
			} else {
				nmea_rmc.flags &= ~(1<<NMEA_RMC_FLAGS_LAT_NORTH);
			}
			break;
		case 5:
			/* longitude
			 * LLLLL.LLLL
			 */
			parse_coord(&nmea_rmc.lon);
			break;
		case 6:
			/* orientation
			 * E east
			 * W west
			 */
			if (strcmp(token_buffer, "E") == 0) {
				nmea_rmc.flags |= (1<<NMEA_RMC_FLAGS_LON_EAST);
			} else {
				nmea_rmc.flags &= ~(1<<NMEA_RMC_FLAGS_LON_EAST);
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
			parse_date(&nmea_rmc.date);
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

static void process_gpgga_token(void) {
	switch (token_nr) {
		case 6:
			/* signal quality */
			nmea_gga.quality = atoi(token_buffer);
			break;
		case 7:
			/* number of used satellites */
			nmea_gga.sats = atoi(token_buffer);
			break;
		default:
			/* nothing to do */
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
	/* the entire sentence has been read;
	 * now copy the constructed data to the ouput struct
	 */
	switch (sentence) {
		case GP_RMC:
			memcpy(&nmea_data.rmc, &nmea_rmc, sizeof(nmea_rmc));
			break;
		case GP_GGA:
			memcpy(&nmea_data.gga, &nmea_gga, sizeof(nmea_gga));
			break;
		default:
			break;
	}
}

static void token_finished(void) {
	/* a token has been completed, process the content in the buffer */
	switch (sentence) {
		case GP_UNKNOWN:
			if (token_nr == 0) {
				/* the first token defines the sentence type */
				if (strcmp(token_buffer, "GPRMC") == 0) {
					sentence = GP_RMC;
				} else if (strcmp(token_buffer, "GPGGA") == 0) {
					sentence = GP_GGA;
				}
			}
			break;
		case GP_RMC:
			/* process data of the minimal data set */
			process_gprmc_token();
			break;
		case GP_GGA:
			process_gpgga_token();
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
