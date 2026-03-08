#ifndef ACTION_LED_H
#define ACTION_LED_H

#include <arduino.h>
#include "io_pins.h"
#include "passive_timer.h"

// Оборачивает OutputPin логикой для мигания светодиода при возникновении некоторых событий. Дизайн
// быть видимым независимо от частоты и продолжительности события.
// Требуются вызовы loop() из основного loop().
class ActionLed {
public:
  ActionLed(volatile uint8& port, uint8 bitIndex)
    : led_(port, bitIndex),
      pending_actions_(false) {
    enterIdleState();
  }

  // Периодически вызывается из основного цикла() для выполнения переходов между состояниями.
  void loop() {
    switch (state_) {
      case kState_IDLE:
        if (pending_actions_) {
          enterActiveOnState();
          pending_actions_ = false;
        }
        break;

      case kState_ACTIVE_ON:
        if (timer_.timeMillis() > 15) {
          enterActiveOffState();
        }
        break;
      case kState_ACTIVE_OFF:
        if (timer_.timeMillis() > 25) {
          // ПРИМЕЧАНИЕ: если pending_actions_, то на следующей итерации войдет в состояние ACTIVE_ON.
          enterIdleState();
        }
        break;
    }
  }

  void action() {
    pending_actions_ = true;
  }

private:
  // Нет ожидающих действий. Светодиод можно включить, как только появится новое действие.
  static const uint8 kState_IDLE = 1;
  // Светодиод мигает.
  static const uint8 kState_ACTIVE_ON = 2;
  // Светодиод был включен и теперь находится в периоде затемнения, пока его нельзя будет включить
  // снова.
  static const uint8 kState_ACTIVE_OFF = 3;
  uint8 state_;

  // Основной вывод светодиода. Активный высокий.
  io_pins::OutputPin led_;

  // Таймер для периодов ACtIVE_ON и ACTIVE_OFF.
  PassiveTimer timer_;

  // Указывает, поступило ли новое действие.
  boolean pending_actions_;

  inline void enterIdleState() {
    state_ = kState_IDLE;
    led_.low();
  }

  inline void enterActiveOnState() {
    state_ = kState_ACTIVE_ON;
    led_.high();
    timer_.restart();
  }

  inline void enterActiveOffState() {
    state_ = kState_ACTIVE_OFF;
    led_.low();
    timer_.restart();
  }
};

#endif