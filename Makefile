MCU = attiny2313
F_CPU = 1000000
TARGET = tinygps
SRC = tinygps.c nmea.c usiTwiSlave.c
COMBINE_SRC = 1

include avr-tmpl.mk
