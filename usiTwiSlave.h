/********************************************************************************

Header file for the USI TWI Slave driver.

Created by Donald R. Blake
donblake at worldnet.att.net

---------------------------------------------------------------------------------

Created from Atmel source files for Application Note AVR312: Using the USI Module
as an I2C slave.

This program is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

---------------------------------------------------------------------------------

Change Activity:

    Date       Description
   ------      -------------
  15 Mar 2007  Created.

********************************************************************************/



#ifndef _USI_TWI_SLAVE_H_
#define _USI_TWI_SLAVE_H_



/********************************************************************************

                                    includes

********************************************************************************/

#include <stdbool.h>



/********************************************************************************

                                   prototypes

********************************************************************************/

void    usiTwiSlaveInit( uint8_t );
void    usiTwiSlaveSetTrap( void (*trap)(void));
void    usiTwiSetTransmitWindow( void*, size_t );

#endif  // ifndef _USI_TWI_SLAVE_H_
