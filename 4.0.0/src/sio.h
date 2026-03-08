#ifndef SIO_H
#define SIO_H

#include <arduino.h>
#include "avr_util.h"
#include "lawicel.h"

// Последовательный выход, использующий аппаратный UART0 и без прерываний (для более низких
// дрожание прерывания). Требуются периодические вызовы update() для отправки в буфер.
// байты в UART.
//
// Выход TX — TXD (PD1) — контакт 31
// Вход RX — RXD (PD0— контакт 30
namespace sio {

// Вызов из main setup(и loop(соответственно.
extern void setup();
extern void loop();

// Мгновенный размер свободного места в выходном буфере. Отправка не более этого номера
// символов не потеряет ни одного байта.
extern uint8 capacity();

extern char serial_read ();
extern int available();
extern void printchar(uint8 b);
extern void print(const __FlashStringHelper *str);
extern void println(const __FlashStringHelper *str);
extern void print(const char *str);
extern void println(const char *str);
extern void println();
extern void print_computer();
extern void printf(const __FlashStringHelper *format, ...);
extern void printhex2(uint8 b);
extern void led_Activity();
inline uint16_t get_timestamp();


// Ожидание в цикле занятости, пока все байты не будут сброшены в UART.
// По возможности избегайте использования этого. Полезно, когда нужно распечатать
// во время setup() больше, чем может содержать выходной буфер.
void waitUntilFlushed();
}  // пространство имен sio

#endif
