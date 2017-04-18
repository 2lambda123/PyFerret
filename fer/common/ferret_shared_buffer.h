/*
 * ferret_shared_buffer.h
 *
 * John Osborne
 * Jonathan Callahan (after Oct. 1995)
 *
 * This header file is included by files who's routines wish to have access to 
 * the memory buffer which is shared between Ferret and the GUI.  The only access
 * control is through the following test:
 *
 * 	  if ( sBuffer->flags[FRTN_CONTROL] == FCTRL_IN_FERRET ) 
 *
 * V702 *sh* 2/2017 - eliminated global "memory" - using individual mallocs now
 */

#ifndef _FERRET_SHARED_BUFFER_H 
#define _FERRET_SHARED_BUFFER_H


#define NUMFLAGS 10
#define TEXTLENGTH 500
#define NUMDOUBLES 2048

typedef struct sharedStruct {
	int flags[NUMFLAGS];
	char text[TEXTLENGTH];
	char *textP;
	int numStrings, numNumbers;
 	double nums[NUMDOUBLES];
} sharedMem;

typedef sharedMem *smPtr;

extern smPtr sBuffer;

extern float *ppl_memory;


#endif /* _FERRET_SHARED_BUFFER_H */
