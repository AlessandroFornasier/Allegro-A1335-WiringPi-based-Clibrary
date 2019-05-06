/*******************************************************

	University of Udine

	Programming the Allegro A1335 sensor

	Authors:
	- Alessandro Fornasier

	Requisites:
	- WiringPi library

	Compiling:
	cc -o reading main.c angle.c -lwiringPi
	
	Notes:
	File angles.txt must be placed into the angles
	folder and must be created automatically by
	linearization procedure only!

*******************************************************/

/*******************************************************

	Library:

*******************************************************/

#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "angle.h"

/*******************************************************

	Main function:

*******************************************************/

int main(int argc, char *argv[])
{
	uint8_t buffer[BUFFER_SIZE];	//buffer [15:0]
	float angle, tempAngle = 0;				//Angle value
	float temp = 0;					//Temperature value
	float field = 0;				//Field value
	char ch;
	char str[50];
	int i = 0;
	FILE *fp;
	
	if(argc != 3)
	{
		printf("USE: flag, delay\n\n\tflag: if it is set 1 the EEPROM will be write, otherwise no.\n\tdelay: is the time interval for reading the angle.\n\n");
		exit(1);
	}

	/*
		Setup:
		- Setup WiringPi
		- Setup WiringPiSPI MODE: 3
		- Wait 100 ms for self test
		- Unlock the device
		- Write options on EEPROM (Optional)
		- Write options on SRAM
		- Set segmented linearization coefficients (optional)
		- Read angle, temperature and field every time interval
	*/

	if (wiringPiSetupGpio() == -1)
	{
		printf("\nSetup wiringPi ERROR\n\n");
		return 1 ;
	}

	wiringPiSPISetupMode(0, SPI_CLOCK, MODE);

	printf("\nDo you want to make a reset? (S = Soft Reset | H = Hard Reset | N = No): ");
	scanf("%c", &ch);
	if(ch == 's' || tolower(ch) == 's')
	{
		printf("\nSoft Reset...");
		delay(500);
		SoftReset(0, buffer);
		printf("\nSoft Reset done\n");
	}
	else if(ch == 'h' || tolower(ch) == 'h')
	{
		printf("\nHard Reset...");
		delay(500);
		HardReset(0, buffer);
		printf("\nHard Reset done\n");
	}
	
	/*Cleaning input buffer*/
	while((getchar()) != '\n');

	if(checkSelfTest(0, buffer) == NOERROR)
	{
		printf("\nUnlocking device...");
		delay(500);
		if(UnlockDevice(0, buffer) == NOERROR)
		{
			printf("\nDevice Unlocked\n");
			if(strcmp(argv[1], "1") == 0)
			{
				printf("\nSetup EEPROM...");
				delay(500);
				if(EEPROMSetup(0, buffer) == NOERROR)
					printf("\nSetup EEPROM done\n");
				else
				{
					printf("\nSetup EEPROM ERROR\n");
					return 1 ;
				}
			}
			printf("\nSetup SRAM...");
			delay(500);
			if(SRAMsetup(0, buffer) == NOERROR)
					printf("\nSetup SRAM done\n");
				else
				{
					printf("\nSetup SRAM ERROR\n");
					return 1 ;
				}
			printf("\nDo you want to start the Segmented Linearization Coefficients setup? (Y = Yes | N = No | R = Read from file): ");
			scanf("%c", &ch);
			if(ch == 'y' || tolower(ch) == 'y')
			{
				/*Cleaning input buffer*/
				while((getchar()) != '\n');
					
				strcpy(str,  "angles.txt");
				fp = fopen(str, "a");
					
				if(fp == NULL) 
				{   
					printf("Error! Could not open file\n"); 
					return 1;
				} 
				
				printf("\nSetup SL Coefficients...");
				delay(500);
				
				for(i = 1; i<=15; i++)
				{
					printf("\nTurn magnet of 22.5 degrees and after that press any key for getting angle ");
					getchar();

					/*Get the current angle*/
					printf("\nMeasuring the angle...\n\n");
					delay(500);
					
					if((angle = getAngle(0, buffer)) == (float)ERROR)
						printf("Angle reading ERROR\n");

					delay(500);
						
					fprintf(fp, "%f\n", angle);
					printf("Written angle: %f\n", angle);
					
					if(SetSLCoefficients(0, buffer, angle, i) == NOERROR)
						printf("\nSetup SL Coefficients done\n");
					else
					{
						printf("\nSetup SL Coefficients ERROR\n");
						return 1 ;
					}
				}
				fclose(fp);
			}
			else if(ch == 'r' || tolower(ch) == 'r')
			{
				strcpy(str,  "angles.txt");
				fp = fopen(str, "r");
					
				if (fp == NULL) 
				{   
					printf("Error! Could not open file\n"); 
					return 1;
				}
				
				i = 1;
					
				while(fgets(str, 100, fp) != NULL)
				{
					strtok(str, "\n");			//don't work if a line is just \n
					tempAngle = atof(str);
						
					if(SetSLCoefficients(0, buffer, tempAngle, i) == NOERROR)
						printf("\nSetup SL Coefficient done\n");
					else
					{
						printf("\nSetup SL Coefficient ERROR\n");
						return 1 ;
					}
						
					i++;
						
					if(i > 16)
					{
						printf("\nSetup SL Coefficient ERROR\n");
						return 1 ;
					}
				}
					
				fclose(fp);
			}
		
			printf("\nStart angle reading loop...\n\n");
			while(1)
			{
				delay(atoi(argv[2]));
				if((angle = getAngle(0, buffer)) != (float)ERROR)
					printf("Angle: %f\n", angle);
				else
					printf("Angle reading ERROR\n");
				temp = getTemp(0, buffer);
				printf("Temp: %f\n", temp);
				field = getField(0, buffer);
				printf("Field: %f\n\n", field);				
			}
		}
		else
		{
			printf("\nUnlocking device Error\n");
			return 1;
		}
	}
	else
	{
		printf("\nSelf test Error\n");
		return 1;
	}
	return 1;
}
