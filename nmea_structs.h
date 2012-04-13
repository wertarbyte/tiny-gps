#define NMEA_MINUTE_FRACTS 4
#define NMEA_ALTITUDE_FRACTS 2

#define NMEA_RMC_FLAGS_STATUS_OK 0
#define NMEA_RMC_FLAGS_LAT_NORTH 1
#define NMEA_RMC_FLAGS_LON_EAST 2

struct coord {
	/* degrees, 0-180 or 0-90 */
	uint8_t deg;
	/* minutes, 0-60 */
	uint8_t min;
	/* fractions of minutes saved as BCD */
	uint8_t frac[(NMEA_MINUTE_FRACTS+1)/2];
};

struct altitude_t {
	int16_t m;
	uint8_t frac[(NMEA_ALTITUDE_FRACTS+1)/2];
};

struct clock_t {
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

struct date_t {
	uint8_t day;
	uint8_t month;
	uint8_t year;
};

struct nmea_data_t {
	uint8_t flags;
	struct date_t date;
	struct clock_t clock;
	struct coord lat;
	struct coord lon;
	struct altitude_t alt;
	uint8_t quality;
	uint8_t sats;
};
