#ifndef SPI_H
#define	SPI_H

#include <p33FJ64MC802.h>
#include <stdio.h>
#include <stdlib.h>

void write_register(unsigned char addr, unsigned char data);
unsigned short read_register(unsigned char addr);

#endif	/* SPI_H */

