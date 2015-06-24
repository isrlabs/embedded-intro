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

#include <stdio.h>


/*
 * The ultrasonic ranging sensor is connected to analog input 3,
 * which is PC3.
 */
#define URS_CHANNEL	3


/*
 * Timer1 is set up with a prescaler of 8, which means 2 cycles
 * per microsecond. The URS can range once every 49ms, which translates
 * to a timer value of 98000.
 */
#define URS_CYCLE	98000


struct reading {
	/* val contains the last measurement from the ADC. */
	uint8_t		val;

	/* count stores the number of conversions that have occurred. */
	uint16_t	count;
};
volatile struct reading	sensor = {0, 0};


static void
init_ADC(void)
{
	/* Use Vcc (the main power supply) as the reference. */
	ADMUX = _BV(REFS0);

	/* Left-align the results, which gives 8-bit precision. */
	ADMUX |= _BV(ADLAR);

	ADMUX |= URS_CHANNEL;

	/*
	 * We really don't need a high sample rate, so we use a
	 * high prescale. A prescale of 128 with a 16 MHz clock
	 * means it samples at a rate of 125 kHz.
	 */
	ADCSRA = _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);

	/* Enable the ADC. */
	ADCSRA |= _BV(ADEN);

	/* Disable digital inputs on the URS channel. */
	DIDR0 |= _BV(URS_CHANNEL);

	/* Kick off the first conversion. */
	ADCSRA |= _BV(ADSC);
}


static void
init_timer1(void)
{
	/*
	 * Set the waveform generation mode to CTC with OCR1A as the
	 * top value.
	 */
	TCCR1B |= _BV(WGM12);

	/* Use a prescaler of 8. */
	TCCR1B |= _BV(CS11);

	/* Trigger an interrupt on output compare A. */
	TIMSK1 |= _BV(OCIE1A);

	/* Set the output compare register to the update interval. */
	OCR1A = URS_CYCLE;
}


/*
 * The Timer1 ISR triggers an ADC conversion every URS_CYCLE ticks.
 */
ISR(TIMER1_COMPA_vect)
{
	/* Clear pending ADC interrupts. */
	ADCSRA |= _BV(ADIF);

	/* Select the URS channel. */
	ADMUX = (ADMUX & 0xF8) | URS_CHANNEL;

	/* Trigger an ADC conversion. */
	ADCSRA |= _BV(ADSC);

	/* Enable the ADC interrupt. */
	ADCSRA |= _BV(ADIE);
}


ISR(ADC_vect)
{
	/*
	 * Read the distance measurement into the sensor readout
	 * structure.
	 */
	sensor.val = ADCH;
	sensor.count++;

	/*
	 * Turn off the ADC interrupt and clear any pending
	 * interrupts.
	 */
	ADCSRA |= _BV(ADIF);
	ADCSRA &= ~_BV(ADIE);
}


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


int
main(void)
{
	char	buf[32];

	init_ADC();
	init_timer1();
	init_UART();
	sei();

	write_string("Boot OK.\r\n");

	while (1) {
		_delay_ms(1001);
		snprintf(buf, 31, "URS reading #%5u: %u\r\n",
		    sensor.count, sensor.val);
		write_string(buf);
	}

	return 0;
}


#define CHANNEL_COUNT	3
static uint8_t	channels[CHANNEL_COUNT] = {ADC0, ADC2, ADC3};
static uint16_t	readings[CHANNEL_COUNT] = {0, 0, 0};
static uint8_t  channel = 0;


ISR(ADC_vect)
{
	readings[channel] = ADC;
	/*
	 * If there are channels still left to be read, then
	 * set the ADC to the new channel and kick off a new
	 * conversion.
	 */
	if (channel != sizeof(channels)) {
		channel++;
		/*
		 * Clear the channel selection bits and set the active
		 * channel.
		 */
		ADMUX = (ADMUX & 0xF8) | channels[channel];
		ADCSRA |= _BV(ADSC);
	}
	/*
	 * If all the channels have been read, then disable the
	 * interrupt.
	 */
	else {
		/*
		 * Turn off the ADC interrupt and clear any pending
		 * interrupts.
		 */
		ADCSRA |= _BV(ADIF);
		ADCSRA &= ~_BV(ADIE);
	}
}

