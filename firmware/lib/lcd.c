#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>

#include "lcd.h"

void LCD_init(uint8_t mode, uint8_t cursor){
	//R/W tied to GND => always write 
	LCD_CTRL_DDR |= (1<<LCD_E) | (1<<LCD_RS);
	LCD_CTRL_PORT |= (1<<LCD_E) | (1<<LCD_RS); // set E high --> no data yet, set RS high --> input interpreted as data
		
	LCD_DATA_DDR |= (1<<LCD_DB7) | (1<<LCD_DB6) | (1<<LCD_DB5) | (1<<LCD_DB4);
	LCD_DATA_PORT &= ~((1<<LCD_DB7) | (1<<LCD_DB6) | (1<<LCD_DB5) | (1<<LCD_DB4));
	
	_delay_ms(30); // wait for LCD 

	//###############################################
	// this is the initial soft reset bootup sequence
	LCD_CTRL_PORT &= ~(1<<LCD_RS); // set RS to low --> bytes interpreted as commands 
	LCD_CTRL_PORT |= (1<<LCD_E);
	
	LCD_DATA_PORT |= (1<<LCD_DB5) | (1<<LCD_DB4);
	
	LCD_CTRL_PORT &= ~(1<<LCD_E);
	_delay_us(220);
	LCD_CTRL_PORT |= (1<<LCD_E);
	_delay_ms(5);
	
	LCD_CTRL_PORT &= ~(1<<LCD_E);
	_delay_us(220);
	LCD_CTRL_PORT |= (1<<LCD_E);
	_delay_us(220);
	
	LCD_CTRL_PORT &= ~(1<<LCD_E);
	_delay_us(220);
	LCD_CTRL_PORT |= (1<<LCD_E);
	_delay_us(220);
	
	LCD_DATA_PORT &= ~(1<<LCD_DB4);
	
	LCD_CTRL_PORT &= ~(1<<LCD_E);
	_delay_us(220);
	LCD_CTRL_PORT |= (1<<LCD_E);
	_delay_us(220);
	//###############################################
	
	
	LCD_write(mode); // 4 bits, 2 lines, 5x8 point matrix
	_delay_us(55);
	LCD_write(0x08); // display off
	_delay_us(55);
	LCD_write(0x01); // clear display
	_delay_ms(2);
	LCD_write(0x06); // cursor increments, no display shift
	_delay_us(55);
	LCD_write(cursor); // enable display 
	_delay_us(100);	
	LCD_write(0x02);
	_delay_ms(30);
}

void LCD_write(uint8_t data){
	uint8_t tmp;
	
	LCD_CTRL_PORT &= ~(1<<LCD_RS); // set RS to low --> bytes interpreted as commands 
	//LCD_CTRL_PORT |= (1<<LCD_E);
	
	// send upper nibble
	tmp = LCD_DATA_PORT;
	tmp &= ~((1<<LCD_DB7) | (1<<LCD_DB6) | (1<<LCD_DB5) | (1<<LCD_DB4));
	tmp |= (data & 0xF0)>>2; // this is app specific!!!
	
	LCD_DATA_PORT = tmp;
	LCD_CTRL_PORT |= (1<<LCD_E);
	_delay_us(LCD_CLK_PERIOD);
	LCD_CTRL_PORT &= ~(1<<LCD_E);
	_delay_us(LCD_CLK_PERIOD);
	
	
	// send lower nibble
	tmp = LCD_DATA_PORT;
	tmp &= ~((1<<LCD_DB7) | (1<<LCD_DB6) | (1<<LCD_DB5) | (1<<LCD_DB4));
	tmp |= (data & 0x0F)<<2; // this is app specific!!!
	
	LCD_DATA_PORT = tmp;
	LCD_CTRL_PORT |= (1<<LCD_E);
	_delay_us(LCD_CLK_PERIOD);
	LCD_CTRL_PORT &= ~(1<<LCD_E);
	_delay_us(LCD_CLK_PERIOD);
	
	LCD_DATA_PORT &= ~((1<<LCD_DB7) | (1<<LCD_DB6) | (1<<LCD_DB5) | (1<<LCD_DB4));
}

void LCD_putc(char c){
	uint8_t tmp;
	
	LCD_CTRL_PORT |= (1<<LCD_RS);
	//LCD_CTRL_PORT |= (1<<LCD_E);
	
	// send upper nibble
	tmp = LCD_DATA_PORT;
	tmp &= ~((1<<LCD_DB7) | (1<<LCD_DB6) | (1<<LCD_DB5) | (1<<LCD_DB4));
	tmp |= (c & 0xF0)>>2; // this is app specific!!!
	
	LCD_DATA_PORT = tmp;
	LCD_CTRL_PORT |= (1<<LCD_E);
	_delay_us(LCD_CLK_PERIOD);
	LCD_CTRL_PORT &= ~(1<<LCD_E);
	_delay_us(LCD_CLK_PERIOD);
	
		
	// send lower nibble
	tmp = LCD_DATA_PORT;
	tmp &= ~((1<<LCD_DB7) | (1<<LCD_DB6) | (1<<LCD_DB5) | (1<<LCD_DB4));
	tmp |= (c & 0x0F)<<2; // this is app specific!!!
	
	LCD_DATA_PORT = tmp;
	LCD_CTRL_PORT |= (1<<LCD_E);
	_delay_us(LCD_CLK_PERIOD);
	LCD_CTRL_PORT &= ~(1<<LCD_E);
	_delay_us(LCD_CLK_PERIOD);
	
	LCD_DATA_PORT &= ~((1<<LCD_DB7) | (1<<LCD_DB6) | (1<<LCD_DB5) | (1<<LCD_DB4));	
}

void LCD_puts(char *s){
	for (uint8_t i = 0; i < 16; i++)
	{
		LCD_putc(s[i]);
	}
}

void LCD_reset_display(void){
	LCD_write(0x01);
	_delay_ms(2);
}

void LCD_reset_cursor(void){
	LCD_write(0x02);
	_delay_ms(2);
}

void LCD_set_cursor(uint8_t line, uint8_t character){
	uint8_t data = 0;
	
	switch(line)
	{
		case 0:
			data = 0x80 + 0x00 + character;
			LCD_write(data);
			break;
		
		case 1:
			data = 0x80 + 0x40 + character;
			LCD_write(data);
			break;
		
		case 2:
			data = 0x80 + 0x10 + character;
			LCD_write(data);
			break;
			
		case 3:
			data = 0x80 + 0x50 + character;
			LCD_write(data);
			break;
			
		default: break;
	}
}

/* Sets the cursor to the absolute position on the display */ 
void LCD_setpos(uint8_t curpos)
{	
	LCD_set_cursor((curpos/16), (curpos%16));
}
