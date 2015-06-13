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
#include <util/delay.h>
#include <util/setbaud.h>

#include <stdbool.h>
#include <stdint.h>


#define STROBE_DDR	DDRB
#define STROBE_PORT	PORTB
#define STROBE_PIN	PB4

#define IND_DDR		DDRB
#define IND_PORT	PORTB
#define IND_PIN		PB5

#define RCV_DDR		DDRD
#define RCV_PORT	PORTD
#define RCV_PIN		PD2


/*
 * A 38 kHz cycle requires the strobe is toggled every 13ms; multiplied
 * by two cycles per microsecond yields 26.
 */
#define STROBE_CYCLE	26

/*
 * We'll perform 76 toggles: this turns out to be right around 1.1ms. This
 * is 1000 microseconds (1 millisecond) divided by 13 microseconds per
 * toggle. This gives us 77 ticks, but we'll subtract one to account for
 * minor delays in processing. An even number also ensures we end with the
 * strobe off; this could also be accomplished by explicitly turning the
 * strobe off at the end.
 */
#define MAX_TICKS	74


#define toggle_bit(port, pin)	port ^= _BV(pin)


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



/*
 * setup_strobe prepares Timer1 and PCINT2 for use, and sets up the
 * relevant pins.
 */
static void
setup_strobe(void)
{
	/* The timer should be in CTC mode. */
	TCCR1A |= _BV(WGM01);

	/* Use a prescaler of 8. */
	TCCR1B |= _BV(CS01);

	TCNT1 = 0;	/* Reset the counter. */
	TIFR1 |= _BV(OCF1A);
	TIMSK1 |= _BV(OCIE1A);

	/* Enable only PCINT18 in the PCINT2 register. */
	PCMSK2 = _BV(PCINT18);

	/* Set up the pins. */
	STROBE_DDR |= _BV(STROBE_PIN);
	IND_DDR |= _BV(IND_PIN);

	/*
	 * The default for a port is to be in input mode. However,
	 * the pullup resistor needs to be enabled.
	 */
	RCV_PORT |= _BV(RCV_PIN);
	sei();
}


/*
 * alarm is set to true if the IR strobe detected an object. It is
 * reset each time the strobe is fired.
 */
static volatile bool	alarm = false;

static void
strobe(void)
{
	/* Enable PCINT18. */
	PCIFR |= _BV(PCIF2);	/* Drop any pending interrupts. */
	PCICR |= _BV(PCIE2);	/* Enable PC interrupt bank 2. */

	/* Reset the alarm. */
	alarm = false;

	/* Reset Timer1. */
	TCNT1 = 0;		/* Reset the counter. */
	TIMSK1 |= _BV(OCIE1A);	/* Drop any pending interrupts. */
	OCR1A = STROBE_CYCLE;

	/*
	 * Activate Timer0. PRR is the power-reduction register; any
	 * cleared timer bits will enable that timer, while any set
	 * bits will disable it.
	 */
	PRR &= ~_BV(PRTIM1);
}


/*
 * The strobe's ISR handles setting up the 38kHz strobe and triggering
 * it for roughly 10ms.
 */
ISR(TIMER1_COMPA_vect)
{
	static uint8_t	ticks = 0;

	/*
	 * Once we've reached the maximum number of ticks, stop
	 * the timer and disable the port change interrupt.
	 */
	if (ticks == MAX_TICKS) {
		PRR |= _BV(PRTIM1);	/* Put Timer0 in powersave mode. */
		PCICR &= ~_BV(PCIE2);	/* Disable PCINT2. */
		PCIFR |= _BV(PCIF2);	/* Drop pending PCINT2 interrupts. */

		ticks = 0;		/* Reset tick counter. */
	}
	/*
	 * Otherwise, toggle the strobe and increase the tick count.
	 */
	else {
		toggle_bit(STROBE_PORT, STROBE_PIN);

		/* Reset the timer count and trigger on next cycle. */
		TCNT1 = 0;
		OCR1A = STROBE_CYCLE;
		ticks++;
	}
}


/*
 * If the receiver pin changes, it's a transition from high to low, so
 * the alarm needs to be set.
 */
ISR(PCINT2_vect)
{
	alarm = true;
}


int
main(void)
{
	init_UART();
	setup_strobe();
	write_string("Boot OK.");
	newline();

	sei();

	while (1) {
		strobe();

		/*
		 * Give the strobe time to register its results.
		 */
		_delay_ms(20);

		/*
		 * If the alarm has been triggered, turn on the indicator
		 * LED.
		 */
		if (alarm) {
			write_string("object detected");
			newline();
			IND_PORT |= _BV(IND_PIN);
		}
		/*
		 * Otherwise, make sure the LED is off.
		 */
		else {
			IND_PORT &= ~_BV(IND_PIN);
		}

		/*
		 * Finish delaying for 100ms.
		 */
		_delay_ms(80);
	}

	return 0;
}
