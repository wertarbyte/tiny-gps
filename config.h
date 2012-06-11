/* 7 bit address on the TWI/I²C bus */
#define TWIADDRESS 0x11

/* query a serial GPS? */
#define USE_GPS 1

/* baud rate of the serial GPS receiver */
#define GPS_BAUD 38400

/* send configuration string to the GPS unit to change baud and/or update rate?
 *
 * Please note that not all baud rates are possible depending on your
 * clock frequency; pay attention to compiler warnings!
 */
//#define GPS_INIT_STRING "$PMTK300,200,0,0,0,0*2F\r\n$PMTK251,115200*1F\r\n"

/* wait a few moments (in ms) before sending the init string? */
//#define GPS_INIT_DELAY 250

/* use a different baud rate for sending the init string? */
//#define GPS_INIT_BAUD 9600

/* query additional sonar device?
 *
 * The sonar trigger must be connected to PD5, the echo wire to the ICP pin.
 */
#define USE_SONAR 1
/* query optical flow sensor?
 *
 * The optical flow sensor (A5050) must be connected to PA0 (SCK), PA1 (SDIO)
 * and PB6 (CSEL/NCS) pins.
 */
#define USE_OPTICAL 1

/* extract date/time information from the GPS signal?
 *
 * Disabling this can reduce flash footprint for smaller controllers like the
 * ATTiny2313.
 */
#define PARSE_GPS_TIME 1

/* extract altitude information from GPS signal?
 *
 * Disabling this can reduce flash footprint for smaller controllers like the
 * ATTiny2313.
 */
#define PARSE_GPS_ALTITUDE 1

/* only parse specific sentences?
 *
 * GGA does not carry date information
 * RMC does not carry altitude/quality information
 */
#define PARSE_GPS_NMEA_GGA 1
#define PARSE_GPS_NMEA_RMC 1

/* sloppy sonar distance conversion?
 *
 * When enabled, the echo time of the sonar pulse will be divided by 64 instead
 * of 58, reducing the code size, but also introducing a 10% error in absolute
 * distance measurement (cm). If you are only interested in relative distances
 * regardless of the unit of measurement and running low on flash memory, you
 * can safely enable this.
 */
#define SLOPPY_SONAR_CONVERSION 0
