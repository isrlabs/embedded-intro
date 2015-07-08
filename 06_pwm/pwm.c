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

// The PWM subsystem uses Timer1.
static void
initTimer1(void)
{
	// Disable powersaving mode on Timer1 to enable it.
	PRR0 &= ~_BV(PRTIM1);

	TCCR1A = 0;
	TCCR1B = _BV(CS11);	// Set prescaler to 8.

	// Now, Timer1 must be prepared for use.
	TCNT1 = 0;		// Reset the timer counter.
	TIFR1 |= _BV(OCF1A);	// Drop any existing interrupts on the timer.
	TIMSK1 |= _BV(OCIE1A);	// Enable Timer1's output compare interrupt.
}

// The PWM code takes the same approach as the Arduino code: OC1A is
// used as a tick counter and pins are manually strobed. The previous
// approach of using OC1A and OC1B was leading to the motors having
// different speeds.

// All servos are attached to port B. See the hardware documentation for
// more details.


#define DUTY_CYCLE	20000	// Servo duty cycle is 20ms.
#define UPDATE_INTERVAL	40000
#define UPDATE_WAIT	5	// Allow some delay for updates.

// The following pulse ranges are specified in the servo datasheet.

// Note: drivetrain servos (which are continuous rotation) define
// 1.3ms for their minimum; 1.5ms for the stop position, and 1.7ms
// for their maximum.
#define MIN_PULSE	1300	// 1.3ms for full backward rotation.
#define MID_PULSE	1500	// 1.5ms stop pulse.
#define MAX_PULSE	1700	// 1.7ms for full forward rotation.


// A servo collects relevant information about a collected servo. All
// servos are connected to port B, so the pin is relative to PORTB.
struct servo {
	uint8_t		pin;
	uint32_t	tcnt;	// Tick counter
	uint16_t	min;
	uint16_t	max;
	int16_t		trim;	// Adjust for variances in an individual servo.
};


// The servos variable stores all the servos connected to the board.
#define ACTIVE_SERVOS	2
static struct servo	servos[ACTIVE_SERVOS] = {
	{0, MID_PULSE, 0, 0, 0},
	{0, MID_PULSE, 0, 0, 0}
};
static volatile int8_t	active = 0;


void
pwm::connect(uint8_t which, uint8_t pin)
{
	// Verify servo is active.
	if (which >= ACTIVE_SERVOS) {
		return;
	}

	servos[which].pin = pin;
	servos[which].min = MIN_PULSE;
	servos[which].max = MAX_PULSE;
	servos[which].trim = 0;
}

void
pwm::set_limits(uint8_t which, uint16_t min, uint16_t max)
{
	// Verify servo is active.
	if (which >= ACTIVE_SERVOS) {
		return;
	}

	if (min > 0) {
		servos[which].min = min;
	}

	if (max > 0) {
		servos[which].max = max;
	}
}


void
pwm::trim(uint8_t which, int16_t trim)
{
	// Verify servo is active.
	if (which >= ACTIVE_SERVOS) {
		return;
	}

	servos[which].trim = trim;
}


static inline uint16_t
servo_max(uint8_t which)
{
	return servos[which].max;
}


static inline uint16_t
servo_min(uint8_t which)
{
	return servos[which].min;
}


void
pwm::set_servo(uint8_t which, uint32_t us)
{
	uint8_t	saved_SREG;

	// Verify that a valid servo is being addressed.
	if (which >= ACTIVE_SERVOS) {
		return;
	}

	us += servos[which].trim;

	// Verify that pulse time is within acceptable limits.
	if (us < servos[which].min) {
		us = servos[which].min;
	}
	else if (us > servos[which].max) {
		us = servos[which].max;
	}

	us *= 2;
	saved_SREG = SREG;
	cli();
	servos[which].tcnt = us;
	SREG = saved_SREG;
	sei();
}


uint16_t
pwm::get_servo(uint8_t which)
{
	// Verify that a valid servo is being addressed.
	if (which >= ACTIVE_SERVOS) {
		return 0;
	}

	return (uint16_t)servos[which].tcnt;
}


// The following are the interrupt handlers for timer 1.
static void
timer1ISR(void)
{
	if (active < ACTIVE_SERVOS) {
		// Pulse active pin low.
		PORTB &= ~_BV(servos[active].pin);
	}
	// The PWM cycle is complete, reset timer.
	else {
		active = -1;
		TCNT1 = 0;
	}

	active++;
	if (active < ACTIVE_SERVOS) {
		OCR1A = TCNT1 + servos[active].tcnt;
		PORTB |= _BV(servos[active].pin);
	}
	// Pulsing complete, don't pulse until the update interval is over.
	else {
		if ((TCNT1 + UPDATE_WAIT) < UPDATE_INTERVAL) {
			OCR1A = UPDATE_INTERVAL;
		}
		else {
			OCR1A = TCNT1 + UPDATE_WAIT;
		}
	}
}


// Install the ISR.
ISR(TIMER1_COMPA_vect)
{
	timer1ISR();
}

