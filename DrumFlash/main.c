/*
 * main.c
 *
 *  Created on: Apr 1, 2014
 *      Author: walberts
 */

#include <avr/io.h>
#include <avr/interrupt.h>
// HSI
//
// light output     PB3   (2)
// debug0           PB2   (7)
// debug1           PB4   (3)
// sound reference  AIN1  (6)
// sound input      AIN0  (5)
// SPI MOSI         PB0   (5)
// SPI MISO         PB1   (6)
// SPI SCK          PB2   (7)
// gnd                    (4)
// vcc                    (8)

#define CLOCKFREQUENCY (1000000)
#define PRESCALER      (1024)

#define FLASHDURATION (100)
#define DEAFDURATION  (400)

#define DBG_STARTED   (0)
#define DBG_IDLE      (1)
#define DBG_TRIGGERED (2)
#define DBG_DEAF      (3)

typedef void(*TimerCB)();

void enableInputEvent();
void disableInputEvent();

void activateTimer(int ms, TimerCB cb);

void lightOn();
void lightOff();

void handleSoundHeard();
void handleEndFlashTimer();
void handleEndDeafTimer();

void initialize();
void debugOut(int state);
void halt();

TimerCB wakeupCB = 0;

int main()
{
	debugOut(DBG_STARTED);
	initialize();
	enableInputEvent();
	debugOut(DBG_IDLE);
	halt();
}

void halt()
{
	// Todo: look for a better way to halt.
	while (1);
}

ISR(TIMER1_COMPA_vect)
{
	// Deactivate Timer
	TCCR1 &= ~(_BV(CS13) |_BV(CS12) |_BV(CS11) |_BV(CS10));

	// Invoke registered callback
	if (wakeupCB==0)
		wakeupCB();
}

ISR(ANA_COMP_vect)
{
	handleSoundHeard();
}

void initialize()
{
	cli();

	//


	// Set clock prescaler such that we run on 1 Mhz
	CLKPR = _BV(CLKPCE);
	CLKPR = _BV (CLKPS1) | _BV(CLKPS0);

	// Set light output (PB4) to output
	// (default all pins are set to output

	// Set sound inputs to input
	// Turn off pull up resistor
	// sound reference  AIN1  (6)
	// sound input      AIN0  (5)
	DDRB  &= ~(_BV(DDB1)|_BV(DDB0));
	PORTB &= ~(_BV(PORTB1)|_BV(PORTB0));

	// Set comparator to trigger on flank.
	ACSR &= ~_BV(ACIE);
	ACSR = (ACSR & ~_BV(ACIS1)) | _BV(ACIS0);

	sei();
}

void enableInputEvent()
{
	// enable pin driven interrupt
	ACSR |= _BV(ACIE);
}


void disableInputEvent()
{
	// Disable pin driven interrupt
	ACSR = (ACSR & ~_BV(ACIE)) | _BV(ACD);
}

void activateTimer(int ms, TimerCB cb)
{
	wakeupCB = cb;

	// Calculate timer compare value
	uint8_t cmp = CLOCKFREQUENCY/PRESCALER/1000 * ms;

	// Set compare value and start timer
	OCR1A = cmp;
	TCNT1 = 0;
	TIMSK |= _BV(OCIE1A);
	TCCR1 |= _BV(CTC1) | _BV(CS13) |_BV(CS12) |_BV(CS11) |_BV(CS10);
}


void handleSoundHeard()
{
	disableInputEvent();

	lightOn();
	activateTimer(FLASHDURATION, handleEndFlashTimer);
	debugOut(DBG_TRIGGERED);
}

void handleEndFlashTimer()
{
	lightOff();
	activateTimer(DEAFDURATION, handleEndDeafTimer);
	debugOut(DBG_DEAF);
}

void handleEndDeafTimer()
{
	enableInputEvent();
	debugOut(DBG_IDLE);
}

void lightOn()
{
	// Light: PB3
	PORTB |= _BV(PORTB3);
}

void lightOff()
{
	// Light: PB3
	PORTB &= ~_BV(PORTB3);
}

void debugOut(int val)
{
	// debug0           PB2   (7)
	// debug1           PB4   (3)

	// Clear all bits
	PORTB &= ~( _BV(PORTB2) | _BV(PORTB4));
	switch (val) {
	case 0:
		// NOthing
		break;
	case 1:
		PORTB |= _BV(PORTB2);
		break;
	case 2:
		PORTB |= _BV(PORTB4);
		break;
	case 3:
		PORTB |= _BV(PORTB2) | _BV(PORTB4);
		break;
	default:
		break;
	}
}

