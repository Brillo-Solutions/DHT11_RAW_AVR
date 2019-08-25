#define F_CPU 16000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define DHTPIN 2
#define LCD_PORT PORTD
#define rs 2
#define en 3

void writeDataNibble(uint8_t d)
{
	LCD_PORT = d;
	LCD_PORT |= (1 << rs);
	LCD_PORT |= (1 << en);
	_delay_ms(1);
	LCD_PORT &= ~(1 << en);
}

void breakDataByte(uint8_t dataByte)
{
	char tempValue;
	tempValue = dataByte & 0xF0;
	writeDataNibble(tempValue);
	tempValue = ((dataByte << 4) & 0xF0);
	writeDataNibble(tempValue);
}

void writeInstNibble(uint8_t c)
{
	LCD_PORT = c;
	LCD_PORT &= ~(1 << rs);
	LCD_PORT |= (1 << en);
	_delay_ms(1);
	LCD_PORT &= ~(1 << en);
}

void breakInstByte(uint8_t instByte)
{
	char tempValue;
	tempValue = instByte & 0xF0;
	writeInstNibble(tempValue);
	tempValue = ((instByte << 4) & 0xF0);
	writeInstNibble(tempValue);
}

void initDisplay(void)
{
	breakInstByte(0x02);
	_delay_ms(1);
	breakInstByte(0x28);
	_delay_ms(1);
	breakInstByte(0x01);
	_delay_ms(1);
	breakInstByte(0x0C);
	_delay_ms(1);
}

void putString(const char *s)
{
	while (*s != '\0')
	{
		breakDataByte(*s);
		s++;
	}
}

void putChar(uint8_t c)
{
	breakDataByte(c);
}

void requestToSensor()
{
	DDRB |= (1 << DHTPIN);
	
	PORTB &= ~(1 << DHTPIN);
	_delay_ms(18);
	PORTB |= (1 << DHTPIN);
}

void responseFromSensor()
{
	DDRB &= ~(1 << DHTPIN);
	
	while(PINB & (1 << DHTPIN));
	while((PINB & (1 << DHTPIN)) == 0);
	while(PINB & (1 << DHTPIN));
}

uint8_t getDataFromSensor()
{
	uint8_t rxdByte = 0;
	for (int k = 0; k <= 7; k++)
	{
		while((PINB & (1 << DHTPIN)) == 0);
		_delay_us(50);
		
		if(PINB & (1 << DHTPIN))
		rxdByte = (rxdByte << 1) | (0x01);
		else
		rxdByte = (rxdByte << 1);
		while(PINB & (1 << DHTPIN));
	}
	return rxdByte;
}

int main()
{
	uint8_t iRelativeHumidity, dRelativeHumidity, dataSum;
	uint8_t dTemperature, iTemperature, checkSum;
	DDRD = 0xFF;
	initDisplay();
	_delay_ms(1000);
	while (1)
	{
		requestToSensor();
		responseFromSensor();
		
		iRelativeHumidity = getDataFromSensor();
		dRelativeHumidity = getDataFromSensor();
		iTemperature = getDataFromSensor();
		dTemperature = getDataFromSensor();
		checkSum = getDataFromSensor();
		
		dataSum = iRelativeHumidity + dRelativeHumidity + iTemperature + dTemperature;
		
		if (dataSum == checkSum)
		{
			breakInstByte(0x85);
			_delay_ms(1);
			putString("T:");
			putChar(0x30 | (iTemperature) / 10);
			putChar(0x30 | (iTemperature) % 10);
			putString("\337C");
			
			breakInstByte(0xC5);
			_delay_ms(1);
			putString("RH:");
			putChar(0x30 | (iRelativeHumidity) / 10);
			putChar(0x30 | (iRelativeHumidity) % 10);
			
			putString("%");
		}
		else
			putString("Error!");
		_delay_ms(2000);
	}
}