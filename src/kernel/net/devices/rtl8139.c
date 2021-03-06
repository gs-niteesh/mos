#include <include/if_ether.h>
#include <kernel/devices/pci.h>
#include <kernel/cpu/hal.h>
#include <kernel/cpu/pic.h>
#include <kernel/memory/vmm.h>
#include <kernel/proc/task.h>
#include <kernel/system/console.h>
#include <kernel/utils/printf.h>
#include <kernel/net/net.h>
#include "rtl8139.h"

extern struct process *current_process;

static uint32_t ioaddr;
static char rx_buffer[RX_PADDING_BUFFER_SIZE] __attribute__((aligned(4)));
// NOTE: MQ 2020-04-10 The maximum ethernet transmitted packet's size is 1792 -> one page
static char tx_buffer[4][PMM_FRAME_SIZE] __attribute__((aligned(4)));
static uint8_t mac_address[6];
static uint8_t tx_counter = 0;

void rtl8139_send_packet(void *payload, uint32_t size)
{
  memcpy(tx_buffer[tx_counter], payload, size);

  outportl(ioaddr + 0x20 + tx_counter * 4, vmm_get_physical_address(&tx_buffer[tx_counter], false));
  outportl(ioaddr + 0x10 + tx_counter * 4, size);

  tx_counter = tx_counter >= 3 ? 0 : tx_counter + 1;
}

uint32_t cip;
void rtl8139_receive_packet()
{
  while ((inportb(ioaddr + RTL8139_ChipCmd) & RTL8139_RxBufEmpty) == 0)
  {
    uint16_t rx_buf_addr = inportw(ioaddr + RTL8139_RxBufAddr);
    uint16_t rx_buf_ptr = inportw(ioaddr + RTL8139_RxBufPtr) + 0x10;
    uint32_t rx_read_ptr = rx_buffer + rx_buf_ptr;
    struct rtl8139_rx_header *rx_header = (struct rtl8139_rx_header *)rx_read_ptr;

    rx_buf_ptr = (rx_buf_ptr + rx_header->size + sizeof(struct rtl8139_rx_header) + 3) & RX_BUF_PTR_MASK;

    if (rx_header->status & (RX_PACKET_HEADER_FAE | RX_PACKET_HEADER_CRC | RX_PACKET_HEADER_RUNT | RX_PACKET_HEADER_LONG))
    {
      debug_print(DEBUG_ERROR, "rtl8139 rx packet header error %x0x", rx_header->status);
    }
    else
    {
      uint8_t *buf = rx_read_ptr + sizeof(struct rtl8139_rx_header);
      uint8_t *payload = kcalloc(1, rx_header->size);

      memcpy(payload, buf, rx_header->size);
      push_rx_queue(payload, rx_header->size);
    }
    outportw(ioaddr + RTL8139_RxBufPtr, rx_buf_ptr - 0x10);
  }
}

int rtl8139_irq_handler(struct interrupt_registers *regs)
{
  uint16_t status = inportw(ioaddr + RTL8139_IntrStatus);

  if (!status)
    return;

  outportw(ioaddr + RTL8139_IntrStatus, status);

  if (status & TOK)
  {
  }
  if (status & ROK)
  {
    rtl8139_receive_packet();
  }

  return IRQ_HANDLER_CONTINUE;
}

uint8_t *get_mac_address()
{
  return mac_address;
}

void rtl8139_init()
{
  struct pci_device *dev = get_pci_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
  ioaddr = dev->bar0 & 0xFFFFFFFC;
  memset(rx_buffer, 0, RX_PADDING_BUFFER_SIZE);

  for (int i = 0; i < 6; ++i)
    mac_address[i] = inportb(ioaddr + RTL8139_MAC0 + i);

  // Enable bus master
  uint32_t command_reg = pci_read_field(dev->address, PCI_COMMAND);
  if (!(command_reg & PCI_COMMAND_REG_BUS_MASTER))
  {
    command_reg |= PCI_COMMAND_REG_BUS_MASTER;
    pci_write_field(dev->address, PCI_COMMAND, command_reg);
  }

  // Turn on RTL8139
  outportb(ioaddr + RTL8139_Config1, 0x0);

  // Software reset
  outportb(ioaddr + RTL8139_ChipCmd, RTL8139_CmdReset);
  while ((inportb(ioaddr + RTL8139_ChipCmd) & RTL8139_CmdReset) != 0)
    ;

  // Init receive buffer
  outportl(ioaddr + RTL8139_RxBuf, vmm_get_physical_address(rx_buffer, false)); // send uint32_t memory location to RBSTART (0x30)

  // Set IMR + ISR
  outportw(ioaddr + RTL8139_IntrMask, RTL8139_PCIErr |                 /* PCI error */
                                          RTL8139_PCSTimeout |         /* PCS timeout */
                                          RTL8139_RxFIFOOver |         /* Rx FIFO over */
                                          RTL8139_RxUnderrun |         /* Rx underrun */
                                          RTL8139_RxOverflow |         /* Rx overflow */
                                          RTL8139_TxErr |              /* Tx error */
                                          RTL8139_TxOK |               /* Tx okay */
                                          RTL8139_RxErr |              /* Rx error */
                                          RTL8139_RxOK /* Rx okay */); // Sets the TOK and ROK bits high

  // Configuring receive buffer (RCR)
  outportl(ioaddr + RTL8139_RxConfig, RTL8139_AcceptBroadcast |
                                          RTL8139_AcceptMulticast |
                                          RTL8139_AcceptMyPhys |
                                          RTL8139_RxNoWrap); // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP

  // Enable Receive and Transmitter
  outportb(ioaddr + RTL8139_ChipCmd, RTL8139_CmdRxEnb | RTL8139_CmdTxEnb); // Sets the RE and TE bits high

  int8_t interrupt_line = pci_get_interrupt_line(dev->address);
  register_interrupt_handler(32 + interrupt_line, rtl8139_irq_handler);
  pic_clear_mask(interrupt_line);
}