#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include "bcmGPIO.c"

#define TESTING_PC 0
#define DEBUG 0

typedef enum
{
	HDR,
	HIR,
	TDR,
	TIR,
	RUNTEST,
	SDR,
	SIR,
	STATE,
	ENDDR,
	ENDIR,
	FREQUENCY,
	NONE
} command;

typedef enum
{
    RESET,
    IDLE,
    DRPAUSE,
    IRPAUSE,
    SHIFTDR,
    SHIFTIR,
    SELECTDR,
    SELECTIR,
    CAPTUREDR,
    CAPTUREIR,
    EXIT1DR,
    EXIT1IR,
    EXIT2DR,
    EXIT2IR,
    UPDATEDR,
    UPDATEIR
} state;

state currentState = RESET;
state endStateDataRegister = DRPAUSE;
state endStateInstructionRegister = IRPAUSE;
state endStateRunTest = IDLE;
double sleepTime = 1000.0;
double sleepTimeHalf = 1000.0;
double sleepTimeQuarter = 1000.0;
unsigned *headerDataRegister;
unsigned *headerInstructionRegister;
unsigned *tailDataRegister;
unsigned *tailInstructionRegister;
double completesleeptime = 0.0;
unsigned looping = 0;

void changeState(unsigned tms)
{
	switch(currentState)
	{
		case RESET:
			switch(tms)
			{
				case 0:
					currentState = IDLE;
					return;
				case 1:
					currentState = RESET;
					return;
			}
		case IDLE:
			switch(tms)
			{
				case 0:
					currentState = IDLE;
					return;
				case 1:
					currentState = SELECTDR;
					return;
			}
		case SELECTDR:
			switch(tms)
			{
				case 0:
					currentState = CAPTUREDR;
					return;
				case 1:
					currentState = SELECTIR;
					return;
			}
		case SELECTIR:
			switch(tms)
			{
				case 0:
					currentState = CAPTUREIR;
					return;
				case 1:
					currentState = RESET;
					return;
			}
		case CAPTUREDR:
			switch(tms)
			{
				case 0:
					currentState = SHIFTDR;
					return;
				case 1:
					currentState = EXIT1DR;
					return;
			}
		case CAPTUREIR:
			switch(tms)
			{
				case 0:
					currentState = SHIFTIR;
					return;
				case 1:
					currentState = EXIT1IR;
					return;
			}
		case SHIFTDR:
			switch(tms)
			{
				case 0:
					currentState = SHIFTDR;
					return;
				case 1:
					currentState = EXIT1DR;
					return;
			}
		case SHIFTIR:
			switch(tms)
			{
				case 0:
					currentState = SHIFTIR;
					return;
				case 1:
					currentState = EXIT1IR;
					return;
			}
		case EXIT1DR:
			switch(tms)
			{
				case 0:
					currentState = DRPAUSE;
					return;
				case 1:
					currentState = UPDATEDR;
					return;
			}
		case EXIT1IR:
			switch(tms)
			{
				case 0:
					currentState = IRPAUSE;
					return;
				case 1:
					currentState = UPDATEIR;
					return;
			}
		case DRPAUSE:
			switch(tms)
			{
				case 0:
					currentState = DRPAUSE;
					return;
				case 1:
					currentState = EXIT2DR;
					return;
			}
		case IRPAUSE:
			switch(tms)
			{
				case 0:
					currentState = IRPAUSE;
					return;
				case 1:
					currentState = EXIT2IR;
					return;
			}
		case EXIT2DR:
			switch(tms)
			{
				case 0:
					currentState = SHIFTDR;
					return;
				case 1:
					currentState = UPDATEDR;
					return;
			}
		case EXIT2IR:
			switch(tms)
			{
				case 0:
					currentState = SHIFTIR;
					return;
				case 1:
					currentState = UPDATEIR;
					return;
			}
		case UPDATEDR:
			switch(tms)
			{
				case 0:
					currentState = IDLE;
					return;
				case 1:
					currentState = SELECTDR;
					return;
			}
		case UPDATEIR:
			switch(tms)
			{
				case 0:
					currentState = IDLE;
					return;
				case 1:
					currentState = SELECTDR;
					return;
			}
	}
}

char *stateToString(state currentState)
{
	switch(currentState)
	{
		case RESET:
			return "RESET";
			break;
		case IDLE:
			return "IDLE";
			break;
		case SELECTDR:
			return "SELECTDR";
			break;
		case SELECTIR:
			return "SELECTIR";
			break;
		case CAPTUREDR:
			return "CAPTUREDR";
			break;
		case CAPTUREIR:
			return "CAPTUREIR";
			break;
		case SHIFTDR:
			return "SHIFTDR";
			break;
		case SHIFTIR:
			return "SHIFTIR";
			break;
		case EXIT1DR:
			return "EXIT1DR";
			break;
		case EXIT1IR:
			return "EXIT1IR";
			break;
		case DRPAUSE:
			return "DRPAUSE";
			break;
		case IRPAUSE:
			return "IRPAUSE";
			break;
		case EXIT2DR:
			return "EXIT2DR";
			break;
		case EXIT2IR:
			return "EXIT2IR";
			break;
		case UPDATEDR:
			return "UPDATEDR";
			break;
		case UPDATEIR:
			return "UPDATEIR";
			break;
	}
}

unsigned clockJTAG(unsigned tms, unsigned tdi)
{
	writeGPIO(TMS,tms);
	writeGPIO(TDI,tdi);
	//1-0-1
	writeGPIO(TCK,0);
	delayMicroseconds(sleepTimeHalf);
	writeGPIO(TCK,1);
	delayMicroseconds(sleepTimeHalf);

	//unsigned tdo = readGPIO(TDO);

    changeState(tms);

	//printf("[TMS:%d, TDI:%d, TDO_LINE:%d]\n", tms, tdi, tdo);

	return readGPIO(TDO);
}

unsigned shiftJTAG(unsigned tdi)
{
	writeGPIO(TMS,0);
	writeGPIO(TDI,tdi);
	//1-0-1
	writeGPIO(TCK,0);
	delayMicroseconds(sleepTimeHalf);
	writeGPIO(TCK,1);
	delayMicroseconds(sleepTimeHalf);

	unsigned tdo = readGPIO(TDO);
	return tdo;
}

void resetJTAG()
{
	for(int i = 0; i < 6; i++)
	{
		clockJTAG(1,0);
	}
}

unsigned moveToState(state destination)
{
	switch(currentState)
	{
		case RESET:
			switch(destination)
			{
				case RESET:
					break;
				case IDLE:
					clockJTAG(0,0);
					break;
				case SHIFTDR:
					clockJTAG(0,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					clockJTAG(0,0);
					break;
				case SHIFTIR:
					clockJTAG(0,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					clockJTAG(0,0);
					break;
				case DRPAUSE:
					clockJTAG(0,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					break;
				case IRPAUSE:
					clockJTAG(0,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					break;
				default:
					return 0;
			}
			break;
		case IDLE:
			switch(destination)
			{
				case RESET:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					break;
				case IDLE:
					break;
				case SHIFTDR:
					clockJTAG(1,0);
					clockJTAG(0,0);
					clockJTAG(0,0);
					break;
				case SHIFTIR:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					clockJTAG(0,0);
					break;
				case DRPAUSE:
					clockJTAG(1,0);
					clockJTAG(0,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					break;
				case IRPAUSE:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					break;
				default:
					return 0;
			}
			break;
		case SHIFTDR:
			switch(destination)
			{
				case RESET:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					break;
				case IDLE:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					break;
				case SHIFTDR:
					break;
				case SHIFTIR:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					clockJTAG(0,0);
					break;
				case DRPAUSE:
					clockJTAG(1,0);
					clockJTAG(0,0);
					break;
				case IRPAUSE:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					break;
				default:
					return 0;
			}
			break;
		case SHIFTIR:
			switch(destination)
			{
				case RESET:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					break;
				case IDLE:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					break;
				case SHIFTDR:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					clockJTAG(0,0);
					break;
				case SHIFTIR:
					break;
				case DRPAUSE:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					break;
				case IRPAUSE:
					clockJTAG(1,0);
					clockJTAG(0,0);
					break;
				default:
					return 0;
			}
			break;
		case DRPAUSE:
			switch(destination)
			{
				case RESET:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					break;
				case IDLE:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					break;
				case SHIFTDR:
					clockJTAG(1,0);
					clockJTAG(0,0);
					break;
				case SHIFTIR:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					clockJTAG(0,0);
					break;
				case DRPAUSE:
					break;
				case IRPAUSE:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					break;
				default:
					return 0;
			}
			break;
		case IRPAUSE:
			switch(destination)
			{
				case RESET:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					break;
				case IDLE:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					break;
				case SHIFTDR:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					clockJTAG(0,0);
					break;
				case SHIFTIR:
					clockJTAG(1,0);
					clockJTAG(0,0);
					break;
				case DRPAUSE:
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					clockJTAG(1,0);
					clockJTAG(0,0);
					break;
				case IRPAUSE:
					break;
				default:
					return 0;
			}
			break;
		default:
			return 0;
	}
	return 1;
}

unsigned shiftDataRegister(int bits, unsigned *tdi, unsigned *tdo, unsigned *mask)
{
	moveToState(SHIFTDR);
    unsigned data[bits];
	//shift tdi in reading data
	for (int i = 0; i < bits-1; i++)
	{
		data[bits-i-1] = clockJTAG(0,tdi[bits-1-i]);
	}
	data[0] = clockJTAG(1,tdi[0]);
	clockJTAG(0,0);
	moveToState(ENDDR);
	//compare data and tdo (if mask given with mask)
	if (tdo != NULL)
	{
		if (mask != NULL)
		{
			for (int i = 0; i < bits; i++)
			{
				if (mask[i] != 0)
				{
					if (data[i] != tdo[i])
					{
						free(tdi);
						free(tdo);
						free(mask);
						return 0;
					}
				}
			}
		}
		else
		{
			for (int i = 0; i < bits; i++)
			{
				if (data[i] != tdo[i])
				{
					if (!looping)
					{
	                    free(tdi);
						free(tdo);
						return 0;
					}
				}
				else if (looping && data[0] == tdo[0])
				{
					looping = 0;
					free(tdo);
				}
			}
		}
	}
	free(tdi);
	return 1;
}

unsigned shiftInstructionRegister(int bits, unsigned *tdi, unsigned *tdo, unsigned *mask)
{
	moveToState(SHIFTIR);
	unsigned data[bits];
	//shift tdi in reading data
	for (int i = 0; i < bits-1; i++)
	{
		data[bits-i-1] = clockJTAG(0,tdi[bits-1-i]);
	}
	data[0] = clockJTAG(1,tdi[0]);
	clockJTAG(0,0);
	moveToState(ENDIR);
	//compare data and tdo (if mask given with mask)
	if (tdo != NULL)
	{
		if (mask != NULL)
		{
			for (int i = 0; i < bits; i++)
			{
				if (mask[i] != 0)
				{
					if (data[i] != tdo[i])
					{
					    free(tdi);
						free(tdo);
						free(mask);
						return 0;
					}
				}
			}
		}
		else
		{
			for (int i = 0; i < bits; i++)
			{
				if (data[i] != tdo[i])
				{
					free(tdi);
					free(tdo);
					return 0;
				}
			}
		}
	}
    free(tdi);
	return 1;
}

unsigned setHeaderDataRegister(int bits, unsigned *pattern)
{
	headerDataRegister = realloc(headerDataRegister, sizeof *headerDataRegister * bits);
	if (!headerDataRegister)
		return 0;
	for (int i = 0; i < bits; i++)
	{
		headerDataRegister[i] = pattern[i];
	}
	return 1;
}

unsigned setHeaderInstructionRegister(int bits, unsigned *pattern)
{
	headerInstructionRegister = realloc(headerInstructionRegister, sizeof *headerInstructionRegister * bits);
	if (!headerInstructionRegister)
		return 0;
	for (int i = 0; i < bits; i++)
	{
		headerInstructionRegister[i] = pattern[i];
	}
	return 1;
}

unsigned setTailDataRegister(int bits, unsigned *pattern)
{
	tailDataRegister = realloc(tailDataRegister, sizeof *tailDataRegister * bits);
	if (!tailDataRegister)
		return 0;
	for (int i = 0; i < bits; i++)
	{
		tailDataRegister[i] = pattern[i];
	}
	return 1;
}

unsigned setTailInstructionRegister(int bits, unsigned *pattern)
{
	tailInstructionRegister = realloc(tailInstructionRegister, sizeof *tailInstructionRegister * bits);
	if (!tailInstructionRegister)
		return 0;
	for (int i = 0; i < bits; i++)
	{
		tailInstructionRegister[i] = pattern[i];
	}
	return 1;
}

unsigned runTest(state waitState, int tck, uint64_t time)
{
	if (!moveToState(waitState))
		return 0;
	for (int i = 0; i < tck; i++) {
		clockJTAG(0,0);
	}
	delayMicroseconds(time);
	return 1;
}

unsigned setEndStateDataRegister(state endState)
{
	endStateDataRegister = endState;
	return 1;
}

unsigned setEndStateInstructionRegister(state endState)
{
	endStateInstructionRegister = endState;
	return 1;
}

unsigned setFrequency(uint64_t frequency)
{
    uint64_t micros = 1000000/frequency;
    if (micros > 1)
    {
        sleepTime = micros;
        sleepTimeHalf = sleepTime/2;
    }
    else
    {
        sleepTime = 1;
        sleepTimeHalf = 1;
    }
	return 1;
}

unsigned setEndStateRunTest(state endState)
{
	endStateRunTest = endState;
	return 1;
}

// Helper-Functions for parsing the data from the SVF-file
state stringToState(char *string)
{
	if (!strncmp(string, "RESET", 5))
	{
		return RESET;
	}
	else if (!strncmp(string, "IDLE", 4))
	{
		return IDLE;
	}
	else if (!strncmp(string, "SELECTDR", 8))
	{
		return SELECTDR;
	}
	else if (!strncmp(string, "SELECTIR", 8))
	{
		return SELECTIR;
	}
	else if (!strncmp(string, "CAPTUREDR", 9))
	{
		return CAPTUREDR;
	}
	else if (!strncmp(string, "CAPTUREIR", 9))
	{
		return CAPTUREIR;
	}
	else if (!strncmp(string, "SHIFTDR", 7))
	{
		return SHIFTDR;
	}
	else if (!strncmp(string, "SHIFTIR", 7))
	{
		return SHIFTIR;
	}
	else if (!strncmp(string, "DRPAUSE", 7))
	{
		return IRPAUSE;
	}
	else if (!strncmp(string, "IRPAUSE", 7))
	{
		return IRPAUSE;
	}
	else if (!strncmp(string, "EXIT1DR", 7))
	{
		return EXIT1DR;
	}
	else if (!strncmp(string, "EXIT1IR", 7))
	{
		return EXIT1IR;
	}
	else if (!strncmp(string, "EXIT2DR", 7))
	{
		return EXIT2DR;
	}
	else if (!strncmp(string, "EXIT2IR", 7))
	{
		return EXIT2IR;
	}
	else if (!strncmp(string, "UPDATEDR", 8))
	{
		return UPDATEDR;
	}
	else if (!strncmp(string, "UPDATEIR", 8))
	{
		return UPDATEIR;
	}
}

unsigned isInt(char character)
{
	return (character > 47 && character < 58);
}

char *getSubstring(char *string, int start, int end)
{
	char *substring;
	int sizeSubstring = end - start + 1;
	if (sizeSubstring > 0)
		substring = malloc(sizeof *substring * sizeSubstring);
	for (int i = 0; i < sizeSubstring; i++)
	{
		substring[i] = string[start+i];
	}
	substring[sizeSubstring-1] = '\0';
	return substring;
}

int findFirstOccurenceOfChar(char *string, char character)
{
	for (int i = 0; i < strlen(string)+1; i++)
	{
		if (string[i] == character)
		{
			return i;
		}
	}
	return -1;
}

int powerOfTen(int e)
{
	int result = 1;
	for (int i = 0; i < e; i++)
	{
		result *= 10;
	}
	return result;
}

int findFirstOccurenceOfString(char *string, char *searchstring)
{
	int length = strlen(string) + 1;
	for (int i = 0; i < length-strlen(searchstring); i++)
	{
		if (string[i] == searchstring[0])
		{
			if (strlen(searchstring) == 1)
			{
				return i;
			}
			for (int j = 1; j < strlen(searchstring); j++)
			{
				if (string[i+j] != searchstring[j])
				{
					break;
				}
				if (j == strlen(searchstring)-1)
				{
					return i;
				}
			}
		}
	}
	return -1;
}

unsigned stringContains(char *string, char *searchstring)
{
	int length = strlen(string) + 1;
	for (int i = 0; i < length-strlen(searchstring); i++)
	{
		if (string[i] == searchstring[0])
		{
			if (strlen(searchstring) == 1)
			{
				return 1;
			}
			for (int j = 1; j < strlen(searchstring); j++)
			{
				if (string[i+j] != searchstring[j])
				{
					break;
				}
				if (j == strlen(searchstring)-1)
				{
					return 1;
				}
			}
		}
	}
	return 0;
}

unsigned *parseHexString(char *string)
{
	if (string == NULL || string == "")
	{
		return NULL;
	}
	int length = strlen(string) * 4;
    unsigned *bitString = malloc(sizeof *bitString * length);
	for (int i = 0, j = 0; i < length; i = i+4, j++)
	{
		switch(string[j])
		{
			case '0':
				bitString[i] 	= 0;
				bitString[i+1] 	= 0;
				bitString[i+2] 	= 0;
				bitString[i+3] 	= 0;
				break;
			case '1':
				bitString[i] 	= 0;
				bitString[i+1] 	= 0;
				bitString[i+2] 	= 0;
				bitString[i+3] 	= 1;
				break;
			case '2':
				bitString[i] 	= 0;
				bitString[i+1] 	= 0;
				bitString[i+2] 	= 1;
				bitString[i+3] 	= 0;
				break;
			case '3':
				bitString[i] 	= 0;
				bitString[i+1] 	= 0;
				bitString[i+2] 	= 1;
				bitString[i+3] 	= 1;
				break;
			case '4':
				bitString[i] 	= 0;
				bitString[i+1] 	= 1;
				bitString[i+2] 	= 0;
				bitString[i+3] 	= 0;
				break;
			case '5':
				bitString[i] 	= 0;
				bitString[i+1] 	= 1;
				bitString[i+2] 	= 0;
				bitString[i+3] 	= 1;
				break;
			case '6':
				bitString[i] 	= 0;
				bitString[i+1] 	= 1;
				bitString[i+2] 	= 1;
				bitString[i+3] 	= 0;
				break;
			case '7':
				bitString[i] 	= 0;
				bitString[i+1] 	= 1;
				bitString[i+2] 	= 1;
				bitString[i+3] 	= 1;
				break;
			case '8':
				bitString[i] 	= 1;
				bitString[i+1] 	= 0;
				bitString[i+2] 	= 0;
				bitString[i+3] 	= 0;
				break;
			case '9':
				bitString[i] 	= 1;
				bitString[i+1] 	= 0;
				bitString[i+2] 	= 0;
				bitString[i+3] 	= 1;
				break;
			case 'A':
				bitString[i] 	= 1;
				bitString[i+1] 	= 0;
				bitString[i+2] 	= 1;
				bitString[i+3] 	= 0;
				break;
			case 'B':
				bitString[i] 	= 1;
				bitString[i+1] 	= 0;
				bitString[i+2] 	= 1;
				bitString[i+3] 	= 1;
				break;
			case 'C':
				bitString[i] 	= 1;
				bitString[i+1] 	= 1;
				bitString[i+2] 	= 0;
				bitString[i+3] 	= 0;
				break;
			case 'D':
				bitString[i] 	= 1;
				bitString[i+1] 	= 1;
				bitString[i+2] 	= 0;
				bitString[i+3] 	= 1;
				break;
			case 'E':
				bitString[i] 	= 1;
				bitString[i+1] 	= 1;
				bitString[i+2] 	= 1;
				bitString[i+3] 	= 0;
				break;
			case 'F':
				bitString[i] 	= 1;
				bitString[i+1] 	= 1;
				bitString[i+2] 	= 1;
				bitString[i+3] 	= 1;
		}
	}
    free(string);
	return bitString;
}

char *concatStrings(char *startstring, char *endstring)
{
	int sizeStartstring = strlen(startstring);
	int sizeEndstring = strlen(endstring);
	int finalsize = sizeStartstring + sizeEndstring + 1;
	char *finalstring;
	finalstring = (char *) malloc(finalsize * sizeof(char));
	strcpy(finalstring, startstring);
	strcat(finalstring, endstring);
	finalstring[finalsize-1] = '\0';
    free(startstring);
	//printf("FINALSTRING:%s\n", finalstring);
	return finalstring;
}


char *removeTabsAndNewline(char *input)
{
	int i,j;
    char *output = malloc(sizeof *output * strlen(input));
    for (i = 0, j = 0; i<strlen(input); i++,j++)
    {
        if (input[i] != ' ' && input[i] != '\t' && input[i] != '\n' && input[i] != 13)
            output[j]=input[i];
        else
            j--;
    }
    output[j]='\0';
    return output;
}

unsigned executeTask(char *commandString)
{
	int bits = 0;
	state commandState = RESET;
	unsigned *tdi;
	unsigned *tdo;
	unsigned *mask;
	uint64_t frequency = 0.0;
	uint64_t sleep = 0.0;
	int clockcycles = 0;
    char *stateString;
	int commandResult = 0;
	if (TESTING_PC)
		commandResult = 1;
	if (strncmp(commandString, "HDR", 3) == 0)
	{
		if (commandString[3] != '0')
		{
			//TODO
			commandResult = 1;
		}
		else
		{
			if (DEBUG)
				printf("executing: HDR(%d)\n", bits);
			if (!TESTING_PC)
				commandResult = setHeaderDataRegister(bits, NULL);
		}
		if (!commandResult)
		{
			printf("ERROR OCCURED DURING HDR!\n");
			return 0;
		}
		return 1;
	}
	else if (strncmp(commandString, "HIR", 3) == 0)
	{
		if (commandString[3] != '0')
		{
			//TODO
			commandResult = 1;
		}
		else
		{
			if (DEBUG)
				printf("executing: HIR(%d)\n", bits);
			if (!TESTING_PC)
				commandResult = setHeaderInstructionRegister(bits, NULL);
		}
		if (!commandResult)
		{
			printf("ERROR OCCURED DURING HIR!\n");
			return 0;
		}
		return 1;
	}
	else if (strncmp(commandString, "TDR", 3) == 0)
	{
		if (commandString[3] != '0')
		{
			//TODO
			commandResult = 1;
		}
		else
		{
			if (DEBUG)
				printf("executing: TDR(%d)\n", bits);
			if (!TESTING_PC)
				commandResult = setTailDataRegister(bits, NULL);
		}
		if (!commandResult)
		{
			printf("ERROR OCCURED DURING TDR!\n");
			return 0;
		}
		return 1;
	}
	else if (strncmp(commandString, "TIR", 3) == 0)
	{
		if (commandString[3] != '0')
		{
			//TODO
			commandResult = 1;
		}
		else
		{
			if (DEBUG)
				printf("executing: TIR(%d)\n", bits);
			if (!TESTING_PC)
				commandResult = setTailInstructionRegister(bits, NULL);
		}
		if (!commandResult)
		{
			printf("ERROR OCCURED DURING TIR!\n");
			return 0;
		}
		return 1;
	}
	else if (strncmp(commandString, "RUNTEST", 7) == 0)
	{
		int start = 7;
		int end = strlen(commandString) + 1;
		for (int i = 7; i < end; i++)
		{
			if (isInt(commandString[i]))
			{
				end = i;
			}
		}
        stateString = getSubstring(commandString, start, end);
		commandState = stringToState(stateString);
        free(stateString);
		if (stringContains(commandString, "TCK"))
		{
			int location = 0;
			for (int i = findFirstOccurenceOfString(commandString, "TCK")-1; i > 0; i--)
			{
				if (isInt(commandString[i]))
				{
					clockcycles += (commandString[i]-48)*powerOfTen(location++);
				}
				else
				{
					break;
				}
			}
		}
		if (stringContains(commandString, "SEC"))
		{
			char *secstring = "";
			int secpos = findFirstOccurenceOfString(commandString, "SEC");
			int testpos = findFirstOccurenceOfString(commandString, "TEST");
			int tckpos = findFirstOccurenceOfString(commandString, "TCK");
			if (testpos > tckpos)
			{
				while (!isInt(commandString[start]))
                {
                    start++;
                }
			}
			else
			{
				start = tckpos+3;
			}
            secstring = getSubstring(commandString, start, findFirstOccurenceOfString(commandString, "SEC"));
			char *tmpptr;
			sleep = strtod(secstring, &tmpptr)*1000000;
            free(secstring);
            completesleeptime += sleep;
		}
		if (DEBUG)
			printf("executing: RUNTEST(%s, %d, %lld)\n", stateToString(commandState), clockcycles, sleep);
		if (!TESTING_PC)
			commandResult = runTest(commandState, clockcycles, sleep);
		if (!commandResult)
		{
			printf("ERROR OCCURED DURING RUNTEST!\n");
			return 0;
		}
		return 1;
	}
	else if (strncmp(commandString, "SDR", 3) == 0)
	{
		int start = 3;
		int end = strlen(commandString)+1;
		for (int i = start; i < end; i++)
		{
			if (isInt(commandString[i]))
			{
				bits = bits*10 + (commandString[i]-48);
			}
			else
			{
				break;
			}
		}
		int bytes = bits/4 + bits%4;
		start = findFirstOccurenceOfString(commandString,"TDI")+4;
        char *tdistring = getSubstring(commandString, start, start+bytes);
        char *tdostring = "";
        char *maskstring = "";
		if (findFirstOccurenceOfString(commandString,"TDO") != -1)
		{
			start = findFirstOccurenceOfString(commandString,"TDO")+4;
            tdostring = getSubstring(commandString, start, start+bytes);
		}
		if (findFirstOccurenceOfString(commandString,"MASK") != -1)
		{
			start = findFirstOccurenceOfString(commandString,"MASK")+5;
			maskstring = getSubstring(commandString, start, start+bytes);
		}
		if (DEBUG)
			printf("executing: SDR(%d, %s, %s, %s)\n", bits, tdistring, tdostring, maskstring);
		if (!TESTING_PC)
			commandResult = shiftDataRegister(bits, parseHexString(tdistring), parseHexString(tdostring), parseHexString(maskstring));
		if (!commandResult)
		{
			printf("ERROR OCCURED DURING SDR!\n");
			return 0;
		}
		return 1;
	}
	else if (strncmp(commandString, "SIR", 3) == 0)
	{
		int start = 3;
		int end = strlen(commandString)+1;
		for (int i = start; i < end; i++)
		{
			if (isInt(commandString[i]))
			{
				bits = bits*10 + (commandString[i]-48);
			}
			else
			{
				break;
			}
		}
        int bytes = bits/4 + bits%4;
		start = findFirstOccurenceOfString(commandString,"TDI")+4;
        char *tdistring = getSubstring(commandString, start, start+bytes);
        char *tdostring = "";
        char *maskstring = "";
		if (findFirstOccurenceOfString(commandString,"TDO") != -1)
		{
			start = findFirstOccurenceOfString(commandString,"TDO")+4;
            tdostring = getSubstring(commandString, start, start+bytes);
		}
		if (findFirstOccurenceOfString(commandString,"MASK") != -1)
		{
			start = findFirstOccurenceOfString(commandString,"MASK")+5;
			maskstring = getSubstring(commandString, start, start+bytes);
		}
		if (DEBUG)
			printf("executing: SIR(%d, %s, %s, %s)\n", bits, tdistring, tdostring, maskstring);
		if (!TESTING_PC)
			commandResult = shiftInstructionRegister(bits, parseHexString(tdistring), parseHexString(tdostring), parseHexString(maskstring));
		if (!commandResult)
		{
			printf("ERROR OCCURED DURING SIR!\n");
			return 0;
		}
		return 1;
	}
	else if (strncmp(commandString, "STATE", 5) == 0)
	{
		int start = 5;
		int end = findFirstOccurenceOfChar(commandString, ';');
        stateString = getSubstring(commandString, start, end);
		commandState = stringToState(stateString);
        free(stateString);
		if (DEBUG)
			printf("executing: STATE(%s)\n", stateToString(commandState));
		if (!TESTING_PC)
			commandResult = moveToState(commandState);
		if (!commandResult)
		{
			printf("ERROR OCCURED DURING STATE!\n");
			return 0;
		}
		return 1;
	}
	else if (strncmp(commandString, "ENDDR", 5) == 0)
	{
		int start = 5;
		int end = findFirstOccurenceOfChar(commandString, ';');
        stateString = getSubstring(commandString, start, end);
		commandState = stringToState(stateString);
        free(stateString);
		if (DEBUG)
			printf("executing: ENDDR(%s)\n", getSubstring(commandString, start, end));
		if (!TESTING_PC)
			commandResult = setEndStateDataRegister(commandState);
		if (!commandResult)
		{
			printf("ERROR OCCURED DURING ENDDR!\n");
			return 0;
		}
		return 1;
	}
	else if (strncmp(commandString, "ENDIR", 5) == 0)
	{
		int start = 5;
		int end = findFirstOccurenceOfChar(commandString, ';');
        stateString = getSubstring(commandString, start, end);
		commandState = stringToState(stateString);
        free(stateString);
		if (DEBUG)
			printf("executing: ENDIR(%s)\n", getSubstring(commandString, start, end));
		if (!TESTING_PC)
			commandResult = setEndStateInstructionRegister(commandState);
		if (!commandResult)
		{
			printf("ERROR OCCURED DURING ENDIR!\n");
			return 0;
		}
		return 1;
	}
	else if (strncmp(commandString, "ENDSTATE", 8) == 0)
	{
		int start = 8;
		int end = findFirstOccurenceOfChar(commandString, ';');
        stateString = getSubstring(commandString, start, end);
		commandState = stringToState(stateString);
        free(stateString);
		if (DEBUG)
			printf("executing: ENDSTATE(%s)\n", getSubstring(commandString, start, end));
		if (!TESTING_PC)
			commandResult = setEndStateRunTest(commandState);
		if (!commandResult)
		{
			printf("ERROR OCCURED DURING ENDSTATE!\n");
			return 0;
		}
		return 1;
	}
	else if (strncmp(commandString, "FREQUENCY", 9) == 0)
	{
		char *hzstring = getSubstring(commandString, findFirstOccurenceOfString(commandString, "Y")+1, findFirstOccurenceOfString(commandString, "HZ"));
		char *tmpptr;
		frequency = strtod(hzstring, &tmpptr);
        free(hzstring);
		if (DEBUG)
			printf("executing: FREQUENCY(%lld), microDelay: %lld\n", frequency, (1000000/frequency));
		if (!TESTING_PC)
			commandResult = setFrequency(frequency);
		if (!commandResult)
		{
			printf("ERROR OCCURED DURING FREQUENCY!\n");
			return 0;
		}
		return 1;
	}
	printf("ERROR: NOT A VALID COMMAND!");
	return 0;
    free(commandString);
}

#define MAXCHAR 256
int main(int argc, char **argv) {
    struct timeval start, end;

	gettimeofday(&start, NULL);

    if (!TESTING_PC)
	{
		initGPIO();
		resetJTAG();
	}

    FILE *fp;
    char *str = malloc(sizeof *str * MAXCHAR);
    char *filename = argv[1];
	char *currentCommandString = malloc(sizeof *currentCommandString * MAXCHAR);
    fp = fopen(filename, "r");
    if (fp == NULL){
        printf("Could not open file %s\n",filename);
        return 0;
    }
	char *tmpstr;
	printf("Programming started\n");
    while (fgets(str, MAXCHAR, fp) != NULL)
    {
		tmpstr = removeTabsAndNewline(str);
		if (tmpstr[0] != '!' && tmpstr[0] != '/' && tmpstr[1] != '/' && strlen(tmpstr) > 1)
		{
			currentCommandString = concatStrings(currentCommandString, tmpstr);
			if (strchr(tmpstr, ';') != NULL)
			{
                if (findFirstOccurenceOfString(currentCommandString, "LOOP") != -1)
                {
					looping = 1;
                    int loopcount = 0;
                    int location = 0;
                    int loopcmdcount = 0;
                    for (int i = findFirstOccurenceOfChar(currentCommandString, ';')-1; i > 0; i--)
        			{
        				if (isInt(currentCommandString[i]))
        				{
        					loopcount += (currentCommandString[i]-48)*powerOfTen(location++);
        				}
        				else
        				{
        					break;
        				}
        			}
                    currentCommandString = malloc(sizeof *currentCommandString * MAXCHAR);
                    while (fgets(str, MAXCHAR, fp) != NULL)
                    {
                        tmpstr = removeTabsAndNewline(str);
                        if (findFirstOccurenceOfString(tmpstr, "ENDLOOP") != -1)
                        {
                            int loopcmdpos[loopcmdcount+1];
                            loopcmdpos[0] = 0;
                            int c = 1;
                            for (int i = 0; i < strlen(currentCommandString); i++) {
                                if (currentCommandString[i] == ';')
                                {
                                    loopcmdpos[c++] = i+1;
                                }
                            }
                            for (int i = 0; i < loopcount; i++)
                            {
                                for (int j = 0; j < loopcmdcount; j++)
                                {
                                    char *currentLoopTask = getSubstring(currentCommandString, loopcmdpos[j], loopcmdpos[j+1]);
                                    executeTask(currentLoopTask);
									if (!looping)
										break;
                                }
								if(!looping)
									break;
                            }
                            break;
                        }
                		else if (tmpstr[0] != '!' && tmpstr[0] != '/' && tmpstr[1] != '/' && strlen(tmpstr) > 1)
                		{
                			currentCommandString = concatStrings(currentCommandString, tmpstr);
                            if (findFirstOccurenceOfChar(tmpstr, ';') != -1)
                                loopcmdcount++;
                        }
                        free(tmpstr);
                    }
                    free(currentCommandString);
                    currentCommandString = malloc(sizeof *currentCommandString * MAXCHAR);
                }
                else
                {
    				//printf("%s\n", currentCommandString);
    				if (!executeTask(currentCommandString))
					{
						printf("STOPPING EXECUTION!\n");
						resetJTAG();
						return 0;
					}
    				currentCommandString = malloc(sizeof *currentCommandString * MAXCHAR);
                }
			}
		}
        free(tmpstr);
    }
    if (DEBUG)
        printf("COMPLETESLEEPTIME: %lf\n", completesleeptime/1000000);
	printf("Programming finished\n");
	resetJTAG();
	if (!TESTING_PC)
		stopGPIO();

    gettimeofday(&end, NULL);

	long seconds = (end.tv_sec - start.tv_sec);
	long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
    printf("Time elpased is %ld seconds and %ld micros\n", seconds, micros);
    return 1;
}

