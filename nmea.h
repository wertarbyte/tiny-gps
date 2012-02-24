#define NMEA_MINUTE_FRACTS 4

struct coord {
	/* degrees, 0-180 or 0-90 */
	uint8_t deg;
	/* minutes, 0-60 */
	uint8_t min;
	/* fractions of minutes saved as BCD */
	uint8_t frac[(NMEA_MINUTE_FRACTS+1)/2];
};

struct nmea_rmc_t {
	/* flag bits (lsb to msb):
	 * 0 status (1 == OK, 0 == warning)
	 * 1 latitude alignment (1 == N, 0 == S)
	 * 2 longitude alignment (1 == E, 0 == W)
	 */
	uint8_t flags;
	struct coord lat;
	struct coord lon;
};

struct nmea_gga_t {
	uint8_t quality;
	uint8_t sats;
};

struct nmea_data_t {
	struct nmea_rmc_t rmc;
	struct nmea_gga_t gga;
};

void nmea_process_character(char c);

