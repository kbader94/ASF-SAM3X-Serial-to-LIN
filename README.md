# ASF-SAM3X-Serial-to-LIN
A USB to LIN adapter for SAM3X devices(Arduino Due) that acts as a slave publisher. 
Takes input from serial UART and responds to LIN requests accordingly. 
Based off of ASFs LIN stack but modified to support LIN slave publishing.
It currently only works for 8 byte frames but I'm working on implementing a
better USB communication protocol which would support variable length frames. 

To build it you'll need to download Microchip Studio.

<b>To use it:</b>
1) Open a serial connection to the Due.
2) Send the LIN data you want the Due to respond to, including the ID, followed by a new line char.

<b>For example:</b>

To respond to ID 0x32 with data 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 you would send
0x32 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x0a

The Due will then consistently respond to ID 0x32 with that data, until another frame is sent to the Due.
