/*
    smbus.h - SMBus level access helper functions

    Copyright (C) 1995-97 Simon G. Vogl
    Copyright (C) 1998-99 Frodo Looijaard <frodol@dds.nl>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA 02110-1301 USA.
*/

#ifndef LIB_I2C_SMBUS_H
#define LIB_I2C_SMBUS_H

extern signed int i2c_smbus_access(int file, char read_write, unsigned char command,
			      int size, union i2c_smbus_data *data);

extern signed int i2c_smbus_write_quick(int file, unsigned char value);
extern signed int i2c_smbus_read_byte(int file);
extern signed int i2c_smbus_write_byte(int file, unsigned char value);
extern signed int i2c_smbus_read_byte_data(int file, unsigned char command);
extern signed int i2c_smbus_write_byte_data(int file, unsigned char command, unsigned char value);
extern signed int i2c_smbus_read_word_data(int file, unsigned char command);
extern signed int i2c_smbus_write_word_data(int file, unsigned char command, unsigned short value);
extern signed int i2c_smbus_process_call(int file, unsigned char command, unsigned short value);

/* Returns the number of read bytes */
extern signed int i2c_smbus_read_block_data(int file, unsigned char command, unsigned char *values);
extern signed int i2c_smbus_write_block_data(int file, unsigned char command, unsigned char length,
					const unsigned char *values);

/* Returns the number of read bytes */
/* Until kernel 2.6.22, the length is hardcoded to 32 bytes. If you
   ask for less than 32 bytes, your code will only work with kernels
   2.6.23 and later. */
extern signed int i2c_smbus_read_i2c_block_data(int file, unsigned char command, unsigned char length,
					   unsigned char *values);
extern signed int i2c_smbus_write_i2c_block_data(int file, unsigned char command, unsigned char length,
					    const unsigned char *values);

/* Returns the number of read bytes */
extern signed int i2c_smbus_block_process_call(int file, unsigned char command, unsigned char length,
					  unsigned char *values);

#endif /* LIB_I2C_SMBUS_H */
