/* programma per convertire miglia in chilometri */

#include <stdio.h>

#define KM_PER_MI	1609	/* fattore conversione miglia-chilometri */

int main(void)
{
	/* dichiarazione delle variabili locali */
	double miglia;		/* input: distanza in miglia */
	double chilometri; 	/* output: distanza in chilometri */

	/* acquisire la distanza in miglia */	
	printf("Digitare la distanza in miglia: ");
	scanf("%lf",
	     &miglia);		

	/* convertire la distanza in chilometri */
	chilometri = miglia * KM_PER_MI;

	/* comunicare la distanza in chilometri */
	printf("La stessa distanza in chilometri é: %f\n",
	      chilometri);		
	return(0);
}
	      	       	

