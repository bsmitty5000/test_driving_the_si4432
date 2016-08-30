#include "spi.h"

void write_register(unsigned char addr, unsigned char data) {

    unsigned short spi_data_combined;
    LATBbits.LATB6 = 0; //bring SS low to start SPI transaction
    //spi_data_combined = (data << 8) + (addr << 1) + 1;
    spi_data_combined = (1 << 15) + (addr << 8) + data;

    SPI1BUF = spi_data_combined;

    //poll the interrupt flag, indicates a complete transfer
    while(IFS0bits.SPI1IF == 0);
    IFS0bits.SPI1IF = 0;

    LATBbits.LATB6 = 1; //bring SS high for idle
    spi_data_combined = SPI1BUF; //read to clear SPIRBF for next transaction


}

unsigned short read_register(unsigned char addr) {

    unsigned short spi_data_combined = 0;
    LATBbits.LATB6 = 0; //bring SS low to start SPI transaction
    //spi_data_combined = (addr << 1);
    spi_data_combined = (addr << 8);

    SPI1BUF = spi_data_combined;

    //poll the interrupt flag, indicates a complete transfer
    while(IFS0bits.SPI1IF == 0);
    IFS0bits.SPI1IF = 0;

    LATBbits.LATB6 = 1; //bring SS high for idle
    spi_data_combined = SPI1BUF; //read SPIRBF
    return spi_data_combined;
}
