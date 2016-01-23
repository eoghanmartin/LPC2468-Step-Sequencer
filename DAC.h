#include <stdlib.h>
#include "FreeRTOS.h"
#include "lpc24xx.h"

#define DAC_CLOCK_FREQ 12000000   // The frequency of the system clock is 12MHz

/* The DAC has 10-bit precision, meaning the amplitude of the wave it generates
 * can be in the range 0 (silent) up to 2^10 (max amplitude).
 */

#define   PEAK_AMPL 1023
#define TROUGH_AMPL 0

/* Note values (in Hz) */

#define C_4 261.626
#define C_4s 277.18 
#define D_4 293.665
#define D_4s 311.13 
#define E_4 329.628
#define F_4  349.228
#define F_4s  369.99 
#define G_4  391.995
#define G_4s  415.30
#define A_4  440
#define A_4s  466.16 
#define B_4  493.88

void DACHardwareSetup(void);

unsigned long convertNoteToTicks(float freq);

__irq void square_wave_handler(void);

__irq void timer_handler(void);
