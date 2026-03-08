#include "action_led.h"
#include "sio.h"
#include "lin_processor.h"
#include "lawicel.h"
#include <stdarg.h>
#include "passive_timer.h"
#include "system_clock.h"
namespace sio
{
  static ActionLed ledActivity(PORTB, 0);

  // TODO: нужно ли установить контакты ввода/вывода (PD0, PD1)? Мы полагаемся на настройку
  // загрузчик?

  // Размер очереди выходных байтов. Должно быть <= 128, чтобы избежать переполнения.
  // TODO: уменьшить размер буфера? Хватит ли нам оперативной памяти?
  // TODO: увеличить размер индекса до 16 бит и увеличить размер буфера до 200?
  static const uint8 kQueueTXSize = 128;
  static const uint8 kQueueSerialRXSize = 64;
  static uint8 bufferTX[kQueueTXSize];
  static uint8 bufferSerial[kQueueSerialRXSize];
  static const char hex_table[] PROGMEM = "0123456789ABCDEF";

  // Индекс самой старой записи в буфере TX.
  static uint8 start;
  // Количество байтов в очереди TX.
  static uint8 count;
  // Индекс самой старой записи в буфере TX.
  static volatile uint8 rx_buffer_head;
  // Количество байтов в очереди TX.
  static volatile uint8 rx_buffer_tail;

  // Вызывающий должен убедиться, что count < kQueueTXSize перед вызовом этого.
  static void unsafe_enqueue(byte b)
  {
    // kQueueTXSize достаточно мал, чтобы не было переполнения.
    uint8 next = start + count;
    if (next >= kQueueTXSize)
    {
      next -= kQueueTXSize;
    }
    bufferTX[next] = b;
    count++;
  }

  // Вызывающий должен убедиться, что count > 1 перед вызовом этого.
  static byte unsafe_dequeue()
  {
    const uint8 b = bufferTX[start];
    if (++start >= kQueueTXSize)
    {
      start = 0;
    }
    count--;
    return b;
  }

  void setup()
  {
    start = 0;
    count = 0;
    rx_buffer_head = 0;
    rx_buffer_tail = 0;
    lawicel::RX_Index = 0;

    // Девизы см. в таблице 19-12 в техническом описании atmega328p.
    // U2X0, 16 -> 115,2 кбод при 16 МГц.
    // U2X0, 207 -> 9600 бод @ 16 МГц.
    UBRR0H = 0;
    UBRR0L = 16; // Скорость uart
    // Бит U2X0 (1) регистра UCSR0A - удвоение скорости обмена, если установить в 1 (только в асинхронном режиме. в синхронном следует установить этот бит в 0).
    UCSR0A = H(U2X0);
    // Включаем приемник. Включить передатчик. Разрешаем прирывания.
    UCSR0B = H(RXEN0) | H(TXEN0) | H(RXCIE0);
    UCSR0C = H(UCSZ01) | H(UCSZ00); // 8 data bits
    while ((UCSR0A & (1 << RXC0)))
    {
      UDR0;
    }

    sei(); // разрешение глобального прерывания
  }

  ISR(USART_RX_vect)
  {
    uint8 i = (rx_buffer_head + 1) % kQueueSerialRXSize;
    if (i != rx_buffer_tail)
    {
      bufferSerial[rx_buffer_head] = UDR0;
      rx_buffer_head = i;
    }
  }

  int available()
  {
    return (kQueueSerialRXSize + rx_buffer_head - rx_buffer_tail) % kQueueSerialRXSize;
  }

  char serial_read()
  {
    if (rx_buffer_head == rx_buffer_tail)
    {
      return 0;
    }
    char serialChar = bufferSerial[rx_buffer_tail];
    rx_buffer_tail = (rx_buffer_tail + 1) % kQueueSerialRXSize;
    return serialChar;
  }

  void printchar(uint8 c)
  {
    // Если буфер заполнен, отбрасываем этот символ.
    // TODO: отбросить последний байт, чтобы освободить место для нового байта?
    if (count >= kQueueTXSize)
    {
      return;
    }
    unsafe_enqueue(c);
  }

  void loop()
  {
    ledActivity.loop();
    if (count && (UCSR0A & H(UDRE0)))
    {
      UDR0 = unsafe_dequeue();
    }
  }

  uint8 capacity()
  {
    return kQueueTXSize - count;
  }

  void waitUntilFlushed()
  {
    // Цикл занятости до тех пор, пока все не будет сброшено в UART.
    while (count)
    {
      loop();
    }
  }

  // Предположим, что n находится в диапазоне [0, 15].
  static void printHexDigit(uint8 n)
  {
    if (n < 10)
    {
      printchar((char)('0' + n));
    }
    else
    {
      printchar((char)(('a' - 10) + n));
    }
  }

  void printhex2(uint8 b)
  {
    printHexDigit(b >> 4);
    printHexDigit(b & 0xf);
  }

  static inline void printhex2_fast(uint8_t b)
  {
    printchar(pgm_read_byte(&hex_table[b >> 4]));
    printchar(pgm_read_byte(&hex_table[b & 0x0F]));
  }

  static inline void printhex4(uint16_t v)
  {
    printchar(pgm_read_byte(&hex_table[(v >> 12) & 0x0F]));
    printchar(pgm_read_byte(&hex_table[(v >> 8) & 0x0F]));
    printchar(pgm_read_byte(&hex_table[(v >> 4) & 0x0F]));
    printchar(pgm_read_byte(&hex_table[v & 0x0F]));
  }

  void println()
  {
    printchar('\n');
  }

  inline uint16_t get_timestamp()
  {
    cli();
    uint16_t ts = (uint16_t)system_clock::timeMillis();
    sei();
    static uint16_t filtered_ts = 0;
    static bool first = true;
    if (first)
    {
      filtered_ts = ts;
      first = false;
    }
    else
    {
      // α = 0.1 → filtered = 0.1 * raw + 0.9 * filtered
      // В целых числах: filtered = (raw + 9 * filtered) / 10
      filtered_ts = (uint16_t)(((uint32_t)ts + 9UL * filtered_ts) / 10);
    }
    return filtered_ts;
  }

  void print_computer()
  {
    LinFrame frame;
    if (lin_processor::readNextFrame(&frame))
    {
      if (frame.isValid())
      {
        uint8_t num = frame.num_bytes();
        if (num < 3)
        {
          return;
        }
        uint8_t id = frame.get_byte(0);
        uint8_t dlc = num - 2;
        led_Activity();
        // тип кадра
        printchar('t');
        // ----- ID -----
        printHexDigit(0);
        printhex2_fast(id);
        // ----- DLC без id и checksum -----
        printHexDigit(dlc);
        // ----- DATA -----
        for (uint8_t i = 1; i < (num - 1); i++)
        {
          printhex2_fast(frame.get_byte(i));
        }
        // ----- TIMESTAMP -----
        if (lawicel::timestampEnabled)
        {
          printhex4(get_timestamp());
        }
        // конец строки
        printchar(CR);
      }
    }
  }

  void print(const __FlashStringHelper *str)
  {
    const char *PROGMEM p = (const char PROGMEM *)str;
    for (;;)
    {
      const unsigned char c = pgm_read_byte(p++);
      if (!c)
      {
        return;
      }
      printchar(c);
    }
  }

  void println(const __FlashStringHelper *str)
  {
    print(str);
    println();
  }

  void print(const char *str)
  {
    for (;;)
    {
      const char c = *(str++);
      if (!c)
      {
        return;
      }
      printchar(c);
    }
  }

  void println(const char *str)
  {
    print(str);
    println();
  }

  void printf(const __FlashStringHelper *format, ...)
  {
    // Предполагается один поток, использующий статический буфер.
    static char buf[80];
    va_list ap;
    va_start(ap, format);
    vsnprintf_P(buf, sizeof(buf), (const char *)format, ap); // программа для AVR
    for (char *p = &buf[0]; *p; p++)                         // эмулировать приготовленный режим для новых строк
    {
      printchar(*p);
    }
    va_end(ap);
  }

  void led_Activity()
  {
    ledActivity.action();
  }
} // пространство имен sio