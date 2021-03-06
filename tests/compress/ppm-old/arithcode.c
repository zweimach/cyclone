/*
	Arithmetic encoding. Taken largely from Witten, Neal, Cleary,
		CACM, 1987, p520.
	Includes bitfiddling routines.
	Alistair Moffat, alistair@cs.mu.oz.au, January 1987.

	@(#)arithcode.c	1.1 5/3/91
*/

#include <stdio.h>
#include "arithcode.h"

/* #define BECAREFUL */

codevalue	S_low=0, S_high=0, S_value=0;
long		S_bitstofollow=0;
int		S_buffer=0, S_bitstogo=0;

long		cmpbytes=0, rawbytes=0;

/*==================================*/

#define BITPLUSFOLLOW(b)		\
{	OUTPUTBIT((b));			\
	while (bitstofollow)		\
	{	OUTPUTBIT(!(b));	\
		bitstofollow -= 1;	\
	}				\
}

#define OUTPUTBIT(b)			\
{	buffer >>= 1;			\
	if (b) buffer |= 0x80;	 	\
	bitstogo -= 1;			\
	if (bitstogo==0)		\
	{	putc(buffer, stdout);	\
		bitstogo = 8;		\
		cmpbytes += 1;		\
	}				\
}

#define ADDNEXTINPUTBIT(v)		\
{	if (bitstogo==0)		\
	{	buffer = getc(stdin);	\
		bitstogo = 8;		\
	}				\
	v = (v<<1)+(buffer&1);		\
	buffer >>= 1;			\
	bitstogo -=1;			\
} 

/*==================================*/

void
arithmetic_encode(lbnd, hbnd, totl)
int lbnd, hbnd, totl;
{	
	register codevalue low, high;

#ifdef BECAREFUL
        if (!(0<=lbnd && lbnd < hbnd && hbnd <= totl && totl < maxfrequency))
        {
                fprintf(stderr, "coding error: %d %d %d\n", lbnd, hbnd, totl);
                exit(1);
        }
#endif
	
	{	
		register codevalue range;
		range = S_high-S_low+1;
		high = S_low + range*hbnd/totl - 1;
		low  = S_low + range*lbnd/totl;
	}

	{	
		register codevalue H;
		register long bitstofollow;
		register int buffer, bitstogo;

		bitstofollow = S_bitstofollow;
		buffer = S_buffer;
		bitstogo = S_bitstogo;
		H = half;

		for (;;)
		{	
			if (high<H)
			{	
				BITPLUSFOLLOW(0);
			}
			else
				if (low>=H)
				{	
					BITPLUSFOLLOW(1);
					low -= H;
					high -= H;
				}
				else if (low>=firstqtr && high<thirdqtr)
				{	
					bitstofollow++;
					low -= firstqtr;
					high -= firstqtr;
				}
				else
					break;
			low += low;
			high += high; 
			high ++;
		}
		S_bitstofollow = bitstofollow;
		S_buffer = buffer;
		S_bitstogo = bitstogo;
		S_low = low;
		S_high = high;
	}
}

/*==================================*/

/* Made into a macro

codevalue
arithmetic_decode_target(totl)
	register int totl;
{	return (((S_value-S_low+1)*totl-1) / (S_high -S_low+1));
} */

/*==================================*/

void
arithmetic_decode(lbnd, hbnd, totl)
int lbnd, hbnd, totl;
{	
	register codevalue low, high;

#ifdef BECAREFUL
        if (!(0<=lbnd && lbnd < hbnd && hbnd <= totl && totl < maxfrequency))
        {
                fprintf(stderr, "coding error: %d %d %d\n", lbnd, hbnd, totl);
                exit(1);
        }
#endif

	{	
		register codevalue range;
		range = S_high-S_low+1;
		high = S_low + range*hbnd/totl - 1;
		low  = S_low + range*lbnd/totl;
	}

	{	
		register codevalue value, H;
		register int buffer, bitstogo;

		buffer = S_buffer;
		bitstogo = S_bitstogo;
		H = half;
		value = S_value;

		for (;;)
		{	
			if (high<H)
			{	/* nothing */
			}
			else if (low>=H)
			{	
				value -= H;
				low -= H;
				high -= H;
			}
			else if (low>=firstqtr && high<thirdqtr)
			{	
				value -= firstqtr;
				low -= firstqtr;
				high -= firstqtr;
			}
			else
				break;
			low += low;
			high += high; 
			high += 1;
			ADDNEXTINPUTBIT(value);
		}
		S_buffer = buffer;
		S_bitstogo = bitstogo;
		S_low = low;
		S_high = high;
		S_value	= value;
	}
}

/*==================================*/

void
startoutputingbits()
{	
	S_buffer = 0;
	S_bitstogo = 8;
}

void
startinputingbits()
{	
	S_buffer = 0;
	S_bitstogo = 0;
}

void
doneoutputingbits()
{	/* write another byte if necessary */
	if (S_bitstogo<8)
	{	
		putc(S_buffer>>S_bitstogo, stdout);
		cmpbytes += 1;
	}
}

/*==================================*/

void
startencoding()
{	
	S_low = 0;
	S_high = topvalue;
	S_bitstofollow = 0;
}

void
startdecoding()
{	
	register int i;
	register int buffer, bitstogo;
	S_low = 0;
	S_high = topvalue;
	S_value = 0;
	bitstogo = S_bitstogo;
	for (i=0; i<codevaluebits; i++)
		ADDNEXTINPUTBIT(S_value);
	S_bitstogo = bitstogo;
	S_buffer = buffer;
}

void
doneencoding()
{	
	register long bitstofollow;
	register int buffer, bitstogo;
	bitstofollow = S_bitstofollow;
	buffer = S_buffer;
	bitstogo = S_bitstogo;
	bitstofollow += 1;
	if (S_low<firstqtr)
	{	
		BITPLUSFOLLOW(0);
	}
	else
	{	
		BITPLUSFOLLOW(1);
	}
	S_bitstofollow = bitstofollow;
	S_buffer = buffer;
	S_bitstogo = bitstogo;
}
