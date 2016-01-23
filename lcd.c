/* 
	Sample task that initialises the EA QVGA LCD display
	with touch screen controller and processes touch screen
	interrupt events.

	Jonathan Dukes (jdukes@scss.tcd.ie)
*/

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "lcd.h"
#include "lcd_hw.h"
#include "lcd_grph.h"
#include <string.h>
#include <stdio.h>

#include "globals.h"

/* Maximum task stack size */
#define lcdSTACK_SIZE	( ( unsigned portBASE_TYPE ) 256 )
	
/* The LCD task. */
static void vLcdTask( void *pvParameters );

/* Interrupt handlers */
extern void vLCD_ISREntry( void );
void vLCD_ISRHandler( void );

/* Semaphore for ISR/task synchronisation */
xSemaphoreHandle xLcdSemphr;

int k, l;

void vStartLcd( unsigned portBASE_TYPE uxPriority )
{
	vSemaphoreCreateBinary(xLcdSemphr);

	/* Spawn the console task. */
	xTaskCreate( vLcdTask, ( signed char * ) "Lcd", lcdSTACK_SIZE, NULL, 
		uxPriority, ( xTaskHandle * ) NULL );
}

/*** EDIT SETUP **/
// Renders the screen to the correct layout for edit mode using the current sequence[] and final_timing[] arrays.
void setupScreen(int step_width, int left, int bottom, int right, int top)
{
	right = step_width;
	step_width = 320/sequence_steps;
	// Filling the full screen will cause a flicker if this function is over used. Therefore, used as a setup function for changing modes.
	lcd_fillScreen(LIGHT_GRAY);
	// Iterate one row at a time and set cell colours and grid lines.
	for(k=0; k<numNotes; k++){
		for(l=0; l<sequence_steps; l++) {
			lcd_drawRect(bottom, left, top, right, DARK_GRAY);
			if(k == sequence[l]){
				// If a note is found in this row. Setup the relative cell with the corresponding note size colour value.
				if(final_timing[l] == 1){
						lcd_fillRect(bottom + 4, left + 4, top - 4, right - 4, MAROON);
				}
				else if(final_timing[l] == 2){
						lcd_fillRect(bottom + 4, left + 4, top - 4, right - 4, OLIVE);
				}
				else if(final_timing[l] == 4){
						lcd_fillRect(bottom + 4, left + 4, top - 4, right - 4, PURPLE);
				}
			}
			left = left + step_width;
			right = right + step_width;
		}
		left = 0;
		right = step_width;
		top = top + (240/12);
		bottom = bottom + (240/12);
	}
}

/*** PLAYBACK SETUP **/
// Renders the screen to the correct layout for playback mode using the current sequence[], currentlySelectedSequence variable and final_timing[] array.
void setupPlaybackMode () {
	// Filling the screen as this function is used as a setup function when changing modes.
	lcd_fillScreen(LIGHT_GRAY);
	lcd_fillRect(180, 0, 240, 320, DARK_GRAY);
	
	// Visually showing the sequence currently in use.
	for(k=0; k<9; k++) {
		if(currentlySelectedSequence == k) {
			lcd_fillRect(185, (35*k) + 5, 235, (35*k) + 35, MAGENTA);
		}
		else {
			lcd_fillRect(185, (35*k) + 5, 235, (35*k) + 35, LIGHT_GRAY);
		}
	}
	// Setting up the graphic bars for the sequence in use.
	displayNoteBars();
	// Setup BPM visual display.
	updateBPMVisuals();
	// Setup octave visual display.
	updateOctaveVisuals();
}

void displayNoteBars () {
	// Setting up the graphic bars for the sequence in use.
	for(k=0; k<sequence_steps; k++) {
		if(sequence[k] < 12){
			if(final_timing[k]==1){
				lcd_fillRect(0, (k*31)+5, ((sequence[k]+1)*11), (k*31)+31, MAROON);
			}
			else if(final_timing[k]==2){
				lcd_fillRect(0, (k*31)+5, ((sequence[k]+1)*11), (k*31)+31, OLIVE);
			}
			else if(final_timing[k]==4){
				lcd_fillRect(0, (k*31)+5, ((sequence[k]+1)*11), (k*31)+31, PURPLE);
			}
		}
	}
}

void updateBPMVisuals () {
	// Setup BPM visual display.
	if(t3speed == 12000000) {
		lcd_fillRect(160, 5, 170, 30, DARK_CYAN);
		lcd_fillRect(160, 31, 170, 136, LIGHT_GRAY);
	}
	else if(t3speed == 6000000) {
		lcd_fillRect(160, 5, 170, 30, DARK_CYAN);
		lcd_fillRect(160, 35, 170, 65, DARK_CYAN);
		lcd_fillRect(160, 66, 170, 136, LIGHT_GRAY);
	}
	else if(t3speed == 3000000) {
		lcd_fillRect(160, 5, 170, 30, DARK_CYAN);
		lcd_fillRect(160, 35, 170, 65, DARK_CYAN);
		lcd_fillRect(160, 70, 170, 100, DARK_CYAN);
		lcd_fillRect(160, 101, 170, 136, LIGHT_GRAY);
	}
	else if(t3speed == 1500000) {
		lcd_fillRect(160, 5, 170, 30, DARK_CYAN);
		lcd_fillRect(160, 35, 170, 65, DARK_CYAN);
		lcd_fillRect(160, 70, 170, 100, DARK_CYAN);
		lcd_fillRect(160, 105, 170, 135, DARK_CYAN);
	}
}

void updateOctaveVisuals () {
	// Setup BPM visual display.
	if(octive == 1) {
		lcd_fillRect(140, 5, 150, 30, CYAN);
		lcd_fillRect(140, 35, 150, 65, DARK_GRAY);
	}
	else if(octive == 2) {
		lcd_fillRect(140, 5, 150, 30, DARK_GRAY);
		lcd_fillRect(140, 35, 150, 65, CYAN);
	}
}

static portTASK_FUNCTION( vLcdTask, pvParameters )
{
	unsigned int pressure;
	unsigned int xPos;
	unsigned int yPos;
	//portTickType xLastWakeTime;
	
	int step_width = 320/sequence_steps;
	int step_height = 240/numNotes;
	int left, bottom = 0;
	int right = step_width;
	int top = step_height;
	
	int i, n, x;
	int waiting = 0;
	
	int newlySelectedSequence = 0;
	
	int prevMode = 0;
	int prevSeq = 0;

	/* Just to stop compiler warnings. */
	( void ) pvParameters;

	/* Initialise LCD display */
	/* NOTE: We needed to delay calling lcd_init() until here because it uses
	 * xTaskDelay to implement a delay and, as a result, can only be called from
	 * a task */
	lcd_init();
	
	lcd_fillScreen(LIGHT_GRAY);
	
	/* Initial LCD display */
	setupScreen(step_width,left, bottom,right,top);

	/* Infinite loop blocks waiting for a touch screen interrupt event from
	 * the queue. */
	 
	while(1)
	{
		/* Clear TS interrupts (EINT3) */
		/* Reset and (re-)enable TS interrupts on EINT3 */
		EXTINT = 8;						/* Reset EINT3 */

		/* Enable TS interrupt vector (VIC) (vector 17) */
		VICIntEnable = 1 << 17;			/* Enable interrupts on vector 17 */

		/* Block on a queue waiting for an event from the TS interrupt handler */
		xSemaphoreTake(xLcdSemphr, portMAX_DELAY);
				
		/* Disable TS interrupt vector (VIC) (vector 17) */
		VICIntEnClr = 1 << 17;

		/* Measure next sleep interval from this point */
		//xLastWakeTime = xTaskGetTickCount();
		
		// When switching sequences in edit mode.
		if(mode == 2){
			setupScreen(step_width,0, 0,step_width,step_height);
			mode = 0;
		}
		// Switching from playback mode to edit mode, screen must be setup.
		if((prevMode == 1) && (mode == 0)){
			setupScreen(step_width,0, 0,step_width,step_height);
		}
		// Switching from edit mode to playback mode.
		else if((prevMode == 0) && (mode == 1)) {
			setupPlaybackMode();
		}
		
		prevMode = mode;
		
		/*** EDIT MODE ***/
		/*****************/
		if(mode != 1) {
			/* Start polling the touchscreen pressure and position ( getTouch(...) ) */
			/* Keep polling until pressure == 0 */
			getTouch(&xPos, &yPos, &pressure);
			while (pressure > 0)
			{
				// Button debounce on screen touch.
				waiting = 0;
				while(waiting<100000) {
					waiting++;
				}
				// Iterating through each row.
				for(i=0; i<numNotes; i++) {
					if((xPos < top) && (xPos > bottom)) {
						// If touch within bounds of row i, check for the corresponding column.
						for(n=0; n<sequence_steps; n++) {
							if((yPos < right) && (yPos > left)){
								// Render column cells to clear previously selected cell and set up newly selected or change previously selected note time.
								for(x=0; x<numNotes;x++){
									lcd_fillRect((x*step_height), (n*step_width), (x*step_height)+step_height, ((n*step_width)+step_width), LIGHT_GRAY);
									lcd_drawRect((x*step_height), (n*step_width), (x*step_height)+step_height, ((n*step_width)+step_width), DARK_GRAY);
									if(x != i){
										// Majority of cells will have no note.
										timing[x][n] = 0;
									}
								}
								if(timing[i][n]==0 || timing[i][n]==1){
									timing[i][n]++;
								}
								else {
									timing[i][n] = timing[i][n] + 2;
								}
								if (timing[i][n] > 4){
									timing[i][n] = 0;
								}
								final_timing[n] = timing[i][n];
							  
								if(timing[i][n] == 1){
									// Full note.
									lcd_fillRect(bottom + 4, left + 4, top - 4, right - 4, MAROON);
								}
								else if(timing[i][n] == 2){
									// Half note.
									lcd_fillRect(bottom + 4, left + 4, top - 4, right - 4, OLIVE);
								}
								else if(timing[i][n] == 4){
									// Quarter note.
									lcd_fillRect(bottom + 4, left + 4, top - 4, right - 4, PURPLE);
								}
								sequence[n] = i;
							}
							left = left + step_width;
							right = right + step_width;
						}	
					}
					left = 0;
					right = step_width;
					top = top + step_height;
					bottom = bottom + step_height;
				}
				step_width = 320/sequence_steps;
				bottom = 0;
				left = 0;
				right = step_width;
				top = step_height;
				
				getTouch(&xPos, &yPos, &pressure);
			}
		}
		/*** PLAYBACK MODE ***/
		/*********************/
		else {
			/* Start polling the touchscreen pressure and position ( getTouch(...) ) */
			/* Keep polling until pressure == 0 */
			getTouch(&xPos, &yPos, &pressure);
			while (pressure > 0)
			{
				// Bound touch area for more efficiency.
				if((xPos < 235) && (xPos > 185)){
					// Select current sequence.
					for(i=0; i<9; i++) {
						if(((xPos < 235) && (xPos > 185)) && ((yPos < (35*i) + 35) && (yPos > (35*i) + 5))){
							// If a new sequence selected, must save the one that's currently in use to the listOfSequences[][] and listOfTimingSequences[][]
							// and then reinitialise the current sequence[] to the relative one in listOfSequences[][] and listOfTimingSequences[][].
							newlySelectedSequence = i;
							if(newlySelectedSequence != currentlySelectedSequence) {
								lcd_fillRect(185, (35*currentlySelectedSequence) + 5, 235, (35*currentlySelectedSequence) + 35, LIGHT_GRAY);
								lcd_fillRect(185, (35*newlySelectedSequence) + 5, 235, (35*newlySelectedSequence) + 35, MAGENTA);
								for(n=0;n<sequence_steps;n++){
									listOfSequences[currentlySelectedSequence][n] = sequence[n];
									listOfTimingSequences[currentlySelectedSequence][n] = final_timing[n];
								}
								currentlySelectedSequence = newlySelectedSequence;
								for(n=0;n<sequence_steps;n++){
									sequence[n] = listOfSequences[currentlySelectedSequence][n];
									final_timing[n] = listOfTimingSequences[currentlySelectedSequence][n];
								}
							}
						}
					}
				}
				getTouch(&xPos, &yPos, &pressure);
			}
			// Prevent flicker.
			if(prevSeq != currentlySelectedSequence){
				// Only fill part of screen to reduce flicker.
				lcd_fillRect(0, 0, 175, 320, LIGHT_GRAY);
				// Setup note bars for current sequence[] and final_timing[].
				for(i=0; i<sequence_steps; i++) {
					if(sequence[i] != numNotes) {
						if(final_timing[i]==1){
							lcd_fillRect(0, (i*31)+5, ((sequence[i]+1)*11), (i*31)+31, MAROON);
						}
						else if(final_timing[i]==2){
							lcd_fillRect(0, (i*31)+5, ((sequence[i]+1)*11), (i*31)+31, OLIVE);
						}
						else if(final_timing[i]==4){
							lcd_fillRect(0, (i*31)+5, ((sequence[i]+1)*11), (i*31)+31, PURPLE);
						}
					}
				}
				// Setup BPM visual display.
				updateBPMVisuals();
				// Setup octave visual display.
				updateOctaveVisuals();
			}
			prevSeq = currentlySelectedSequence;
		}
	}
}


void vLCD_ISRHandler( void )
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	/* Process the touchscreen interrupt */
	/* We would want to indicate to the task above that an event has occurred */
	xSemaphoreGiveFromISR(xLcdSemphr, &xHigherPriorityTaskWoken);

	EXTINT = 8;					/* Reset EINT3 */
	VICVectAddr = 0;			/* Clear VIC interrupt */

	/* Exit the ISR.  If a task was woken by either a character being received
	or transmitted then a context switch will occur. */
	portEXIT_SWITCHING_ISR( xHigherPriorityTaskWoken );
}


