#include "DAC.h"
#include "task.h"
#include "lcd.h"
#include "lcd_grph.h"
#include "lcd_hw.h"

#include "globals.h"
#include "Playback.h"

#define P210BIT ( ( unsigned long ) 0x4 )

/* 
 *  Interrupt handler for TIMER0 interrupts
 */
__irq void timer_handler(void)
{
	unsigned long currentState;
	T3IR = 0xFF; // Clear interrupt source (on the TIMER peripheral).
	if(noteCount == 10) // End.
	{
		T2TCR = 0x2;		/* Stop and reset TIMER2 */
		T3TCR = 0x2;		/* Stop and reset TIMER3 */
		if(mode) {
			displayNoteBars();
		}
	}
	else {
		if (final_timing[noteCount] == 0) { // No note.
			T2MR0 = 0;
			T3MR0  = t3speed;
			if(mode) {
				displayNoteBars();
			}
		}
		else
		{
			if(mode) {
				displayNoteBars();
				lcd_fillRect(0, (noteCount*31)+5, ((sequence[noteCount]+1)*11), (noteCount*31)+31, NAVY);
			}
			T3MR0  = t3speed/final_timing[noteCount]; // Note time size.
			if(sequence[noteCount] < numNotes) {	// Set note frequency.
				T2MR0  = octive*notes[sequence[noteCount]];
			}
		}
		if(noteCount < sequence_steps) {
			noteCount++;
			T2TCR = 0x2;		/* Stop and reset TIMER2 */
			T3TCR = 0x2;		/* Stop and reset TIMER3 */
			T2TCR = 0x1;		/* Start TIMER2 */
			T3TCR = 0x1;		/* Start TIMER3 */
		}
		else {
			noteCount = 0;
			T2TCR = 0x2;		/* Stop and reset TIMER2 */
			T3TCR = 0x2;		/* Stop and reset TIMER3 */
		}
		// Invert LED state
		currentState = (FIO2PIN1 & P210BIT);
		if (currentState)
		{
				FIO2CLR1 = P210BIT;
		}
		else
		{
				FIO2SET1 = P210BIT;
		}
	}
	// Acknowledge VIC interrupt (on the VIC)
	VICVectAddr = 0;
}

void Playing(void) {
	PCONP |= (1 << 23);
	
	/* Configure LED GPIO pin (P2.10) */
	PINSEL4 &= ~(3<<20);
	FIO2DIR1 = P210BIT;

	/* Configure TIMER0 to count for 1 second */
	T3IR = 0xFF;		/* Clear any previous interrupts */
	T3TCR = 0x2;		/* Stop and reset TIMER0 */
	T3CTCR = 0x0;		/* Timer mode */
	T3MR0 = 6000000;	/* Match every 0.5 seconds */
	T3MCR = 0x3;		/* Interrupt, reset and re-start on match */
	T3PR = 0x0;			/* Prescale = 1 */

	/* Configure vector 4 (TIMER0) for IRQ */
	VICIntSelect &= ~(1 << 27);
	
	/* Set vector priority to 8 (mid-level priority) */
	VICVectPriority27 = 8;
	
	/* Set handler vector. This uses a "function pointer" which is just
	 * the address of the function. The function is defined above.*/
	VICVectAddr27 = (unsigned long)timer_handler;
	
	/* Enable interrupts on vector 4. Note when writing to this register, 0s
	 * haveno effect, 1s cause the corresponding vector to be enabled.
	 * (Avoids need to Read-Modify-Write. Use VICIntEnClear to disable). */
	VICIntEnable = 1 << 27;
	
	T3TCR = 0x1;		/* Start TIMER0 */
}