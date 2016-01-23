#include <stdlib.h>
#include "FreeRTOS.h"
#include "lpc24xx.h"
#include "DAC.h"
#include "task.h"

#include "globals.h"
	
void DACHardwareSetup(void) {
	/* Configure the DAC */
  PINSEL1 &= ~(3 << 20);    /* Enable P0.26 for AOUT function */
  PINSEL1 |= (2 << 20);

  /* Configure TIMER2  */

  PCONP |= (1 << 22); // Enable power for Timer 2 (bit 22 of PCONP register, user manual p.65/66)

  T2IR   = 0xFF;  /* Clear any previous interrupts */
  T2TCR  = 0x2;   /* Stop and reset TIMER0 */
  T2CTCR = 0x0;   /* Timer mode */
	T2TCR  = 0x1;   /* start timer2*/
  T2MR0  = 0x0; 	// Play not sound on start-up.

  T2MCR  = 0x3;   /* Interrupt, reset and re-start on match */
  T2PR   = 0x0;   /* No prescaling */

  VICIntSelect &= ~(1 << 26); /* Slot 26 is for Timer 2 (user manual p.118) */
  VICVectPriority26 = 8;      /* Medium priority */
  VICVectAddr26 = (unsigned long) square_wave_handler;
  VICIntEnable |= 1 << 26;
}

__irq void square_wave_handler(void) {

  /*
   * Every time we get an interrupt, we need to switch from the low amplitude to
   * the high amplitude, or vice versa, to generate a square wave.
   */

  T2IR = 0xFF;  /* Clear the interrupt on TIMER2 */

  /* Note the values below are all shifted to the right by 6 bits.
   * This is because bits [15:6] of the DACR register are used to set the value (user manual p.675)
   * We have to shift our 10-bit amplitude value 6 places to the right
   * to make sure the bits are in the correct position in the DACR register.
   */

  switch (DACR & (PEAK_AMPL << 6)) { /* We use a logical AND to zero out the bits we don't care about */

    /* If the amplitude is currently zero, set it to the PEAK_AMPL value */
    case 0:
      DACR = (PEAK_AMPL << 6);
      break;

    /* If the amplitude is anything else, set it to the TROUGH_AMPL value */
    default:
      DACR = (TROUGH_AMPL << 6);
      break;
  }
	T2TCR  = 0x2;   /* Stop and reset TIMER0 */
	T2TCR  = 0x1;   /* Stop and reset TIMER0 */
	

  VICVectAddr = 0; /* Acknowledge VIC interrupt (on the VIC) */
}