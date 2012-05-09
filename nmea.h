#include "nmea_structs.h"

struct nmea_rmc_t {
	/* flag bits (lsb to msb):
	 * 0 status (1 == OK, 0 == warning)
	 * 1 latitude alignment (1 == N, 0 == S)
	 * 2 longitude alignment (1 == E, 0 == W)
	 */
	uint8_t flags;
#if PARSE_GPS_TIME
	struct date_t date;
	struct clock_t clock;
#endif
	struct coord lat;
	struct coord lon;
};

struct nmea_gga_t {
	uint8_t flags;
#if PARSE_GPS_TIME
	struct clock_t clock;
#endif
	struct coord lat;
	struct coord lon;
#if PARSE_GPS_ALTITUDE
	struct altitude_t alt;
#endif
	uint8_t quality;
	uint8_t sats;
};

void nmea_init(struct nmea_data_t *output);
void nmea_process_character(char c);

