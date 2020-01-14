#include <include/list.h>
#include <kernel/utils/printf.h>
#include <kernel/utils/string.h>
#include <kernel/memory/vmm.h>
#include "idt.h"
#include "pic.h"

extern void idt_flush(uint32_t);

static struct idt_descriptor _idt[I86_MAX_INTERRUPTS];
static struct idtr _idtr;

typedef struct interrupt_handler
{
  I86_IRQ_HANDLER handler;
  struct list_head sibling;
} interrupt_handler;

struct list_head interrupt_handlers[256];

void idt_install_ir(uint32_t i, uint16_t flags, uint16_t sel, I86_IVT irq)
{
  if (i > I86_MAX_INTERRUPTS)
    return;

  uint64_t base = (uint64_t) & (*irq);

  _idt[i].baseLo = base & 0xffff;
  _idt[i].baseHi = (base >> 16) & 0xffff;
  _idt[i].flags = flags;
  _idt[i].sel = sel;
  _idt[i].reserved = 0;
}

void setvect(uint32_t i, I86_IVT irq)
{
  idt_install_ir(i, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x8, irq);
}

void setvect_flags(uint32_t i, I86_IVT irq, uint32_t flags)
{
  idt_install_ir(i, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32 | flags, 0x8, irq);
}

void i86_default_handler(interrupt_registers *regs)
{
  disable_interrupts();

  uint8_t int_no = regs->int_no & 0xff;

  DebugPrintf("\n*** [i86 Hal]: unhandled exception %d", int_no);

  halt();
}

void idt_init()
{
  _idtr.limit = sizeof(struct idt_descriptor) * I86_MAX_INTERRUPTS - 1;
  _idtr.base = &_idt[0];

  memset(&_idt[0], 0, _idtr.limit);

  for (int i = 0; i < 256; ++i)
  {
    setvect(i, i86_default_handler);
    INIT_LIST_HEAD(&interrupt_handlers[i]);
  }

  // Install the ISRs
  setvect(0, (I86_IVT)isr0);
  setvect(1, (I86_IVT)isr1);
  setvect(2, (I86_IVT)isr2);
  setvect(3, (I86_IVT)isr3);
  setvect(4, (I86_IVT)isr4);
  setvect(5, (I86_IVT)isr5);
  setvect(6, (I86_IVT)isr6);
  setvect(7, (I86_IVT)isr7);
  setvect(8, (I86_IVT)isr8);
  setvect(9, (I86_IVT)isr9);
  setvect(10, (I86_IVT)isr10);
  setvect(11, (I86_IVT)isr11);
  setvect(12, (I86_IVT)isr12);
  setvect(13, (I86_IVT)isr13);
  setvect(14, (I86_IVT)isr14);
  setvect(15, (I86_IVT)isr15);
  setvect(16, (I86_IVT)isr16);
  setvect(17, (I86_IVT)isr17);
  setvect(18, (I86_IVT)isr18);
  setvect(19, (I86_IVT)isr19);
  setvect(20, (I86_IVT)isr20);
  setvect(21, (I86_IVT)isr21);
  setvect(22, (I86_IVT)isr22);
  setvect(23, (I86_IVT)isr23);
  setvect(24, (I86_IVT)isr24);
  setvect(25, (I86_IVT)isr25);
  setvect(26, (I86_IVT)isr26);
  setvect(27, (I86_IVT)isr27);
  setvect(28, (I86_IVT)isr28);
  setvect(29, (I86_IVT)isr29);
  setvect(30, (I86_IVT)isr30);
  setvect(31, (I86_IVT)isr31);

  // Install the IRQs
  setvect(32, (I86_IVT)irq0);
  setvect(33, (I86_IVT)irq1);
  setvect(34, (I86_IVT)irq2);
  setvect(35, (I86_IVT)irq3);
  setvect(36, (I86_IVT)irq4);
  setvect(37, (I86_IVT)irq5);
  setvect(38, (I86_IVT)irq6);
  setvect(39, (I86_IVT)irq7);
  setvect(40, (I86_IVT)irq8);
  setvect(41, (I86_IVT)irq9);
  setvect(42, (I86_IVT)irq10);
  setvect(43, (I86_IVT)irq11);
  setvect(44, (I86_IVT)irq12);
  setvect(45, (I86_IVT)irq13);
  setvect(46, (I86_IVT)irq14);
  setvect(47, (I86_IVT)irq15);

  setvect_flags(DISPATCHER_ISR, (I86_IVT)isr127, I86_IDT_DESC_RING3);

  idt_flush((uint32_t)&_idtr);

  pic_remap();
}

void register_interrupt_handler(uint32_t n, I86_IRQ_HANDLER handler)
{
  interrupt_handler *ih = kmalloc(sizeof(interrupt_handler));
  ih->handler = handler;
  list_add_tail(&ih->sibling, &interrupt_handlers[n]);
}

void handle_interrupt(interrupt_registers *regs)
{
  uint32_t int_no = regs->int_no & 0xff;
  struct list_head *ihlist = &interrupt_handlers[int_no];

  if (!list_empty(ihlist))
  {
    interrupt_handler *ih;
    list_for_each_entry_reverse(ih, ihlist, sibling)
    {
      if (ih->handler(regs) == IRQ_HANDLER_STOP)
        return;
    }
  }
  else
  {
    DebugPrintf("\nunhandled interrupt %d", int_no);
  }
}

void isr_handler(interrupt_registers *reg)
{
  handle_interrupt(reg);
}

void irq_handler(interrupt_registers *reg)
{
  if (reg->int_no >= 40)
    outportb(PIC2_COMMAND, PIC_EOI);
  outportb(PIC1_COMMAND, PIC_EOI);

  handle_interrupt(reg);
}