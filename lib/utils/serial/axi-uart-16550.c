/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi_utils/serial/axi-uart-16550.h>

/* clang-format off */

#define UART_REG_TXFIFO		0
#define UART_REG_RXFIFO		1
#define UART_REG_TXCTRL		2
#define UART_REG_RXCTRL		3
#define UART_REG_IE		4
#define UART_REG_IP		5
#define UART_REG_DIV		6

#define UART_TXFIFO_FULL	0x80000000
#define UART_RXFIFO_EMPTY	0x80000000
#define UART_RXFIFO_DATA	0x000000ff
#define UART_TXCTRL_TXEN	0x1
#define UART_RXCTRL_RXEN	0x1

/* clang-format on */

// RBR: Receiver buffer register [Read, LCR[7] == 0]
#define UART_RBR 0x0u

// THR: Transmitter Holding register [Write, LCR[7] == 0]
#define UART_THR 0x0u

// IER: Interrupt enable register [Read/Write, LCR[7] == 0]
#define UART_IER 0x1u

// IIR: Interrupt identification register [Read]
#define UART_IIR 0x2u

// FCR: FIFO control register [Write, Read only when LCR[7] == 1]
#define UART_FCR 0x2u

// LCR: Line control register [Read/Write]
#define UART_LCR 0x3u

// MCR: Modem control register [Read/Write]
#define UART_MCR 0x4u

// LSR: Line status register [Read/Write]
#define UART_LSR 0x5u

// MSR: Modem status register [Read/Write]
#define UART_MSR 0x6u

// SCR: Scratch register [Read/Write]
#define UART_SCR 0x7u

// DLL: Divisor latch (least significant byte) register [Read/Write, LCR[7] == 1]
#define UART_DLL 0x0u

// DLM: Divisor latch (most significant byte) register [Read/Write, LCR[7] == 1]
#define UART_DLM 0x1u

volatile uint32_t *uart_base_ptr = (uint32_t *)((uint32_t)(0x60000000 | 0x1000));


void axi_uart_16550_putc(char ch)
{
	while(! (*(uart_base_ptr + UART_LSR) & 0x40u));
  	*(uart_base_ptr + UART_THR) = ch;
}

int axi_uart_16550_getc(void)
{

	//u32 ret = get_reg(UART_REG_RXFIFO);
	if (*(uart_base_ptr + UART_LSR) & 0x1u)
		return (*(uart_base_ptr + UART_RBR)) & UART_RXFIFO_DATA;
	return -1;
}

int axi_uart_16550_init()
{

    // set 0x0080 to UART.LCR to enable DLL and DLM write
  	// configure baud rate
	*(uart_base_ptr + UART_LCR) = 0x0080;

	// System clock 30 MHz, 115200 baud rate
	// divisor = clk_freq / (16 * Baud)
	*(uart_base_ptr + UART_DLL) = 30*1000*1000u / (16u * 115200u) % 0x100u;
	*(uart_base_ptr + UART_DLM) = 30*1000*1000u / (16u * 115200u) >> 8;
    *(uart_base_ptr + UART_LCR) = 0x000Bu;
	/* Disable interrupts */
	// set_reg(UART_REG_IE, 0);
    *(uart_base_ptr + UART_IER) = 0x1u;
	/* Enable TX */
	// set_reg(UART_REG_TXCTRL, UART_TXCTRL_TXEN);
	/* Enable Rx */
	// set_reg(UART_REG_RXCTRL, UART_RXCTRL_RXEN);

	return 0;
}