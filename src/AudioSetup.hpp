/*
These settings are usually defined in the board definitions file,
but if you're compiling for a board that doesn't have them defined, you can do so here.
*/

#ifndef BUFFSIZE
#define BUFFSIZE 1024 //number of samples in the buffer
#endif

#ifndef SAMPLERATE
#define SAMPLERATE 16000
#endif

#ifndef BYTESPERSAMPLE
#define BYTESPERSAMPLE 2
#endif

#ifndef NUMCHANNELS
#define NUMCHANNELS 2
#endif
