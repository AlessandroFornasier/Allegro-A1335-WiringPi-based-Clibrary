/*******************************************

	University of Udine

	Library for using Allegro A1335 with
	Raspberry Pi

	Authors:
	- Alessandro Fornasier

*******************************************/

/*******************************************

	NOTE:

	- CRC diabled by default
	- Self Test enabled by default
	- To write a full 16 bit serial register, two Write commands are required one for even and one for odd byte addresses
	- To write a only a single byte serial register, one Write commands are required specifying the address
	- To read a full 16 bit serial register, two equal Read commands are required specifying even byte address
	- To read only a single byte serial register, two equal Read commands are required specifying odd byte address, the 8 MSB will be 0

*******************************************/

/*******************************************

	Library:

*******************************************/

#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <unistd.h>
#include "angle.h"

/*******************************************

	Functions:

*******************************************/

int setBuffer(uint8_t buffer[], uint8_t rw, uint8_t reg, uint8_t data)
{
	buffer[0] = rw|reg;	//[15:8]
	buffer[1] = data;	//[7:0]
}

int ExtendedWrite(int cs, uint8_t buffer[], uint16_t address, uint32_t value)
{
	uint32_t timeout;

	/*Write into EWA (Extended Write Address) register at addresses 0x02:0x03*/
	setBuffer(buffer, W, 0x02, (uint8_t)((address >> 8) & 0x00FF));
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, W, 0x03, (uint8_t)(address & 0x00FF));
	wiringPiSPIDataRW(cs, buffer, 2);

	/*Write into EWD (Extended Write Data) register at addresses 0x04:0x07*/
	setBuffer(buffer, W, 0x04, (uint8_t)((value >> 24) & 0x000000FF));
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, W, 0x05, (uint8_t)((value >> 16) & 0x000000FF));
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, W, 0x06, (uint8_t)((value >> 8) & 0x000000FF));
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, W, 0x07, (uint8_t)(value) & 0x000000FF);
	wiringPiSPIDataRW(cs, buffer, 2);

	/*Write EXW (Extended Execute Write = Start writing process) into EWCS (Extended Write Control and Status) register at address 0x08*/
	setBuffer(buffer, W, 0x08, 0x80);
	wiringPiSPIDataRW(cs, buffer, 2);

	/*If writing, program parameters in the EEPROM (addressing range (0x306 – 0x319)) send the programming pulses*/
	if((address >= 0x306) && (address <= 0x319))
    	{
        	/*Setup gpio BCM23 for sending program pulses*/
			pinMode(23, OUTPUT);

			/*Send programming pulse (ATTENTION! NEED SUPPLEMENTARY HARWARE TO REACH 18V)*/
			digitalWrite(23, HIGH);
			delay(10);
			digitalWrite(23, LOW);
			delayMicroseconds(400);
			digitalWrite(23, HIGH);
			delay(10);
			digitalWrite(23, LOW);
	}

	timeout = millis() + 100;

	/*Wait untill the write operation is complete*/
	do
    {
		/*Read WDN (Write Done to Extended Address) into EWCS (Extended Write Control and Status) register at address 0x08*/
		setBuffer(buffer, R, 0x08, 0x00);
		wiringPiSPIDataRW(cs, buffer, 2);
		setBuffer(buffer, R, 0x08, 0x00);
		wiringPiSPIDataRW(cs, buffer, 2);

		/*if write operation takes more than 100 us return a TIMEOUTError*/
		if (timeout < millis())
           	return ERROR;
		
	} while((buffer[1] & 0x01) != 0x01);

	return NOERROR;
}

int ExtendedRead(int cs, uint8_t buffer[], uint16_t address, uint32_t *value)
{
	uint32_t timeout;
	
	/*Clear value*/
	*value = 0x00000000;

	/*Write into ERA (Extended Read Address) register at address 0x0A:0x0B*/
	setBuffer(buffer, W, 0x0A, (uint8_t)((address >> 8) & 0x00FF));
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, W, 0x0B, (uint8_t)(address & 0x00FF));
	wiringPiSPIDataRW(cs, buffer, 2);
	
	/*Write EXR (Extended Execute Read = Start reading process) into ERCS (Extended Read Control and Status) register at address 0x0C*/
	setBuffer(buffer, W, 0x0C, 0x80);
	wiringPiSPIDataRW(cs, buffer, 2);
	
	timeout = millis() + 100;
	
	/*Wait untill the read operation is complete*/
	do
	{
		/*Read RDN (Read Done to Extended Address) into ERCS (Extended Read Control and Status) register at address 0x0C*/
      		setBuffer(buffer, R, 0x0C, 0x00);
		wiringPiSPIDataRW(cs, buffer, 2);
		setBuffer(buffer, R, 0x0C, 0x00);
		wiringPiSPIDataRW(cs, buffer, 2);
		
		/*if read operation takes more than 100 us return a TIMEOUTError*/
        	if (timeout < millis())
            		return ERROR;

    	} while((buffer[1] & 0x01) != 0x01);
	
	/*Read the ERD (Extended Read Data) register at address 0x0E:0x11*/
	setBuffer(buffer, R, 0x0E, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, R, 0x0E, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);
	
	*value = ((uint32_t)buffer[0] << 24) + ((uint32_t)buffer[1] << 16);
	
	setBuffer(buffer, R, 0x10, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, R, 0x10, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);
	
	*value = *value + ((uint32_t)buffer[0] << 8) + ((uint32_t)buffer[1]);
	
	return NOERROR;
}

int checkSelfTest(int cs, uint8_t buffer[])
{
	uint32_t data = 0x00000000;

	/*Read the XERR register at address 0x26:0x27*/
	setBuffer(buffer, R, 0x26, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, R, 0x26, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);

	/*Check the ST bit*/
	if((buffer[1] & 0x01) == 0x00)
		return NOERROR;
	else
		return ERROR;
}

int SoftReset(int cs, uint8_t buffer[])
{
	/* Write soft reset command into CTRL (Control) register at address 0x1E:0x1F*/
	setBuffer(buffer, W, 0x1F, 0xB9);
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, W, 0x1E, 0x16);
	wiringPiSPIDataRW(cs, buffer, 2);

	return NOERROR;
}

int HardReset(int cs, uint8_t buffer[])
{
	/* Write soft reset command into CTRL (Control) register at address 0x1E:0x1F*/
	setBuffer(buffer, W, 0x1F, 0xB9);
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, W, 0x1E, 0x32);
	wiringPiSPIDataRW(cs, buffer, 2);

	return NOERROR;
}

int SetProcessorStateToIdle(int cs, uint8_t buffer[])
{ 
    /*Write the idle state command into CTRL (Control) register at address 0x1E:0x1F*/
	setBuffer(buffer, W, 0x1E, 0x80);
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, W, 0x1F, 0x46);
	wiringPiSPIDataRW(cs, buffer, 2);
    
	/*Wait 1 ms*/
	delay(1);

	/*Check if the processor is in the idle state - Read the status register at address 0x22:0x23*/
	setBuffer(buffer, R, 0x22, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, R, 0x22, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);

	if((buffer[1] & 0xFF) != 0x10)
		return ERROR;
	else
		return NOERROR;
}

int SetProcessorStateToRun(int cs, uint8_t buffer[])
{
	/*Write the run state command into CTRL (Control) register at address 0x1E:0x1F*/
	setBuffer(buffer, W, 0x1E, 0xC0);
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, W, 0x1F, 0x46);
	wiringPiSPIDataRW(cs, buffer, 2);
	
    /*Wait 1 ms*/
	delay(1);
    
	/*Check if the processor is in the run state - Read the status register at address 0x22:0x23*/
	setBuffer(buffer, R, 0x22, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, R, 0x22, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);

	if((buffer[1] & 0xFF) != 0x11)
		return ERROR;
	else
		return NOERROR;
}

int UnlockDevice(int cs, uint8_t buffer[])
{	
	/*Unlock the device by writing 0x27811F77 to extended address 0xFFFE*/
	if(ExtendedWrite(cs, buffer, 0xFFFE, 0x27811F77) == ERROR)
		return ERROR;
	else
		return NOERROR;
}

int EEPROMSetup(int cs, uint8_t buffer[])
{	
	uint32_t flagsAndZeroOffset;
	uint16_t angle;
	
	/*After the A1335 has been unlocked for writing, the processor must be set to Idle mode*/
	if(SetProcessorStateToIdle(cs, buffer) == ERROR)
		return ERROR;
	
	/*	Preload register with data
			ATTENTION!
			All configuration can be set every time writing the SRAM, configure the EEPROM only with configuration that will be used by default
	
		Write the EEPROM configuration
			ATTENTION!
			External hardware is needed for programming EEPROM
			
		This 2 steps must be done with ExtendedWrite() function
	*/
	
	if(SetProcessorStateToRun(cs, buffer) == ERROR)
		return ERROR;
	
	return NOERROR;
}

int SRAMsetup(int cs, uint8_t buffer[])
{	
	uint32_t data;
	uint16_t addrContent;
	uint16_t MaxAngle;
	uint16_t MinAngle;
	uint16_t GainOffset;
	float angle;
	
	/*
		Write SRAM configuration
		- ORATE
		- Short Stroke Application
		- PreLinearization Rotation
		- Gain Offset
		- Gain Adjust (SSA)
		- Max/Min angle (SSA)
		- PreLinearization 0 Offset (SL)
		- Segmented Linearization
		- PostLinearization 0 Offset (Zero Offset)
		- PostLinearization Rotation (same as pre)
		- Angle clamping (SSA)
	*/

	if(SetProcessorStateToIdle(cs, buffer) == ERROR)
		return ERROR;

	/*Set the ORATE (Output RATE to 128 sample -> 4ms refresh time) by writing 0x00000007 to extended address 0xFFD0*/
	if(ExtendedWrite(cs, buffer, 0xFFD0, 0x00000007) == ERROR)
		return ERROR;

	if(SetProcessorStateToRun(cs, buffer) == ERROR)
		return ERROR;

	/*
		Enable by writing SRAM at address 0x06: 
			- Short Stroke Application
			- Prelinearization rotation
			- Rotation Direction
			- Enable Segmented Linearization (Internal Encoder)
 	*/

	data = 0x0FFFFFFF & (((uint32_t)LINEARIZATION << 26) + ((uint32_t)SHORTSTROKE << 24) + ((uint32_t)DIRECTION << 21) + ((uint32_t)ENCODER << 20));
	if(ExtendedWrite(cs, buffer, 0x0006, data)== ERROR)
		return ERROR;

	/*
		Enable by writing SRAM a address 0x01:0x03
			- Min/Max Angle
			- Clamp Hi/Lo
			- Gain Offset
			- Gain values
	*/
	
	if(SHORTSTROKE == 1)
	{
		MaxAngle = MAXANGLE * 65536 / 360;
		MinAngle = MINANGLE * 65536 / 360;
		data = (uint32_t)MaxAngle << 16 + (uint32_t)MinAngle;
		if(ExtendedWrite(cs, buffer, 0x0001, data)== ERROR)
			return ERROR;
		if(ExtendedWrite(cs, buffer, 0x0002, data)== ERROR)
			return ERROR;
	}
	
	GainOffset = GAINOFFSET * 65536/360;
	data = ((uint32_t)GAINOFFSET << 16) + (uint32_t)(((uint16_t)GAIN << 8) + (uint16_t)(100*(float)(GAIN - (uint16_t)GAIN)));
	
	if(ExtendedWrite(cs, buffer, 0x0003, data)== ERROR)
		return ERROR;

	/*Bypass the Segmented Linearization Algorithm*/
	/*Read the SRAM at address 0x06*/
	if(ExtendedRead(cs, buffer, 0x0006, &data) == ERROR)
		return ERROR;

	/*Set SB to 1 (Prevent Segmented Linearization)*/
	data += 0x02000000;

	/*Write the SRAM at address 0x06*/
	if(ExtendedWrite(cs, buffer, 0x0006, data) == ERROR)
		return ERROR;

	/*Read the SRAM at address 0x06*/
	if(ExtendedRead(cs, buffer, 0x0006, &data) == ERROR)
		return ERROR;

	/*Set angle offset to 0 (2 bytes flags, 2 bytes angle offset) preserving the flag*/
	data &= 0xFFFF0000;
	
	if(ExtendedWrite(cs, buffer, 0x0006, data)== ERROR)
		return ERROR;
	
	/*Get the current angle (in angle resolution units) reading the primary register 0x20:0x21 after 100ms*/
	delay(100);
	setBuffer(buffer, R, 0x20, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, R, 0x20, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);

	/*Set actual angle as zero (ATTENTION: SRAM 0x06 register bits are [15:4] angle and [3:0] zero)*/ 
	addrContent = ((uint16_t)(buffer[0] & 0x0F) << 12) + ((uint16_t)buffer[1] << 4);
	data = (data & 0xFFFF0000) | ((uint32_t)addrContent & 0x0000FFF0);
	
	/*Write the SRAM at address 0x06*/
	if(ExtendedWrite(cs, buffer, 0x0006, data) == ERROR)
		return ERROR;
	
	/*Get the current angle (in degrees) after 100ms*/
	delay(100);
	angle = getAngle(cs, buffer);
	
	/*Set the PreLinearization 0 Offset by writing SRAM at address 0x13*/
	data = (uint32_t)((65536 / 365) * angle) << 16;
	if(ExtendedWrite(cs, buffer, 0x0013, data)== ERROR)
		return ERROR;

	return NOERROR;
}

int SetSLCoefficients(int cs, uint8_t buffer[], float angle, int i)
{
	uint16_t address;
	uint32_t data;

	/*Calculate address*/
	address = 0x000C + (uint16_t)((i-1) / 2);

	/*Read the SRAM at address*/
	if(ExtendedRead(cs, buffer, address, &data) == ERROR)
		return ERROR;

	if((i % 2) != 0)
	{
		/*Odd coefficient*/
		data &= 0xFFFF0000;
		data += (uint32_t)((65536 / 365) * angle);
	}
	else
	{
		/*Even coefficient*/
		data &= 0x0000FFFF;
		data += ((uint32_t)((65536 / 365) * angle) << 16) & 0xFFFF0000;
	}
	
	/*Write the SRAM at address*/
	if(ExtendedWrite(cs, buffer, address, data)== ERROR)
		return ERROR;

	/*Disable the Segmented Linearization algorithm Bypass*/
	if(i == 15)
	{
		/*Read the SRAM at address 0x06*/
		if(ExtendedRead(cs, buffer, 0x0006, &data) == ERROR)
			return ERROR;

		/*Set SB to 0 (Allow Segmented Linearization)*/
		data &= 0xFDFFFFFF;

		/*Write the SRAM at address 0x06*/
		if(ExtendedWrite(cs, buffer, 0x0006, data) == ERROR)
			return ERROR;
	}
	return NOERROR;
}

float getAngle(int cs, uint8_t buffer[])

{
	float angle;
	uint16_t input, cnt;
	int i;

	/*Get the current angle reading the primary register 0x20:0x21*/
	setBuffer(buffer, R, 0x20, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, R, 0x20, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);
	
	input = ((uint16_t)buffer[0] << 8) + (uint16_t)buffer[1];
	cnt = 0x0000;	

	/*Count up the number of lsb in the input*/
	for(i = 0; i < 16; i++)
	{
		if((input & 0x0001) == 0x0001)
			cnt++;
		
		input >>= 1;
	}

	if((cnt & 0x0001) == 0x0001)
		angle = (float)(((((uint16_t)buffer[0] << 8) + ((uint16_t)buffer[1])) & 0x0FFF) * 360.0 / 4096.0 );
	else
		angle = (float)ERROR;

	return angle;
}

float getTemp(int cs, uint8_t buffer[])
{
	float temp = 0;

	/*Get the current temperature reading the primary register 0x28:0x29*/
	setBuffer(buffer, R, 0x28, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, R, 0x28, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);
	
	temp = (float)((((((uint16_t)buffer[0] << 8) + ((uint16_t)buffer[1])) & 0x0FFF) / 8.0) - 273.16);
	
	return temp;
}

float getField(int cs, uint8_t buffer[])
{
	float field = 0;

	/*Get the current field reading the primary register 0x2A:0x2B*/
	setBuffer(buffer, R, 0x2A, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);
	setBuffer(buffer, R, 0x2A, 0x00);
	wiringPiSPIDataRW(cs, buffer, 2);
	
	field = (float)((((uint16_t)buffer[0] << 8) + ((uint16_t)buffer[1])) & 0x0FFF);
	
	return field;
}
