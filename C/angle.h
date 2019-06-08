#ifndef ANGLE_H__
#define ANGLE_H__

/*stdint.h has the definitions of int8_t, int16_t, ...*/
#include <stdint.h>

/*******************************************

	Definitions:

*******************************************/

#define SPI_CLOCK 1000000	//SPI Clock Rate 1 MHz
#define MODE 3				//SPI MODE
#define BUFFER_SIZE 2		//2 bytes buffer
#define W 0x40				//Write Code [15:14] = 01
#define R 0x00				//Read Code [15:14] = 00
#define NOERROR 0			//Codice assenza errore
#define ERROR -1			//Codice errore generico
#define MINANGLE 0			//Min angle (in Degrees)
#define MAXANGLE 90			//Max angle (in Degrees)
#define DIRECTION 0			//Direction of rotation (0 = Clockwise | 0x12 = Counterclockwise)
#define SHORTSTROKE 0		//Short Stroke Application (1 = Yes | 0 = No)
#define GAINOFFSET 0		//Angle = Measured angle - gain offset (in Degrees)
#define GAIN 0				//Gain = (360 / (MAXANGLE - MINANGLE)) - 1
#define LINEARIZATION 1		//Segmented Linearization (1 = Yes | 0 = No)
#define ENCODER 1			//Encoder for linearization setup (1 = Internal | 0 = External) (External Encoder common used)

/*******************************************

	Prototypes:

*******************************************/

int setBuffer(uint8_t buffer[], uint8_t rw, uint8_t reg, uint8_t data);
int ExtendedWrite(int cs, uint8_t buffer[], uint16_t address, uint32_t value);
int ExtendedRead(int cs, uint8_t buffer[], uint16_t address, uint32_t *value);
int SetProcessorStateToRun(int cs, uint8_t buffer[]);
int SetProcessorStateToIdle(int cs, uint8_t buffer[]);
int UnlockDevice(int cs, uint8_t buffer[]);
int EEPROMSetup(int cs, uint8_t buffer[]);
int SRAMsetup(int cs, uint8_t buffer[]);
int SetSLCoefficients(int cs, uint8_t buffer[], float angle, int i);
int checkSelfTest(int cs, uint8_t buffer[]);
int SoftReset(int cs, uint8_t buffer[]);
int HardReset(int cs, uint8_t buffer[]);
float getAngle(int cs, uint8_t buffer[]);
float getTemp(int cs, uint8_t buffer[]);
float getField(int cs, uint8_t buffer[]);

#endif
