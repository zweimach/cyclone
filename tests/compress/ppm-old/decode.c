/* Decodes the encoded input file. */
#include "stdio.h"
#include "model.h"
#include "arithcode.h"

void decode_file()
{
    int symbol, eof_sym;

    init_symbols();

    eof_sym = eof_symbol();

    startinputingbits();
    startdecoding();
    while ((symbol = decode_symbol()) < eof_sym)
        putchar( symbol );
}

void main(argc,argv)
int argc;
char *argv[];
{

    argv++;
    argv++;
    if (argc > 2)
	init_arguments(argc,argv,3);

    decode_file(stdin );
}
