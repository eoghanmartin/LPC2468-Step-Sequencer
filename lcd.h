#ifndef LCD_H
#define LCD_H

void vStartLcd( unsigned portBASE_TYPE uxPriority );

void setupScreen(int step_width, int left, int bottom, int right, int top);
void setupPlaybackMode ();
void updateBPMVisuals();
void updateOctaveVisuals();
void displayNoteBars();

#endif
