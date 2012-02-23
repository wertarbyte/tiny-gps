#define NMEA_MINUTE_FRACTS 4

struct coord {
	/* degrees, 0-180 or 0-90 */
	uint8_t deg;
	/* minutes, 0-60 */
	uint8_t min;
	/* fractions of minutes saved as BCD */
	uint8_t frac[(NMEA_MINUTE_FRACTS+1)/2];
};

void nmea_process_character(char c);

