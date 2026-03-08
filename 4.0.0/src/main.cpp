#include "action_led.h"
#include "avr_util.h"
#include "custom_defs.h"
#include "hardware_clock.h"
#include "io_pins.h"
#include "lin_processor.h"
#include "sio.h"
#include "system_clock.h"
#include "lawicel.h"

// Светодиод ОШИБКИ - мигает при обнаружении ошибок.
static ActionLed ledError(PORTB, 1);

// Функция настройки Arduino. Вызывается один раз во время инициализации.
void setup()
{
  DDRB |= (1 << PB5);
  // Жестко запрограммировано на скорость 115,2 кбод. Использует URART0, без прерываний.
  // Сначала инициализируйте это, так как некоторые методы настройки используют его.
  sio::setup();

  // Использует Timer1, без прерываний.
  hardware_clock::setup();

  // Использует Timer2 с прерываниями и несколькими контактами ввода-вывода. Подробности смотрите в исходном коде.
  lin_processor::setup();

  // Включить глобальные прерывания. Мы ожидаем, что будут прерывания только от таймера 1
  // процессор lin для уменьшения джиттера ISR.
  sei();
}

// Метод Arduino loop(). Вызывается после установки(). Никогда не возвращается.
// Это быстрый цикл, который не использует delay() или другие занятые циклы или
// блокировка вызовов.
void loop()
{
  PORTB |= (1 << PB5);
  // Наличие собственного цикла сокращает примерно 4 мкс на итерацию. Это также устраняет
  // любая базовая функциональность, которая может нам не понадобиться.
  // Периодические обновления.
  system_clock::loop();
  lin_processor::loop();
  sio::loop();
  ledError.loop();

  // Печатать периодические текстовые сообщения, если нет активности.
  static PassiveTimer idle_timer;
  // Используется для запуска периодической печати ошибок.
  static PassiveTimer lin_errors_timeout;
  // Накапливает флаги ошибок до следующей печати.
  static uint8 pending_lin_errors = 0;
  const uint8 new_lin_errors = lin_processor::getAndClearErrorFlags();

  // Обработка флагов ошибок процессора LIN.
  {
    if (new_lin_errors && lawicel::isConnected == true)
    {
      // Сделать так, чтобы светодиод ERRORS мигал.
      ledError.action();
      idle_timer.restart();
    }

    // Если есть ожидающие ошибки очистите.
    pending_lin_errors |= new_lin_errors;
    if (pending_lin_errors && lin_errors_timeout.timeMillis() > 1000)
    {
      lin_errors_timeout.restart();
      pending_lin_errors = 0;
    }
  }

  if (sio::available())
  {
    lawicel::process();
  }
  // Обработка полученных кадров LIN.
  if (lawicel::isConnected == true)
  {
    sio::print_computer();
  }
  PORTB &= ~(1 << PB5);
}