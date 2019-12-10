/*
   XModem.h
   XModem/YModem library
   (C) Mattheij Computer Service 1994
   Modified by Tomonobu.Saito@gmail.com 2019
*/

#include <File.h>
#include "XModem.h"

// Number of seconds until giving up hope of receiving sync packets from
// host.
#define SYNC_TIMEOUT 30
// Number of times we try to send a packet to the host until we give up
// sending..
#define MAX_RETRY    30

/* Initialize XModem session */
XModem::XModem(Stream *port, char mode) :
  packetNo(1), crcBuf(0), checksumBuf(0), filepos(0), packetLen(128), mode(mode), port(port)
{
}

/* Send out a byte of payload data,
   includes checksumming
*/
void XModem::outputByte(unsigned char inChar)
{
  char j;
  checksumBuf += inChar;

  crcBuf = crcBuf ^ (int) inChar << 8;
  j = 8;
  do
  {
    if (crcBuf & 0x8000)
      crcBuf = crcBuf << 1 ^ 0x1021;
    else
      crcBuf = crcBuf << 1;
  } while (--j);

  port->write(inChar);
}

/* Wait for either C or NACK as a sync packet.
   Determines protocol details, like block size
   and checksum algorithm.
*/
char XModem::sync(void)
{
  char tryNo;
  char inChar;
  // Wait a second every time until timeout.
  // The magic char expected from the host is 'C' or
  // NAK
  port->setTimeout(1000);
  tryNo = 0;
  do
  {
    port->readBytes(&inChar, 1);
    tryNo++;
    // When timed out, leave immediately
    if (tryNo == SYNC_TIMEOUT)
      return (-1);
  } while ((inChar != 'C') && (inChar != NAK));

  // Determine which checksum algorithm to use
  // this routine also determines the packet length
  this->packetLen = 128;
  this->oldChecksum = (inChar == NAK) ? 1 : 0;

  return (0);
}

/** Wait for the remote to acknowledge or cancel.
    Returns the received char if no timeout occured or
    a CAN was received. In this cases, it returns -1.
  **/
char XModem::waitACK(void)
{
  char i, inChar;
  i = 0;
  do
  {
    port->readBytes(&inChar, 1);
    i++;
    if (i > 200)
      return (-1);
    if (inChar == CAN)
      return (-1);
  } while ((inChar != NAK) && (inChar != ACK) && (inChar != 'C'));
  return (inChar);
}

void XModem::sendFile(File dataFile, const char *fileName)
{
  char inChar;
  int i;
  unsigned char tryNo;

  // Rewind data file before sending the file..
  dataFile.seek(0);

  // When doing YModem, send block 0 to inform host about
  // file name to be received
  if (this->mode == ModeYModem)
  {
    if (this->sync() != 0)
      goto err;

    // Send header for virtual block 0 (file name)
    port->write(SOH);
    port->write((uint8_t)0);
    port->write(0xFF);

    for (i = 0; i < strlen(fileName); i++)
    {
      this->outputByte(fileName[i]);
    }
    char filesize[16];
    memset(filesize, 0, sizeof(filesize));
    sprintf(filesize, "%d", dataFile.size());
    this->outputByte((uint8_t)0x00);
    ++i;
    for (int j = 0; j < strlen(filesize); ++j, ++i) {
      this->outputByte(filesize[j]);
    }
    for (; i < 128; i++)
    {
      this->outputByte((uint8_t)0x00);
    }
    if (oldChecksum)
      port->write((char)255 - checksumBuf);
    else
    {
      port->write((char) (crcBuf >> 8));
      port->write((char) (crcBuf & 0xFF));
    }
    // Discard ACK/NAK/CAN, in case
    // we communicate to an XMODEM-1k client
    // which might not know about the 0 block.
    waitACK();
  }

  if (this->sync() != 0)
    goto err;

  while (0 < dataFile.available())
  {
    filepos = dataFile.position();

    // Sending a packet will be retried
    tryNo = 0;
    do
    {
      // Seek to start of current data block,
      // will advance through the file as
      // block will be acked..
      dataFile.seek(filepos);

      // Reset checksum stuff
      checksumBuf = 0x00;
      crcBuf = 0x00;

      // Try to use 1K(1024 byte) mode if possible
      if (ModeYModem == mode && 128 < dataFile.available()) {
        packetLen = 1024; // 1K mode
      } else {
        packetLen = 128; // normal mode
      }

      // Try to send packet, so header first
      if (packetLen == 128)
        port->write(SOH);
      else
        port->write(STX);

      port->write(packetNo);
      port->write(~packetNo);
      for (i = 0; i < packetLen; i++)
      {
        inChar = (0 < dataFile.available()) ? dataFile.read() : EOF;
        this->outputByte(inChar);
      }
      // Send out checksum, either CRC-16 CCITT or
      // classical inverse of sum of bytes.
      // Depending on how the received introduced himself
      if (oldChecksum)
        port->write((char)checksumBuf);
      else
      {
        port->write((char)(crcBuf >> 8));
        port->write((char)(crcBuf & 0xFF));
      }

      inChar = waitACK();
      tryNo++;
      if (tryNo > MAX_RETRY)
        goto err;
    } while (inChar != ACK);

    packetNo++;
  }
  // Send EOT and wait for ACK
  tryNo = 0;
  do
  {
    port->write(EOT);
    inChar = waitACK();
    tryNo++;
    // When timed out, leave immediately
    if (tryNo == SYNC_TIMEOUT)
      goto err;
  } while (inChar != ACK);

  // Send "all 00 data" to finish YModem.
  if (this->mode == ModeYModem) {
    // wait 'C' from Rx(PC)
    this->sync();
    // send header.
    port->write(SOH);
    port->write((uint8_t)0x00);
    port->write((uint8_t)0xFF);
    // Reset checksum stuff
    checksumBuf = 0x00;
    crcBuf = 0x00;
    // send all '00' data (128byte)
    for (i = 0; i < 128; ++i) {
      this->outputByte(0x00);
    }
    // send checksum/CRC
    if (oldChecksum) {
      port->write((char)checksumBuf); // need debug
    } else {
      port->write((uint8_t)(crcBuf >> 8));
      port->write((uint8_t)(crcBuf & 0xFF));
    }
    // Wait ACK from Rx.
    if (ACK != waitACK())
      goto err;
  }

  port->println("Finish sending.");
  return;

  // When we get here everything was successful.
err:
  port->println("Error sending...");
}
