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
 * This is the same program as in 01_getting_started, minus the UART
 * bits and swapping the avr/io.h _BV macro for our io.h's BV macro.
 */


#include "io.h"
#include <util/delay.h>


#define LED_DDR         DDRB
#define LED_PORT        PORTB
#define LED_PIN         PB5


int
main(void)
{
	/* Set the LED pin as an output pin. */
	LED_DDR |= BV(LED_PIN);

	while (1) {
		/* Toggle the LED. */
		LED_PORT ^= BV(LED_PIN);

		/* Sleep for one second. */
		_delay_ms(1000);
	}

	/*
	 * This should never be reached, but C requires that
	 * main possibly returns *something*.
	 */
	return 0;
}
