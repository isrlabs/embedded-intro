/*
 * Copyright (c) 2015 Kyle Isom <coder@kyleisom.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * This file accompanies an illustration of where the avr/io.h names
 * for things come from.
 */


#ifndef __IO_H
#define __IO_H


/*
 * The argument should be the address of an 8-bit register. A description
 * and listing of memory locations is given in section 30 of the ATmega328P
 * data sheet.
 */
#define REGISTER8_ADDR(addr)	(*((volatile uint8_t *)(addr)))


/*
 * Page 426 gives the memory locations for PORTB and DDRB.
 */
#define PORTB_ADDR	0x25
#define DDRB_ADDR	0x24


/*
 * Now we apply the memory addresses.
 */
#define PORTB	REGISTER8_ADDR(PORTB_ADDR)
#define DDRB	REGISTER8_ADDR(DDRB_ADDR)


/*
 * We define the bits in PORTB.
 */
#define PB0	0
#define PB1	1
#define PB2	2
#define PB3	3
#define PB4	4
#define PB5	5
#define PB6	6
#define PB7	7


/*
 * This is a convenience macro for getting a bit value.
 */
#define BV(x)	(1 << x)


#endif
