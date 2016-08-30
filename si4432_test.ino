/*

Test program to interface with si4432

This program will initially wait for a message
Once a valid message arrives it sends an ack
The ack string is constantly changing

*/
 
#include <SPI.h>

//Pin definitions
//Interrupt pin from the si4432
const int nIRQ_pin = 4;
//Slave select for SPI
const int SS_pin = 17;

//Used when reading the two interrupt status registers on the
//si4432
int int_status1 = 0;
int int_status2 = 0;

//string that's sent as an ack. The last character will be incremented
//each time a new message arrives
const int ack_len = 8;
char ack_mess[ack_len] = "ackmsgA";

//String to hold the incoming message
const int buff_max = 10;
char message_buffer[buff_max];

//variable to hold lenght of incoming message
int buff_len = 0;

//loop variable
int i;

//timer to help with the dots. Just something to help make sure
//the program didn't freeze while waiting for a message
unsigned long beep_beep_timer;

void setup() {                

  Serial.begin(115200);
  
  //using an old version of SPI
  //Setting SPI clock to 1MHz because of the apparent
  //limitations of the logic converter
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  
  //initialize SS
  pinMode(SS_pin, OUTPUT);
  digitalWrite(SS_pin, HIGH);
  
  //si4430 initialization
  //si4430 needs 15ms to boot up
  delay(15); 
  
  //clear the nIRQ pin
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
  //Setting registers to what's prescribed in the excel
  //doc
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
  
  //Default for Arduino is rcv mode
  //Write to Op Mode & Func Control 1 to set
  //rcv mode and Xtal on
  write_register(0x07, 0x05);
  
  //enable two interrupts
  //one for valid message 'ipkval'
  //one for invalid CRC error 'icrcerror'
  write_register(0x05, 0x03);
  write_register(0x06, 0x00);
  
  //clear interrupt status regs
  int_status1 = read_register(0x03);
  int_status2 = read_register(0x04);
  
  //Writing a dot to the screen every second
  //to make sure the Arduino hasn't stalled somewhere
  beep_beep_timer = millis() + 1000;

}

void loop() {

  //Poll the nIRQ pin each loop
  //When a message has arrived the interrupt
  //will be set and the pin brought low
  if (digitalRead(nIRQ_pin) == LOW) {
   
    //Extra line because of the dot printing
    Serial.println();
    Serial.println("Got something");
    
    //read interrupt status regs
    int_status1 = read_register(0x03);
    int_status2 = read_register(0x04);
    
    //Checking if there was a CRC error
    if((int_status1 & 0x01) == 0x01) {
      
      //disable rcv chain
      write_register(0x07, 0x01);
       
      write_register(0x08, 0x02);
      write_register(0x08, 0x00);
       
      Serial.println("CRC Error!!");
       
      write_register(0x07, 0x05);
    }
    
    //Checking if the message is without errors
    if((int_status1 & 0x02) == 0x02) {
     
      //disable rcv chain
      write_register(0x07, 0x01);
    
      //Read length of the message
      buff_len = read_register(0x4B);
     
      //Safeguard to make sure there's enough room in the buffer
      if(buff_len <= buff_max) {
       
        for(i = 0; i < buff_len; i++) {
         
         message_buffer[i] = read_register(0x7F);
          
        }
       
      }
      
      Serial.print("Message rcvd: ");
      Serial.println(message_buffer);
      
      //reset rcv FIFO
      write_register(0x08, 0x02);
      write_register(0x08, 0x00);
      
      //sending the ACK
      //write length of the ack to be sent, 7 bytes
      write_register(0x3E, ack_len);
      
      //write the ack message to the TX FIFO
      for(i = 0; i < ack_len; i++) {
       
       write_register(0x7F, ack_mess[i]);
        
      }
      
      //changing up the ack_message
      //Doing this just to make sure everything is sending/rcving OK
      ack_mess[6]++;
      if(ack_mess[6] == 'z')
        ack_mess[6] = 'A';
      
      //disable all other interrupts
      //Except ipksent
      write_register(0x05, 0x04);
      write_register(0x06, 0x00);

      //read interrupt status regs to clear anything that
      //might be there
      int_status1 = read_register(0x03);
      int_status2 = read_register(0x04);
      
      //enable transmit
      write_register(0x07, 0x09);
  
      //wait for packet to be sent indicated by the nIRQ
      //being pulled low
      while(digitalRead(nIRQ_pin) == HIGH);
      
      //clear interrupt status regs
      int_status1 = read_register(0x03);
      int_status2 = read_register(0x04);
      
      //Setupt the two interrupts for rcving
      write_register(0x05, 0x03);
      write_register(0x06, 0x00);
      
      //clear interrupt status regs
      int_status1 = read_register(0x03);
      int_status2 = read_register(0x04);
      
      //enable rcv chain again
      write_register(0x07, 0x05);
      
    }
    
  }
  
  if(millis() >= beep_beep_timer) {
   
   Serial.print(".");
   beep_beep_timer += 1000; 
    
  }
  
}


//SPI helper functions
//Using the old SPI library
void write_register(byte addr, byte data) {
 
 addr = addr | (0x80); //Setting the R/W bit for Write op
 
 digitalWrite(SS_pin, LOW);
 SPI.transfer(addr);
 SPI.transfer(data);
 digitalWrite(SS_pin, HIGH);
  
}

unsigned int read_register(byte addr) {
 
 unsigned int return_val;
  
 addr = addr & (0x7F); //Clearing the R/W bit for Read op
 
 digitalWrite(SS_pin, LOW);
 SPI.transfer(addr);
 return_val = SPI.transfer(0x00);
 digitalWrite(SS_pin, HIGH);
 return return_val; 
  
}

