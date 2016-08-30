#include "init.h"

void init_uart1() {

    //must set to digital
    AD1PCFGLbits.PCFG4 = 1;
    AD1PCFGLbits.PCFG5 = 1;

    //Page 181 of dsPIC33FJ datasheet. This ties RP2 to UART1 RX
    RPINR18bits.U1RXR = 2;
    TRISBbits.TRISB2 = 1;

    // Page 189 of dsPIC33FJ datasheet. This ties RP3 to UART1 TX
    //Table 11-2 lists the decoded values for this register pg 167
    RPOR1bits.RP3R = 3;

    U1MODEbits.STSEL = 0; //1 stop bit
    U1MODEbits.PDSEL = 0; //8 bit data, no parity
    U1MODEbits.ABAUD = 0; //auto-baud disabled
    U1MODEbits.BRGH = 0; //standard speed mode

    //check ref manual uart section for calculation
    if (BAUDRATE == 115200)
        U1BRG = 20;
    else if (BAUDRATE == 9600)
        U1BRG = 256;
    else
        U1BRG = 0xFF;

    //not using interrupts for transmit, polling instead
    //U1STAbits.UTXISEL0 = 0;
    //U1STAbits.UTXISEL1 = 0;
    //IEC0bits.U1TXIE = 1;

    //interrupt after one character is rcvd
    U1STAbits.URXISEL = 0;
    
    //clear flag then enable interrupts
    IFS0bits.U1RXIF = 0;
    IEC0bits.U1RXIE = 1;

    U1MODEbits.UARTEN = 1; //enable uart
    U1STAbits.UTXEN = 1; //transmitter enabled

    //IFS0bits.U1TXIF = 0;

}

void init_clock() {
    /* Configure Oscillator to operate the device at 40Mhz
       Fosc= Fin*M/(N1*N2), Fcy=Fosc/2
       Fosc= 7.37*(43)/(2*2)=80Mhz for Fosc, Fcy = 40Mhz */
    PLLFBD = 41; // M = 43
    CLKDIVbits.PLLPOST = 0; // N1 = 2
    CLKDIVbits.PLLPRE = 0; // N2 = 2
    OSCTUN = 0;
    RCONbits.SWDTEN = 0;

    // Clock switch to incorporate PLL
    __builtin_write_OSCCONH(0x01); // Initiate Clock Switch to
    // FRC with PLL (NOSC=0b001)
    __builtin_write_OSCCONL(0x01); // Start clock switching
    while (OSCCONbits.COSC != 0b001); // Wait for Clock switch to occur

    while (OSCCONbits.LOCK != 1) {
    };
}

void init_timer1()
{
	// Clear Timer value (i.e. current tiemr value) to 0
	TMR1 = 0;
        
	T1CONbits.TCS = 0; //source is Fcy
	T1CONbits.TCKPS = 3; //1:256; period = 6.4us
        //Set PR1 to 250ms
	PR1 = 39062;

	// Clear Timer 1 interrupt flag. This allows us to detect the
	// first interupt.
	IFS0bits.T1IF = 0;

	// Enable the interrupt for Timer 1
	IEC0bits.T1IE = 1;
}


void init_spi() {

    //configuring the PPS
    //SDO on RP9
    RPOR4bits.RP9R = 7;
    //SDI on RP8
    RPINR20bits.SDI1R = 8;
    //SCLK on RP7
    //ref man states you have to configure input/output
    RPOR3bits.RP7R = 8;
    RPINR20bits.SCK1R = 7;

    //configuring SS on RB6
    TRISBbits.TRISB6 = 0; //output only
    LATBbits.LATB6 = 1; //initialize to high (idle on Si4430)

     /* The following code sequence shows SPI register configuration for Master mode */
    IFS0bits.SPI1IF = 0;                // Clear the Interrupt flag
    IEC0bits.SPI1IE = 0;                // Disable the interrupt

    // SPI1CON1 Register Settings
    SPI1CON1bits.DISSCK = 0;            // Internal serial clock is enabled
    SPI1CON1bits.DISSDO = 0;            // SDOx pin is controlled by the module
    SPI1CON1bits.MODE16 = 1;            // Communication is word-wide (16 bits)
    SPI1CON1bits.MSTEN = 1;             // Master mode enabled
    SPI1CON1bits.SMP = 0;               // Input data is sampled at the middle of data output time
    SPI1CON1bits.CKE = 1;               // Serial output data changes on transition from
                                        // active clock state to Idle clock state
    SPI1CON1bits.SPRE = 6;              // secondary presclaer 2:1
    SPI1CON1bits.PPRE = 2;              // primary prescaler 4:1
    SPI1CON1bits.SSEN = 0;              // in master need to manually control this
    //SCLK = Fcy / (secondary * primary) = 40MHz / 8 = 5MHz

    SPI1CON1bits.CKP = 0;             // Idle state for clock is a low level;

    // active state is a high level
    SPI1STATbits.SPIEN = 1;           // Enable SPI module

}