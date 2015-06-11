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


#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>


#define LED_DDR		DDRB
#define LED_PORT	PORTB
#define LED_PIN		PB5


/*
 * init_UART brings up the serial port.
 */
static void
init_UART(void)
{
	/*
	 * The UBRR register is a UART baud rate register. We're using
	 * UART 0. It's a 16-bit register, and we need to set the high
	 * and low bytes to the values automatically provided for us by
	 * the util/setbaud.h header.
	 */
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;

	/*
	 * UART control status registers (or UCSR0 for UART 0) control
	 * the UART's operation. There are three such registers, labeled
	 * A, B, and C.
	 */

	/*
	 * In UCSR0A, we want to disable 2x transmission speed.
	 */
	UCSR0A &= ~(1 << U2X0);

	/*
	 * Enable the transmitter (transmit enable 0) and receiver
	 * (receive enable 0).
	 */
	UCSR0B = ((1 << TXEN0)|(1 << RXEN0));

	/*
	 * Now, we set the framing to the most common format: 8 data bits,
	 * one stop bit.
	 */
	UCSR0C = ((1 << UCSZ01)|(1 << UCSZ00));
}


/*
 * write_string writes the string to the serial port.
 */
static void
write_string(char *s)
{
	int	i = 0;

	while (s[i] != 0) {
		loop_until_bit_is_set(UCSR0A, UDRE0);
		UDR0 = s[i];
		i++;
	}
}


/*
 * serial_read blocks until a character is available from the UART,
 * returning that character when it is received.
 */
char
serial_read(void)
{
	/*
	 * Similar to the write_string function, UCSR0A's RXC0 bit will
	 * be set when a byte is ready to be read from the serial port.
	 */
	loop_until_bit_is_set(UCSR0A, RXC0);

	/*
	 * The contents of the UDR0 register have the byte that was
	 * read in.
	 */
	return UDR0;
}


/*
 * newline sends a CR-LF over the serial port.
 */
static void
newline(void)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = '\r';

	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = '\n';
}


int
main(void)
{
	/* Set the LED pin as an output pin. */
	LED_DDR |= _BV(LED_PIN);

	/* Set up the serial port. */
	init_UART();

	/* The Arduino is now booted up and ready. */
	write_string("Boot OK.");
	newline();

	while (1) {
		/* Toggle the LED. */
		LED_PORT ^= _BV(LED_PIN);

		/* Sleep for one second. */
		_delay_ms(1000);
	}

	/*
	 * This should never be reached, but C requires that
	 * main possibly returns *something*.
	 */
	return 0;
}

