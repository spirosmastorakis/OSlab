/* test.c
 * 
 * Test the implemented driver
 *
 * Spyridon (Spiros) Mastorakis
 * George Matikas
 *
 */
 
 
#include <stdio.h>
#include <errno.h>
#include <string.h>


int main() {
	
	char input[20],output[50],meas[6];
	float temp;
	int sensor,count,i;
	FILE *in,*out;
	
	printf("Choose sensor: ");
	scanf("%d",&sensor);
	
	printf("Choose measurement (batt/temp/light): ");
	scanf("%s",meas);
	
	printf("How many measurements: ");
	scanf("%d",&count);
	
	printf("Name of file to be created: ");
	scanf("%s",output);
	
	sprintf(input,"/dev/lunix%d-%s",sensor,meas);
	
	in = fopen(input,"r");
	
	if (in == NULL) {
		printf("Error opening sensor file %s: %s.\n",input,strerror(errno));
		return 1;
	}
	
	out = fopen(output,"w");
	
	if (out == NULL) {
		printf("Error creating output file: %s.\n",strerror(errno));
		return 1;
	}
	
	for (i=0;i<count;i++) {
		fscanf(in,"%f",&temp);
		fprintf(out,"%.3f\n",temp);
	}
	
	fclose(in);
	fclose(out);
	
	return 0;
}