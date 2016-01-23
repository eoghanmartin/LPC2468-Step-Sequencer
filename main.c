/* Standard includes. */
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "console.h"

#include "lcd.h"
#include "DAC.h"
#include "lcd_hw.h"
#include "lcd_grph.h"
#include "sensors.h"

#include "Playback.h"
#include "globals.h"

extern void vLCD_ISREntry( void );

// Initialising globals.h variables.
int const sequence_steps = 10;
int const numNotes = 12;
float notes[numNotes] = {C_4, C_4s, D_4, D_4s, E_4, F_4, F_4s, G_4, G_4s, A_4, A_4s, B_4};

int listOfSequences[9][sequence_steps];
int listOfTimingSequences[9][sequence_steps];
int currentlySelectedSequence = 0;

int sequence[sequence_steps];

int timing[numNotes][sequence_steps];
int final_timing[sequence_steps];
int play = 1;
int i, t = 0;
int t3speed = 6000000;
int octive = 1;

int noteCount = 0;
int mode = 0;

#define DAC_CLOCK_FREQ 12000000   // The frequency of the system clock is 12MHz
/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
//__irq void octive_button_handler(void);
//void octiveButton(void);
unsigned long convertNoteToTicks(float freq);


int main (void)
{
	// Initialise all notes array to correct frequencies.
	for ( i = 0; i < numNotes; i++ ) {
			notes[i] = convertNoteToTicks(notes[i]);
	}
	// Initialise the array of sequences to 12 (no note) and note size arrays to 0 (no note).
	for ( i = 0; i < 9; i++ ) {
		for ( t = 0; t < sequence_steps; t++ ) {
			listOfSequences[i][t] = numNotes;
			listOfTimingSequences[i][t] = 0;
		}
	}
	// Initiallise live sequence[] and note size array.
	for ( i = 0; i < sequence_steps; i++ ) {
			sequence[i] = listOfSequences[0][i];
			final_timing[i] = 0;
	}
	// Currently using the 1st sequence in the array of sequences and note sizes.
	currentlySelectedSequence = 0;
	// INitiallised to the first note for playing.
	noteCount = 0;
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();
	/* Start the console task */
	vStartConsole(1, 19200);
	
	// Start the sensors task setting up interupts on the 4 I2C buttons.
	vStartSensors(2);
	
	/* Start the lcd task */
	vStartLcd(1);
	
	// Setup DAC for playing sounds and start playing to setup all of the timers and interruptes. Will play through the initialised sequence (no notes) at start-up.
	DACHardwareSetup();
	Playing();

	/* Start the FreeRTOS Scheduler ... after this we're pre-emptive multitasking ...

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	while(1);
}

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */
	
	/* Enable UART0. */
	PCONP   |= (1 << 3);                 /* Enable UART0 power                */
	PINSEL0 |= 0x00000050;               /* Enable TxD0 and RxD0              */

	/* Initialise LCD hardware */
	lcd_hw_init();

	/* Setup LCD interrupts */
	PINSEL4 |= 1 << 26;				/* Enable P2.13 for EINT3 function */
	EXTMODE |= 8;					/* EINT3 edge-sensitive mode */
	EXTPOLAR &= ~0x8;				/* Falling edge mode for EINT3 */

	/* Setup VIC for LCD interrupts */
	VICIntSelect &= ~(1 << 17);		/* Configure vector 17 (EINT3) for IRQ */
	VICVectPriority17 = 15;			/* Set priority 15 (lowest) for vector 17 */
	VICVectAddr17 = (unsigned long)vLCD_ISREntry;
									/* Set handler vector */
}
unsigned long convertNoteToTicks(float freq) {

  /*
   * We have the desired note frequency in Hz (float freq).
   * The TIMER devices count time ticks at the system clock speed (12MHz)
   * We need to return the number of system clock ticks corresponding to
   * the frequency we want to generate.
   */

  return (unsigned long) ((float) DAC_CLOCK_FREQ / (2 * freq));
}


