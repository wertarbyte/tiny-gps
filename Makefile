MCU = attiny2313 -U lfuse:w:0xe4:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
F_CPU = 8000000
TARGET = tiny-gps
SRC = tiny-gps.c nmea.c sonar.c optical.c usiTwiSlave.c
COMBINE_SRC = 0

include avr-tmpl.mk
