#ifndef LAWICEL_H
#define LAWICEL_H

#include <arduino.h>

#include "avr_util.h"

namespace lawicel
{
  extern bool isConnected;
  extern uint8 RX_Index;
  extern uint8 id;
  extern uint8 dlc;
  extern bool timestampEnabled;
  extern bool list_only_Enabled;

#define CR '\r'
#define BEL 7

#define SERIAL_RESPONSE "N0001\r"
#define SW_VERSION_RESPONSE "v0107\r"
#define VERSION_RESPONSE "V1010\r"

  enum COMMAND : char
  {
    COMMAND_SET_BITRATE = 'S',    // установить битрейт LIN
    COMMAND_SET_BTR = 's',        // установить битрейт LIN через
    COMMAND_OPEN_CAN_CHAN = 'O',  // открыть LIN-канал
    COMMAND_CLOSE_CAN_CHAN = 'C', // закрыть LIN-канал
    COMMAND_SEND_11BIT_ID = 't',  // отправить LIN-сообщение с 11bit ID
    COMMAND_SEND_29BIT_ID = 'T',  //
    COMMAND_SEND_11BIT_RTR = 'r', //
    COMMAND_SEND_29BIT_RTR = 'R', //
    COMMAND_GET_VERSION = 'V',    // получить версию аппаратного и программного обеспечения
    COMMAND_GET_SW_VERSION = 'v', // получить только версию ПО
    COMMAND_GET_SERIAL = 'N',     // получить серийный номер устройства
    COMMAND_TIME_STAMP = 'Z',     // переключить настройку метки времени
    COMMAND_LISTEN_ONLY = 'L',    // listen only
    COMMAND_POLL_ONE = 'P',       //
    COMMAND_POLL_ALL = 'A',       //
    COMMAND_AUTO_POLL = 'X',      //
    COMMAND_FLAG_STATUS = 'F',    //
    COMMAND_SERIAL_SPEED = 'U'    //
  };

  extern void processChar(char rxChar);
  extern void process();
  extern void receiveCommand();
  extern void connectLin();
  extern void disconnectLin();
  extern void receiveSetBitrateCommand();
  extern void receiveTransmitCommand();
  extern void receiveTimestampCommand();
  extern void receiveSetBtrCommand();
  extern void receivelisten_only();
  extern void receivePollOneCommand();
  extern void receivePollAllCommand();
  extern void receiveTransmit29Command();
  extern void receiveRtr11Command();
  extern void receiveRtr29Command();
  extern void receiveAutoPollCommand();
  extern void receiveFlagStatusCommand();
  extern void receiveUartSpeedCommand();

  unsigned char hexCharToByte(char hex);
  extern unsigned char getDlc(uint8 n);

} // namespace lawicel

#endif