# LPC2468-Step-Sequencer
Experimentation with the LPC2468 development board. This project implements a step sequencer on an LCD screen and used hardware interrupts via the FreeRTOS operating system to add user functionality.

## Requirements
* Keil uVision 5 with LPC2468 drivers.
* NXP LPC2468 Development Board with ARM7 Microcintroller.

## Design
The basic design for this project involved building a touchscreen application on the LPC2468 development board. The application should have two user functionality modes and the I2C push buttons on the development board should be used to add extra functionality for the user.
The application should work as a step sequencer, allowing the user to create sound sequences, play them back, save them and make some other custom changes to the playback of the sequences.
The modes used in the application are the edit and playback mode.
The sequencer was built to these specifications. The user interface was kept as minimal as possible so as to give the user a fluid experience of use. Colours used are soft and the functionality is intuitive.
The functionalities implemented are:
* play/pause sequences
* create up to nine sequences
* change sequences
* build a custom sequence using the touchscreen with up to 10 notes
* add half, quarter and full notes
* change the beats per minute to 4 different speeds
* view sequences graphically and see beats by the flashing of the GPIO LED
* follow the playback of sequences graphically
* switch between playback and edit modes
* switch between two octave ranges.

### Edit Mode
This mode allows the user to change the sounds to be played within a sequence. In doing this, the user must be able to play half notes as well as quarter notes and full notes. Added functionality here allows the user to switch between different sequences from within this mode using button two.
A single touch on a cell will denote a full note, two touches a half note and three touches will assign a quarter note to this step in the sequence. If a cell is pressed a fourth time, or not pressed at all, the step will be denoted no note and will be a silent step in the sequence.
![alt tag](https://raw.githubusercontent.com/eoghanmartin/LPC2468-Step-Sequencer/master/images/EditMode.png)

### Playback Mode
This mode allows the user to switch between different sequences and displays some visuals to allow the user to see how their sequences are playing.
When the sequence is played in this mode by pressing button one, the bars representing the notes visually on the screen will turn blue when their corresponding note is playing.
The blue dashes under the sequence selection pane represent the BPM of the playback. The corresponding values are displayed in the representation below.
Under the BPM display is the octave pitch display. Using button 2, the pitch of the sounds played by the sequencer can be changed in playback mode.
The top part of this screen is the sequence selection pane. Clicking on the light gray rectangles here will change the current sequence. There are a possible 9 sequences that can be saved, edited and played back here.
![alt tag](https://raw.githubusercontent.com/eoghanmartin/LPC2468-Step-Sequencer/master/images/PlaybackMode.png)

### Button Functionalities
In both modes, it is possible to change the speed at which the sequences play. It is also possible to play and pause the sequence. The current sequence can be switched using button 2, only while in edit mode. When in playback mode, this functionality is provided by clicking on the relative sequence that the user wants using the sequences bar at the top of the screen.
![alt tag](https://raw.githubusercontent.com/eoghanmartin/LPC2468-Step-Sequencer/master/images/Buttons.png)

## Implementation
Upon startup, the first job executed is the initialisation of all arrays and global variables contained in globals.h.
Next, the two tasks, “sensors” and “lcd” are implemented.
Finally, the Playback() and DACHardwareSetup() functions are run to set up the timer interrupts to control the sequence sounds before the FreeRTOS scheduler is kicked off.
![alt tag](https://raw.githubusercontent.com/eoghanmartin/LPC2468-Step-Sequencer/master/images/StepSequencer_Arch.png)
