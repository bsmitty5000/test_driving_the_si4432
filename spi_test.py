# -*- coding: utf-8 -*-
"""
Testing out HC-SR04 on dsPIC33f
"""
import serial
import struct
        
ser = serial.Serial(port = '/dev/ttyUSB0', 
                    baudrate = 115200,
                    bytesize = 8,
                    parity = serial.PARITY_NONE,
                    stopbits = 1,
                    timeout = 10)

user_in = 'a'
exit_loop = False


while (not exit_loop) :
    
    user_in = raw_input('r = read; w = write; q = quit: ')
    
    if (user_in == 'q'):
        exit_loop = True
    else:
        if (user_in == 'w'):
            ser.write('w')
            user_in = int(input('enter address in decimal: '))
            ser.write(struct.pack('B', user_in))
            user_in = int(input('enter data in decimal: '))
            ser.write(struct.pack('B', user_in))
        else:
            ser.write('r')
            user_in = int(input('enter address in decimal: '))
            #ser.write(bytes(user_in))
            ser.write(struct.pack('B', user_in))
            rcvd = ser.read(1)
            rcvd = struct.unpack('B', rcvd)
            print('addr: ' + str(user_in) + ' data: ' + format(rcvd[0], '02x'))

ser.close() 

