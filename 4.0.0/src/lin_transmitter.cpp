#include "lin_transmitter.h"
#include "sio.h"
/* ПАКЕТ LIN:
   Он состоит из:
    ____________________ __________________ ___________________ ______________ ____________
   |                    |                  |                   |              |            |
   |Разрыв синхронизации|Байт синхронизации|Байт идентификатора| Байты данных |Контр.сумма |
   |____________________|__________________|___________________|______________|____________|

   Каждый байт имеет стартовый бит и стоповый бит, и сначала отправляется LSB.
   Synch Break — 13 бит доминирующего состояния («0»), за которыми следует 1 бит рецессивного состояния («1»).
   Байт синхронизации — байт для синхронизации граничной скорости, всегда 0x55.
   Байт идентификатора - состоит из четности, длины и адреса; четность определяется стандартом LIN и зависит от адреса и длины сообщения
   Байты данных - определяется пользователем; зависит от устройств на шине LIN
   Контрольная сумма - перевернутая 256 контрольная сумма; байты данных суммируются, а затем инвертируются
*/
#define tx_pin 12

namespace lin_transmitter
{
  SoftwareSerial Lin_Serial = SoftwareSerial(tx_pin, tx_pin);

  // Создает пакет LIN и затем отправляет его через интерфейс USART (последовательный)
  void writeLin(byte ident, byte data[], byte data_size)
  {
    uint8_t ProtectedID = getProtectedID(ident);
    uint16_t suma = 0x00;
    suma = (uint16_t)ProtectedID;
    for (int i = 0; i < data_size; i++)
    {
      suma += (uint16_t)(data[i]);
      if (suma > 255)
      {
        suma -= 255;
      }
    }
    suma = (uint8_t)(0xFF - ((uint8_t)suma));
    cli();
    // Прерывание синхронизации
    Break(13);
    // Отправляем данные через последовательный интерфейс
    Lin_Serial.begin(bound_rate);  // серийный номер конфигурации
    Lin_Serial.write(0x55);        // записываем байт синхронизации в последовательный порт
    Lin_Serial.write(ProtectedID); // записываем байт идентификации в последовательный порт
    for (int i = 0; i < data_size; i++)
    {
      Lin_Serial.write(data[i]); // записываем данные в последовательный
    }
    Lin_Serial.write(suma); // записываем байт контрольной суммы в последовательный порт
    Lin_Serial.end();       // очистить последовательную конфигурацию
    // Очистить счетчик.
    TCNT2 = 0;
    sio::led_Activity();
    sei();
  }

  void writeLinRequest(byte ident)
  {
    // Создать заголовок
    byte identByte = (ident & 0x3f) | getProtectedID(ident);
    byte header[2] = {0x55, identByte};
    // Прерывание синхронизации
    Break(13);
    // Отправляем данные через последовательный интерфейс
    Lin_Serial.begin(bound_rate); // серийный номер конфигурации
    Lin_Serial.write(header, 2);  // записываем данные в последовательный
    Lin_Serial.end();             // очистить последовательную конфигурацию
  }

  void Break(int no_bits)
  {
    // Расчет задержки, необходимой для 13 бит, зависит от граничной скорости
    unsigned int del = Tbit * no_bits; // задержка количества битов (без битов) в микросекундах, зависит от периода
    DDRB |= (1 << 4);
    PORTB &= ~(1 << 4);      //
    delayMicroseconds(del);  // задержка
    PORTB |= (1 << 4);       //
    delayMicroseconds(Tbit); // задержка
  }

  boolean validateParity(byte ident)
  {
    if (ident == identByte)
      return true;
    else
      return false;
  }

  /* Создаем паритет Lin ID */
  byte getProtectedID(byte ident)
  {
    uint8_t p0 = bitRead(ident, 0) ^ bitRead(ident, 1) ^ bitRead(ident, 2) ^ bitRead(ident, 4);
    uint8_t p1 = ~(bitRead(ident, 1) ^ bitRead(ident, 3) ^ bitRead(ident, 4) ^ bitRead(ident, 5));
    return ((p1 << 7) | (p0 << 6) | (ident & 0x3F));
  }
}