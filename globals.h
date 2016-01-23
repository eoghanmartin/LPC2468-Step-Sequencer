
extern int const sequence_steps; // The number of notes per sequence.
extern int sequence[]; // The live sequence used to play notes.

extern int listOfSequences[9][10]; // An array of 9 sequence arrays. These will originally be initialise to play no sound.
extern int listOfTimingSequences[9][10]; 	// An array of timing sequences. In these sequences, a value for the note size is assigned per note.
																					// 0 = no note. 1 = full note. 2 = half note. 4 = quarter notes.
extern int currentlySelectedSequence;	// The sequence within the listOfSequences[][] and listOfTimingSequences[][] arrays which is currently live.

extern int timing[12][10];	// Array corresponding to the sequencer grid view to set up live note size changes for notes.
extern int final_timing[];	//	The array used for the current sequence[] note sizes.
extern int noteCount;	// Counter used to iterate through the live sequence array when playing.
extern int t3speed;	// TIMER3 speed - varies depending on BPM.
extern  int mode;		// mode = 0: Edit mode.
										// mode = 1: Playback mode.
										// mode = 2: Used for switching sequences in edit mode.
extern int play;
extern int octive;
extern float notes[12];	// Array of 12 note frquencies, 1 full octave.
extern int const numNotes;	// The number of notes in this sequencer - 12.