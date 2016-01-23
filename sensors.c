/* 
	Sample task that initialises the EA QVGA LCD display
	with touch screen controller and processes touch screen
	interrupt events.

	Jonathan Dukes (jdukes@scss.tcd.ie)
*/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lpc24xx.h"
#include <stdio.h>
#include <string.h>
#include "sensors.h"

#include "lcd.h"

#include "globals.h"

#define I2C_AA      0x00000004
#define I2C_SI      0x00000008
#define I2C_STO     0x00000010
#define I2C_STA     0x00000020
#define I2C_I2EN    0x00000040


/* Maximum task stack size */
#define sensorsSTACK_SIZE			( ( unsigned portBASE_TYPE ) 256 )

/* The LCD task. */
static void vSensorsTask( void *pvParameters );



void vStartSensors( unsigned portBASE_TYPE uxPriority )
{
	/* Enable and configure I2C0 */
	PCONP    |=  (1 << 7);                /* Enable power for I2C0              */

	/* Initialize pins for SDA (P0.27) and SCL (P0.28) functions                */
	PINSEL1  &= ~0x03C00000;
	PINSEL1  |=  0x01400000;

	/* Clear I2C state machine                                                  */
	I20CONCLR =  I2C_AA | I2C_SI | I2C_STA | I2C_I2EN;
	
	/* Setup I2C clock speed                                                    */
	I20SCLL   =  0x80;
	I20SCLH   =  0x80;
	
	I20CONSET =  I2C_I2EN;

	/* Spawn the console task . */
	xTaskCreate( vSensorsTask, ( signed char * ) "Sensors", sensorsSTACK_SIZE, NULL, uxPriority, ( xTaskHandle * ) NULL );

	printf("Sensor task started ...\r\n");
}


/* Get I2C button status */
unsigned char getButtons()
{
	unsigned char ledData;

	/* Initialise */
	I20CONCLR =  I2C_AA | I2C_SI | I2C_STA | I2C_STO;
	
	/* Request send START */
	I20CONSET =  I2C_STA;

	/* Wait for START to be sent */
	while (!(I20CONSET & I2C_SI));

	/* Request send PCA9532 ADDRESS and R/W bit and clear SI */		
	I20DAT    =  0xC0;
	I20CONCLR =  I2C_SI | I2C_STA;

	/* Wait for ADDRESS and R/W to be sent */
	while (!(I20CONSET & I2C_SI));

	/* Send control word to read PCA9532 INPUT0 register */
	I20DAT = 0x00;
	I20CONCLR =  I2C_SI;

	/* Wait for DATA with control word to be sent */
	while (!(I20CONSET & I2C_SI));

	/* Request send repeated START */
	I20CONSET =  I2C_STA;
	I20CONCLR =  I2C_SI;

	/* Wait for START to be sent */
	while (!(I20CONSET & I2C_SI));

	/* Request send PCA9532 ADDRESS and R/W bit and clear SI */		
	I20DAT    =  0xC1;
	I20CONCLR =  I2C_SI | I2C_STA;

	/* Wait for ADDRESS and R/W to be sent */
	while (!(I20CONSET & I2C_SI));

	I20CONCLR = I2C_SI;

	/* Wait for DATA to be received */
	while (!(I20CONSET & I2C_SI));

	ledData = I20DAT;

	/* Request send NAQ and STOP */
	I20CONSET =  I2C_STO;
	I20CONCLR =  I2C_SI | I2C_AA;

	/* Wait for STOP to be sent */
	while (I20CONSET & I2C_STO);

	return ledData ^ 0xf;
}



static portTASK_FUNCTION( vSensorsTask, pvParameters )
{
	portTickType xLastWakeTime;
	unsigned char buttonState;
	unsigned char lastButtonState;
	unsigned char changedState;
	unsigned int i;
	unsigned char mask;

	/* Just to stop compiler warnings. */
	( void ) pvParameters;

	printf("Starting sensor poll ...\r\n");

	/* initialise lastState with all buttons off */
	lastButtonState = 0;

	/* initial xLastWakeTime for accurate polling interval */
	xLastWakeTime = xTaskGetTickCount();
					 
	/* Infinite loop blocks waiting for a touch screen interrupt event from
	 * the queue. */
	while( 1 )
	{
		int n, x=0;
		int newlySelectedSequence;
		int found=0;
		/* Read buttons */
		buttonState = getButtons();

		changedState = buttonState ^ lastButtonState;
		
		if (buttonState != lastButtonState)
		{
		    /* iterate over each of the 4 LS bits looking for changes in state */
			for (i = 0; i <= 3; i++) {
				mask = 1 << i;
				if (changedState & mask) {
					printf("Button %u is %s\r\n", i, (buttonState & mask) ? "on" : "off");
					if(buttonState & mask){
						// Button 1 pressed - play sequence.
						if(i==0) {
							
							if (noteCount < sequence_steps && play == 1){
								T2TCR = 0x2;		/* Stop and reset TIMER2 */
								T3TCR = 0x2;		/* Stop and reset TIMER3 */
								play = 0;
							}
							else if (noteCount < sequence_steps && play == 0){
								T2TCR = 0x2;		/* Stop and reset TIMER2 */
								T3TCR = 0x2;		/* Stop and reset TIMER3 */
								T3TCR = 0x1;		/* Start TIMER3 */
								T2TCR = 0x1;		/* Start TIMER2 */
								play = 1;
							}
							else{
								noteCount = 0;
								T2TCR = 0x2;		/* Stop and reset TIMER2 */
								T3TCR = 0x2;		/* Stop and reset TIMER3 */
								T3TCR = 0x1;		/* Start TIMER3 */
								T2TCR = 0x1;		/* Start TIMER2 */
							}
						}
						// Button 4 pressed - change BPM.
						else if(i==3)
						{
							if (t3speed == 1500000)
							{
								t3speed = 12000000;
							}
							else
							{
								t3speed = t3speed/2;
							}
							if(mode == 1) {
								updateBPMVisuals();
							}
						}
						// Button 3 pressed - change mode.
						else if(i==2){
							if(mode) {
								mode = 0;
								setupScreen(32, 0, 0, 32, (240/12));
							}
							else {
								mode = 1;
								setupPlaybackMode();
							}
						}
						// Button 3 pressed - switch sequence from within edit mode.
						else if(i==1) {
							if(mode == 0){
								n=0;
								while(!found) {
									if(n==currentlySelectedSequence){
										// New sequence selected, must save the one that's currently in use to the listOfSequences[][] and listOfTimingSequences[][]
										// and then reinitialise the current sequence[] to the relative one in listOfSequences[][] and listOfTimingSequences[][].
										if(n==8) {
											newlySelectedSequence = 0;
										}
										else {
											newlySelectedSequence = currentlySelectedSequence+1;
										}
										for(x=0;x<sequence_steps;x++){
											listOfSequences[currentlySelectedSequence][x] = sequence[x];
											listOfTimingSequences[currentlySelectedSequence][x] = final_timing[x];
										}
										currentlySelectedSequence = newlySelectedSequence;
										for(x=0;x<sequence_steps;x++){
											sequence[x] = listOfSequences[currentlySelectedSequence][x];
											final_timing[x] = listOfTimingSequences[currentlySelectedSequence][x];
										}
										mode=2;
										found=1;
										n=8;
										setupScreen(32, 0, 0, 32, (240/12));
									}
									n++;
								}
							}
							else if (mode == 1) {
								noteCount = 0;
								if(octive == 1){
									octive = 2;
								}
								else{
									octive = 1;
								}
								updateOctaveVisuals();
							}
						}
					}
				}
			} 
			/* remember new state */
			lastButtonState = buttonState;
		}
        
        /* delay before next poll */
    	vTaskDelayUntil( &xLastWakeTime, 20);
	}
}
