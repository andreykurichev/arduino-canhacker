#ifndef IO_PINS_H
#define IO_PINS_H

#include "avr_util.h"

namespace io_pins {
// Класс для абстрагирования выходного вывода, который не обязательно является Arduino
// цифровой пин. Также оптимизирован для быстрого включения/выключения.
//
// Предполагается, что прерывания разрешены и поэтому не должны вызываться
// из ISR.
class OutputPin {
public:
  // порт может быть либо PORTB, PORTC, либо PORTD.
  // индекс бита один из [7, 6, 5, 4, 3, 2, 1, 0] (младший бит).
  // ПРИМЕЧАНИЕ: порт ddr всегда на один адрес ниже portx.
  // ПРИМЕЧАНИЕ: pin-порт всегда на два адреса ниже portx.
  OutputPin(volatile uint8& port, uint8 bitIndex)
    : port_(port),
      pin_(*((&port) - 2)),
      bit_mask_(1 << bitIndex) {
    // ПРИМЕЧАНИЕ: порт ddr всегда на один адрес ниже порта.
    volatile uint8& ddr = *((&port) - 1);
    ddr |= bit_mask_;
    low();  // default state.
  }

  inline void high() {
    // Оборачиваем в cli/sei в случае изменения любого пин этого порта
    // из ISR. Сохраняйте это как можно короче, чтобы избежать дрожания
    // в вызове ISR.
    cli();
    port_ |= bit_mask_;
    sei();
  }

  inline void low() {
    // Оборачиваем в cli/sei в случае изменения любого пин этого порта
    // из ISR. Сохраняйте это как можно короче, чтобы избежать дрожания
    // в вызове ISR.
    cli();
    port_ &= ~bit_mask_;
    sei();
  }

  inline void set(boolean v) {
    if (v) {
      high();
    } else {
      low();
    }
  }

  inline void toggle() {
    set(!isHigh());
  }

  inline boolean isHigh() {
    return pin_ & bit_mask_;
  }

private:
  volatile uint8& port_;
  volatile uint8& pin_;
  const uint8 bit_mask_;
};

// Класс для абстрагирования входного вывода, который не обязательно является Arduino
// цифровой пин. Также оптимизирован для быстрого доступа.
class InputPin {
public:
  // См. описание аргументов в OutputPin().
  InputPin(volatile uint8& port, uint8 bitIndex)
    : pin_(*((&port) - 2)),
      bit_mask_(1 << bitIndex) {
    volatile uint8& ddr = *((&port) - 1);
    ddr &= ~bit_mask_;  // input
    port |= bit_mask_;  // pullup
  }

  inline boolean isHigh() {
    return pin_ & bit_mask_;
  }

private:
  volatile uint8& pin_;
  const uint8 bit_mask_;
};

}  // пространство имен io_pins

#endif