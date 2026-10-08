#pragma once

#define RBR_ADDR 0x00 // RO : Reciever Buffer Register, Read only
#define THR_ADDR 0x00 // WO : Transmitter Holding Register, Write only
#define IER_ADDR 0x01 // RW : Enable(1)/Disable(0) interrupts
#define IIR_ADDR 0x02 // RO : Information which int. occured, read only (default 0x01)
#define FCR_ADDR 0x02 // WO : Control behavior of the internal FIFOs. Write only
#define LCR_ADDR 0x03 // RW : LCR7 / DLAB is the only interesting one (?) (rw)
#define LSR_ADDR 0x05 // RO : Information about state of the uart, 0x60 (default) indicates its ready to transmit