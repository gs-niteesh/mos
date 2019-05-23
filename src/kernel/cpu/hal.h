#ifndef CPU_HAL_H
#define CPU_HAL_H

#include <stdint.h>

void enable_interrupts();
void disable_interrupts();
void halt();
void io_wait();

unsigned char inportb(unsigned short portid);
void outportb(unsigned short portid, unsigned char value);
uint16_t inportw(uint16_t portid);
void outportw(uint16_t portid, uint16_t value);
uint32_t inportl(uint16_t portid);
void outportl(uint16_t portid, uint32_t value);

const char *get_cpu_vender();

#endif