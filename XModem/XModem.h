/*
   XModem.h
   XModem/YModem constants
   (C) Mattheij Computer Service 1994
   Modified by Tomonobu.Saito@gmail.com 2019
*/

/* $Id: XModem.h,v 1.53 2012/10/24 19:03:14 deuce Exp $ */

#ifndef _XMODEM_H
#define _XMODEM_H

#include <Arduino.h>
#include <File.h>

/*
   ascii constants
*/

#ifndef SOH
#define SOH     0x01
#define STX     0x02
#define EOT     0x04
#define ENQ     0x05
#define ACK     0x06
#define LF      0x0a
#define CR      0x0d
#define DLE     0x10
#define XON     0x11
#define XOFF    0x13
#define NAK     0x15
#define CAN     0x18
#define EOF     0x1a
#endif

#define ModeXModem	0
#define ModeYModem	1

class XModem
{
  public:
    XModem(Stream *port, char mode);
    void sendFile(File dataFile, const char *fileName);
  private:
    Stream *port;
    unsigned char packetNo, checksumBuf;
    long filepos;
    unsigned int packetLen;
    int crcBuf;
    unsigned char mode;
    unsigned char oldChecksum;

    char sync(void);
    void outputByte(unsigned char inChar);
    char waitACK(void);
};

#endif // _XMODEM_H
