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
#include <sbi_utils/serial/sifive-uart.h>

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

static volatile void *uart_base;
// static u32 uart_in_freq;
// static u32 uart_baudrate;


/**********************************************************************************/
// RBR: Receiver buffer register [Read, LCR[7] == 0]
#define UART_RBR 0x0u

// THR: Transmitter Holding register [Write, LCR[7] == 0]
#define UART_THR 0x0u

// IER: Interrupt enable register [Read/Write, LCR[7] == 0]
#define UART_IER 0x4u

// IIR: Interrupt identification register [Read]
#define UART_IIR 0x8u

// FCR: FIFO control register [Write, Read only when LCR[7] == 1]
#define UART_FCR 0x8u

// LCR: Line control register [Read/Write]
#define UART_LCR 0xCu

// MCR: Modem control register [Read/Write]
#define UART_MCR 0x10u

// LSR: Line status register [Read/Write]
#define UART_LSR 0x14u

// MSR: Modem status register [Read/Write]
#define UART_MSR 0x18u

// SCR: Scratch register [Read/Write]
#define UART_SCR 0x1Cu

// DLL: Divisor latch (least significant byte) register [Read/Write, LCR[7] == 1]
#define UART_DLL 0x0u

// DLM: Divisor latch (most significant byte) register [Read/Write, LCR[7] == 1]
#define UART_DLM 0x4u

volatile uint32_t *uart_base_ptr = (uint32_t *)((uint32_t)(0x60000000 | 0x1000));


/**
 * Find minimum divisor divides in_freq to max_target_hz;
 * Based on uart driver n SiFive FSBL.
 *
 * f_baud = f_in / (div + 1) => div = (f_in / f_baud) - 1
 * The nearest integer solution requires rounding up as to not exceed max_target_hz.
 * div  = ceil(f_in / f_baud) - 1
 *	= floor((f_in - 1 + f_baud) / f_baud) - 1
 * This should not overflow as long as (f_in - 1 + f_baud) does not exceed
 * 2^32 - 1, which is unlikely since we represent frequencies in kHz.
 */
static inline unsigned int uart_min_clk_divisor(uint64_t in_freq,
						uint64_t max_target_hz)
{
	uint64_t quotient = (in_freq + max_target_hz - 1) / (max_target_hz);
	// Avoid underflow
	if (quotient == 0) {
		return 0;
	} else {
		return quotient - 1;
	}
}

static u32 get_reg(u32 num)
{
	return readl(uart_base + (num * 0x4));
}

//static void set_reg(u32 num, u32 val)
//{
//	writel(val, uart_base + (num * 0x4));
//}

void sifive_uart_putc(char ch)
{
	// while (get_reg(UART_REG_TXFIFO) & UART_TXFIFO_FULL)
	// 	;

	
	// set_reg(UART_REG_TXFIFO, ch);
	while(! (*(uart_base_ptr + UART_LSR) & 0x40u));
  	*(uart_base_ptr + UART_THR) = ch;
}

int sifive_uart_getc(void)
{
	u32 ret = get_reg(UART_REG_RXFIFO);
	if (!(ret & UART_RXFIFO_EMPTY))
		return ret & UART_RXFIFO_DATA;
	return -1;
}

int sifive_uart_init(unsigned long base, u32 in_freq, u32 baudrate)
{
	//uart_base     = (volatile uint32_t *)((uint32_t)(0x60000000 | 0x1000));


	uart_base     = (volatile void *)base;
	// uart_in_freq  = in_freq;
	// uart_baudrate = baudrate;

	// /* Configure baudrate */
	// set_reg(UART_REG_DIV, uart_min_clk_divisor(in_freq, baudrate));
	// /* Disable interrupts */
	// set_reg(UART_REG_IE, 0);
	// /* Enable TX */
	// set_reg(UART_REG_TXCTRL, UART_TXCTRL_TXEN);
	// /* Enable Rx */
	// set_reg(UART_REG_RXCTRL, UART_RXCTRL_RXEN);


	// set 0x0080 to UART.LCR to enable DLL and DLM write
  	// configure baud rate
	*(uart_base_ptr + UART_LCR) = 0x0080;

	// System clock 30 MHz, 115200 baud rate
	// divisor = clk_freq / (16 * Baud)
	*(uart_base_ptr + UART_DLL) = 30*1000*1000u / (16u * 115200u) % 0x100u;
	*(uart_base_ptr + UART_DLM) = 30*1000*1000u / (16u * 115200u) >> 8;

	// 8-bit data, 1-bit odd parity
	*(uart_base_ptr + UART_LCR) = 0x000Bu;

	// Enable read IRQ
	*(uart_base_ptr + UART_IER) = 0x0001u;

	// print "uart is working ..."
	//uart_demo();

	return 0;
}
