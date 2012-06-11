MCU = attiny2313
F_CPU = 8000000
TARGET = tiny-gps
SRC = tiny-gps.c nmea.c sonar.c optical.c usiTwiSlave.c
COMBINE_SRC = 0

include avr-tmpl.mk
