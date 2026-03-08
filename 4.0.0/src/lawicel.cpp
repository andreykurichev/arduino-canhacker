#include "lawicel.h"
#include "system_clock.h"
#include "sio.h"
#include "lin_transmitter.h"

namespace lawicel
{
  static uint8 Transmit_Data[8];
  static const uint8 kQueueRXSize = 26;
  static uint8 bufferRX[kQueueRXSize + 2];
  uint8 RX_Index;
  bool isConnected = false;
  uint8 id;
  uint8 dlc;
  bool timestampEnabled = false;
  bool list_only_Enabled = false;

  void receiveCommand()
  {
    switch (bufferRX[0])
    {
    case COMMAND::COMMAND_GET_SW_VERSION:
      return sio::print(F(SW_VERSION_RESPONSE));

    case COMMAND::COMMAND_GET_VERSION:
      return sio::print(F(VERSION_RESPONSE));

    case COMMAND::COMMAND_GET_SERIAL:
      return sio::print(F(SERIAL_RESPONSE));

    case COMMAND::COMMAND_SEND_11BIT_ID:
      return receiveTransmitCommand();

    case COMMAND::COMMAND_CLOSE_CAN_CHAN:
      return disconnectLin();

    case COMMAND::COMMAND_OPEN_CAN_CHAN:
      return connectLin();

    case COMMAND::COMMAND_SET_BITRATE:
      return receiveSetBitrateCommand();

    case COMMAND::COMMAND_SET_BTR:
      return receiveSetBtrCommand();

    case COMMAND::COMMAND_TIME_STAMP:
      return receiveTimestampCommand();

    case COMMAND::COMMAND_LISTEN_ONLY:
      return receivelisten_only();

    case COMMAND::COMMAND_POLL_ONE:
      return receivePollOneCommand();

    case COMMAND::COMMAND_POLL_ALL:
      return receivePollAllCommand();

    case COMMAND::COMMAND_SEND_29BIT_ID:
      return receiveTransmit29Command();

    case COMMAND::COMMAND_SEND_11BIT_RTR:
      return receiveRtr11Command();

    case COMMAND::COMMAND_SEND_29BIT_RTR:
      return receiveRtr29Command();

    case COMMAND::COMMAND_AUTO_POLL:
      return receiveAutoPollCommand();

    case COMMAND::COMMAND_FLAG_STATUS:
      return receiveFlagStatusCommand();

    case COMMAND::COMMAND_SERIAL_SPEED:
      return receiveUartSpeedCommand();

    default:
    {
      return sio::printchar(BEL);
    }
    }
  }

  void connectLin()
  {
    if (RX_Index != 1 || bufferRX[0] != COMMAND_OPEN_CAN_CHAN)
    {
      return;
    }
    else
    {
      isConnected = true;
      return sio::printchar(CR);
    }
  }

  void disconnectLin()
  {
    if (RX_Index < 1 || bufferRX[0] != COMMAND_CLOSE_CAN_CHAN)
    {
      return;
    }
    else
    {
      isConnected = false;
      return sio::printchar(CR);
    }
  }

  void receiveSetBitrateCommand()
  {
    if (isConnected == true)
    {
      return sio::printchar(BEL);
    }
    switch (bufferRX[1])
    {
    case '0':
      // bitrate = CAN_10KBPS;
      break;
    case '1':
      // bitrate = CAN_20KBPS;
      break;
    case '2':
      // bitrate = CAN_50KBPS;
      break;
    case '3':
      // bitrate = CAN_100KBPS;
      break;
    case '4':
      // bitrate = CAN_125KBPS;
      break;
    case '5':
      // bitrate = CAN_250KBPS;
      break;
    case '6':
      // bitrate = CAN_500KBPS;
      break;
    case '7':
      return sio::printchar(BEL);
    case '8':
      // bitrate = CAN_1000KBPS;
      break;
    default:
      return sio::printchar(BEL);
    }
    return sio::printchar(CR);
  }

  void receiveTransmitCommand()
  {
    if (isConnected == false)
    {
      return;
    }
    if (list_only_Enabled == true)
    {
      return;
    }
    int offset = 1;
    id = 0;
    for (int i = 0; i < 3; i++)
    {
      id <<= 4;
      id += hexCharToByte(bufferRX[offset++]);
    }
    dlc = hexCharToByte(bufferRX[offset++]);
    if (dlc > 8)
    {
      return;
    }

    if (dlc == 0)
    {
      return sio::printchar(CR);
    }

    for (int i = 0; i < dlc; i++)
    {
      char hiHex = bufferRX[offset++];
      char loHex = bufferRX[offset++];
      Transmit_Data[i] = hexCharToByte(loHex) + (hexCharToByte(hiHex) << 4);
    }
    lin_transmitter::writeLin(id, Transmit_Data, dlc);
    return sio::printchar(CR);
  }

  void receiveTimestampCommand()
  {
    if (RX_Index != 2)
    {
      return sio::printchar(BEL);
    }
    switch (bufferRX[1])
    {
    case '0':
      timestampEnabled = false;
      return sio::printchar(CR);
    case '1':
      timestampEnabled = true;
      return sio::printchar(CR);
    default:
      return sio::printchar(BEL);
    }
    return;
  }

  void receiveSetBtrCommand()
  {
    if (isConnected == true)
    {
      return sio::printchar(BEL);
    }
    return sio::printchar(CR);
  }

  unsigned char hexCharToByte(char hex)
  {
    unsigned char result = 0;
    if (hex >= 0x30 && hex <= 0x39)
    {
      result = hex - 0x30;
    }
    else if (hex >= 0x41 && hex <= 0x46)
    {
      result = hex - 0x41 + 0x0A;
    }
    else if (hex >= 0x61 && hex <= 0x66)
    {
      result = hex - 0x61 + 0x0A;
    }
    return result;
  }

  void processChar(char rxChar)
  {
    switch (rxChar)
    {
    case '\r':
    case '\n':
      if (RX_Index > 0)
      {
        bufferRX[RX_Index] = '\0';
        receiveCommand();
        RX_Index = 0;
      }
      break;
    case '\0':
      break;
    default:
      if (RX_Index < kQueueRXSize)
      {
        bufferRX[RX_Index++] = rxChar;
      }
      else
      {
        RX_Index = 0;
        return;
      }
      break;
    }
    return;
  }

  void process()
  {
    while (sio::available())
    {
      processChar(sio::serial_read());
    }
  }

  unsigned char getDlc(uint8 n)
  {
    switch (n - 2)
    {
    case 1:
      return '1';
    case 2:
      return '2';
    case 3:
      return '3';
    case 4:
      return '4';
    case 5:
      return '5';
    case 6:
      return '6';
    case 7:
      return '7';
    case 8:
      return '8';
    default:
      return '0';
    }
  }

  void receivelisten_only()
  {
    isConnected = true;
    list_only_Enabled = true;
    return sio::printchar(CR);
  }

  void receivePollOneCommand()
  {
    // TODO: реализовать опрос одного сообщения
    sio::printchar(CR);
  }

  void receivePollAllCommand()
  {
    sio::printchar(CR);
  }

  void receiveTransmit29Command()
  {
    // TODO: обработка команды T (8-символьный ID)
    sio::printchar(CR);
  }

  void receiveRtr11Command()
  {
    sio::printchar(CR);
  }

  void receiveRtr29Command()
  {
    sio::printchar(CR);
  }

  void receiveAutoPollCommand()
  {
    // TODO: обработка X0/X1
    sio::printchar(CR);
  }

  void receiveFlagStatusCommand()
  {
    // TODO: возвращать флаги состояния
    sio::printchar(CR);
  }

  void receiveUartSpeedCommand()
  {
    // TODO: изменение скорости UART
    sio::printchar(CR);
  }
} // namespace lawicel