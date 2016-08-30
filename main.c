//main.c
#include "init.h"
#include "uart.h"
#include "spi.h"
#include <math.h>

// ******************************************************************************************* //
// Configuration bits for CONFIG1 settings. 
//
// Make sure "Configuration Bits set in code." option is checked in MPLAB.
// This option can be set by selecting "Configuration Bits..." under the Configure
// menu in MPLAB.

_FOSC(OSCIOFNC_ON & FCKSM_CSDCMD & POSCMD_NONE); //Oscillator Configuration (clock switching: disabled;
// failsafe clock monitor: disabled; OSC2 pin function: digital IO;
// primary oscillator mode: disabled)
_FOSCSEL(FNOSC_FRCPLL); //Oscillator Selection PLL
_FWDT(FWDTEN_OFF); //Turn off WatchDog Timer
_FGS(GCP_OFF); //Turn off code protect
_FPOR(FPWRT_PWR1); //Turn off power up timer

//Local function to initialize the si4432 chip
void init_si4432();

//flag used to signal when it's been 3 seconds and time to transmit
volatile unsigned char transmit_flag;
//Helper counter because Timer1 can't reach 3seconds with the current osc
//setting. So Timer1 is set to 250ms timout and timer_counter will be used to 
//count 12 interrupts before asserting transmit_flag
volatile int timer_counter;

//Message to transmit
volatile char message[8] = "AhFirst";
volatile char length = 8;


volatile unsigned short int_status1, int_status2;

int main() {

    //Local loop variable
    int i;

    transmit_flag = 0;
    timer_counter = 0;

    //Using RA4 as nIRQ (input)
    TRISAbits.TRISA4 = 1;

    //initializations
    init_clock();
    init_spi();
    init_timer1();
    init_si4432();

    T1CONbits.TON = 1;

    while (1) {

        if (transmit_flag == 1) {

            transmit_flag = 0;

            //Write the length of the message to be sent
            write_register(0x3E, length);

            //Fill up the TX FIFO
            for (i = 0; i < length; i++) {
                write_register(0x7F, message[i]);
            }

            //disable all other interrupts and enable packet sent interrupt
            write_register(0x05, 0x04);
            write_register(0x06, 0x00);

            //read interrupt status regs to clear
            int_status1 = read_register(0x03);
            int_status2 = read_register(0x04);

            //enable transmitter
            write_register(0x07, 0x09);

            //wait for packet to be sent. The ipksent interrupt will trigger
            //the nIRQ line to be brought low
            while (PORTAbits.RA4 == 1);

            //clear interrupt status regs
            int_status1 = read_register(0x03);
            int_status2 = read_register(0x04);

            //enable two interrupts
            //one for valid message 'ipkval'
            //one for invalid CRC error 'icrcerror'
            write_register(0x05, 0x03);
            write_register(0x06, 0x00);

            //clear interrupt status regs
            int_status1 = read_register(0x03);
            int_status2 = read_register(0x04);

            //turn on rcv module
            write_register(0x07, 0x05);

            //wait for something to arrive. nIRQ will be brought low for
            //CRC error or valid message
            while (PORTAbits.RA4 == 1);

            //read interrupt status regs
            int_status1 = read_register(0x03);
            int_status2 = read_register(0x04);

            //Check if there was a CRC error
            if ((int_status1 & 0x01) == 0x01) {

                //Disable rcv module
                write_register(0x07, 0x01);

                //Clear the rcv FIFO
                write_register(0x08, 0x02);
                write_register(0x08, 0x00);

                //Reply with an error message
                message[0] = 'c';
                message[1] = 'r';
                message[2] = 'c';
                message[3] = 'e';
                message[4] = 'r';
                message[5] = 'r';
                message[6] = 'r';
                message[7] = '\0';

            }

            //check if message is valid
            if ((int_status1 & 0x02) == 0x02) {

                write_register(0x07, 0x01);

                //Read the length of the message arriving
                length = read_register(0x4B);

                //If message is short enough, replace the message to be transmitted
                //next frame
                if (length <= 8) {

                    for (i = 0; i < length; i++) {
                        message[i] = read_register(0x7F);
                    }
                }

                //Clear rcv FIFO
                write_register(0x08, 0x02);
                write_register(0x08, 0x00);


            }

        }

    }

    return 1;
}

void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void) {

    // Clear Timer 1 interrupt flag to allow another Timer 1 interrupt to occur.
    IFS0bits.T1IF = 0;
    timer_counter++;
    if (timer_counter == 12) {
        transmit_flag = 1;
        timer_counter = 0;
    }
}

void init_si4432() {

    //clear interrupt status regs
    int_status1 = read_register(0x03);
    int_status2 = read_register(0x04);

    //Setting the si4432 to the following settings:
    //Modulation: GFSK
    //Manchester: Off
    //Carrier freq: 433.92MHz
    //Bit rate: 9.6 kbps
    //AFC: Enabled
    //Freq. Deviation: 45kHz
    //Packet hander and FIFO mode
    //CRC16-IBM, No header, Variable packet length, 10 nibble preamble
    //Setting registers to what's prescribed in the excel doc
    write_register(0x1C, 0x05);
    write_register(0x1D, 0x3C);
    write_register(0x1E, 0x02);
    write_register(0x1F, 0x03);
    write_register(0x20, 0xA1);
    write_register(0x21, 0x20);
    write_register(0x22, 0x4E);
    write_register(0x23, 0xA5);
    write_register(0x24, 0x00);
    write_register(0x25, 0x24);
    write_register(0x2A, 0xFF);
    write_register(0x2C, 0x28);
    write_register(0x2D, 0x82);
    write_register(0x2E, 0x29);

    write_register(0x30, 0xAD);
    write_register(0x32, 0x8C);
    write_register(0x33, 0x02);
    write_register(0x34, 0x0A);

    write_register(0x58, 0x80);
    write_register(0x69, 0x60);
    write_register(0x6E, 0x4E);
    write_register(0x6F, 0xA5);

    write_register(0x70, 0x2C);
    write_register(0x71, 0x23);
    write_register(0x72, 0x48);

    write_register(0x75, 0x53);
    write_register(0x76, 0x62);
    write_register(0x77, 0x00);

}
