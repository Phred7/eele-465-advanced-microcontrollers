#include <msp430.h> 


/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	int n;
	n = 2;

	int readings[n];

	int i;
	for (i=0; i < 2; i++){
	    readings[i] = 1;
	}

	return 0;
}
