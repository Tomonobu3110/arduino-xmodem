# arduino-xmodem

arduino-xmodem is the X/YModem library for arduino.  
You can send files on your board to your host PC via serial port  
using XModem/YModem protocol.  

## Features

XModem support. (Both of checksum / CRC mode)  
YModem support. (1024k transfer is ready)  

## Requirements

Arduion development environment.  
Arduion IDE.  
No extra libary is required.  
This library uses File and Stream class, these are common for Arduino development environment.  

## Installation

clone or download this git repository.
```
$ git clone https://github.com/Tomonobu3110/arduino-xmodem.git
```
Then, include XModem library by Arduino IDE.  
Menu > Sketch > Include Library > Install Library... > (Select XModel directory)  

## Usage

You can find two examples after install XModem library.  
* XModem_SD_Download
* YModem_SD_Download

## Note

This library supprot only Tx (Transmit eXchange side = sender).  
If you want Rx (= Receiver), please find another library.  
or, implement it and do "pull request" :)  

## Author

* Original implimentation by (C) Mattheij Computer Service 1994  
* Modified by Tomo3110(Tomonobu.Saito@gmail.com) 2019  

## License
