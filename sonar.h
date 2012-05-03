#include "sonar_structs.h"

void sonar_init(void);
uint8_t sonar_ready(void);
void sonar_ping(void);
uint16_t sonar_last_pong(void);
