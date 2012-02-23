/* NMEA parser */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "nmea.h"

static enum {
	GP_UNKNOWN,
	GP_GPRMC,
} sentence = GP_UNKNOWN;

static uint8_t token_nr = 0;

#define TOKEN_BUFFER_SIZE 16

static char token_buffer[TOKEN_BUFFER_SIZE] = "";

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
			break;
		case 3:
			/* latitude
			 * BBBB.BBBB
			 */
			break;
		case 4:
			/* orientation
			 * N north
			 * S south
			 */
			break;
		case 5:
			/* longitude
			 * LLLLL.LLLL
			 */
			break;
		case 6:
			/* orientation
			 * E east
			 * W west
			 */
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
