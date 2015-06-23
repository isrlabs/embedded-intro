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
#include <avr/interrupt.h>


#define LED_DDR		DDRB
#define LED_PORT	PORTB
#define LED_PIN		PB5

#define BLINK_INTERVAL	15630


static void
init_timer1(void)
{
	/*
	 * Set the waveform generation mode to CTC with OCR1A as the
	 * top value.
	 */
	TCCR1B |= _BV(WGM12);

	/* Use a prescaler of 1024. */
	TCCR1B |= _BV(CS12) | _BV(CS10);

	/* Trigger an interrupt on output compare A. */
	TIMSK1 |= _BV(OCIE1A);

	/* Set the output compare register to the update interval. */
	OCR1A = BLINK_INTERVAL;

	/* Enable interrupts. */
	sei();
}


int
main(void)
{
	init_timer1();

	LED_DDR |= _BV(LED_PIN);

	while (1) {}

	return 0;
}


ISR(TIMER1_COMPA_vect)
{
	LED_PORT ^= _BV(LED_PIN);
}
