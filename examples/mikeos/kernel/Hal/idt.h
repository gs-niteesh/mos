#ifndef _IDT_H
#define _IDT_H
//****************************************************************************
//**
//**    Idt.h
//**		Interrupt Descriptor Table. The IDT is responsible for providing
//**	the interface for managing interrupts, installing, setting, requesting,
//**	generating, and interrupt callback managing.
//**
//****************************************************************************

// We can test new architecture here as needed

#include <stdint.h>
#include "regs.h"

//============================================================================
//    INTERFACE REQUIRED HEADERS
//============================================================================
//============================================================================
//    INTERFACE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

//! i86 defines 256 possible interrupt handlers (0-255)
#define I86_MAX_INTERRUPTS 256

//! must be in the format 0D110, where D is descriptor type
#define I86_IDT_DESC_BIT16 0x06		//00000110
#define I86_IDT_DESC_BIT32 0x0E		//00001110
#define I86_IDT_DESC_RING1 0x40		//01000000
#define I86_IDT_DESC_RING2 0x20		//00100000
#define I86_IDT_DESC_RING3 0x60		//01100000
#define I86_IDT_DESC_PRESENT 0x80 //10000000

#define DISPATCHER_ISR 0x80
//============================================================================
//    INTERFACE CLASS PROTOTYPES / EXTERNAL CLASS REFERENCES
//============================================================================
//============================================================================
//    INTERFACE STRUCTURES / UTILITY CLASSES
//============================================================================

//! interrupt descriptor
typedef struct idt_descriptor
{

	//! bits 0-16 of interrupt routine (ir) address
	uint16_t baseLo;

	//! code selector in gdt
	uint16_t sel;

	//! reserved, shold be 0
	uint8_t reserved;

	//! bit flags. Set with flags above
	uint8_t flags;

	//! bits 16-32 of ir address
	uint16_t baseHi;
} __attribute__((packed)) idt_descriptor;

//! describes the structure for the processors idtr register
typedef struct idtr
{

	//! size of the interrupt descriptor table (idt)
	uint16_t limit;

	//! base address of idt
	uint32_t base;
} __attribute__((packed)) idtr;

//============================================================================
//    INTERFACE DATA DECLARATIONS
//============================================================================
//============================================================================
//    INTERFACE FUNCTION PROTOTYPES
//============================================================================

void register_interrupt_handler(uint8_t n, I86_IRQ_HANDLER handler);

//! returns interrupt descriptor
idt_descriptor *i86_get_ir(uint32_t i);

//! installs interrupt handler. When INT is fired, it will call this callback
int i86_install_ir(uint32_t i, uint16_t flags, uint16_t sel, I86_IRQ_HANDLER irq);

// initialize basic idt
int i86_idt_initialize(uint16_t codeSel);

/* ISRs reserved for CPU exceptions */
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void isr128();
/* IRQ definitions */
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

#define IRQ0 32
#define IRQ1 33
#define IRQ2 34
#define IRQ3 35
#define IRQ4 36
#define IRQ5 37
#define IRQ6 38
#define IRQ7 39
#define IRQ8 40
#define IRQ9 41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

void isr_wrapper_handler(interrupt_registers *r);
void irq_wrapper_handler(interrupt_registers *r);
//============================================================================
//    INTERFACE OBJECT CLASS DEFINITIONS
//============================================================================
//============================================================================
//    INTERFACE TRAILING HEADERS
//============================================================================
//****************************************************************************
//**
//**    END [idt.h]
//**
//****************************************************************************
#endif
