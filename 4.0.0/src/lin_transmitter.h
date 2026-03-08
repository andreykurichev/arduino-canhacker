#pragma once
#include <Arduino.h>
#include "custom_defs.h"
#include <SoftwareSerial.h>

namespace lin_transmitter
{

  const unsigned long bound_rate = custom_defs::kLinSpeed; // 10417 лучше всего подходит для интерфейса LIN, большинство устройств должны работать
  const unsigned int Tbit = 1000000 / bound_rate;          // в микросекундах, 1с/10417
  extern byte identByte;                                   // определяемый пользователем байт идентификации

  extern void writeLin(byte add, byte data[], byte data_size);                      // записать весь пакет
  extern void writeLinRequest(byte add);                                            // Запись только заголовка
  extern void Break(int no_bits);                                                // для генерации Synch Break
  extern boolean validateParity(byte ident);                                    // для проверки байта идентификации, можно изменить для проверки четности
  extern uint8_t getChecksum(uint8_t ProtectedID, byte data[], byte data_size); // для проверки байта контрольной суммы
  extern uint8_t getProtectedID(byte ident);
}
