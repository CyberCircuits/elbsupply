#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>

#include "lcd.h"

void LCD_init(uint8_t mode, uint8_t cursor){
	//R/W tied to GND => always write 
	LCD_CTRL_DDR |= (1<<LCD_E) | (1<<LCD_RS);
	LCD_CTRL_PORT |= (1<<LCD_E) | (1<<LCD_RS); // set E high --> no data yet, set RS high --> input interpreted as data
		
	LCD_DATA_DDR |= 0x0F;
	LCD_DATA_PORT &= ~(0x0F);
	
	_delay_ms(30); // wait for LCD 

	//###############################################
	// this is the initial soft reset bootup sequence
	LCD_CTRL_PORT &= ~(1<<LCD_RS); // set RS to low --> bytes interpreted as commands 
	LCD_CTRL_PORT |= (1<<LCD_E);
	
	LCD_DATA_PORT = (1<<LCD_DB5) | (1<<LCD_DB4);
	
	LCD_CTRL_PORT &= ~(1<<LCD_E);
	_delay_us(220);
	LCD_CTRL_PORT |= (1<<LCD_E);
	_delay_ms(5);
	
	LCD_CTRL_PORT &= ~(1<<LCD_E);
	_delay_us(220);
	LCD_CTRL_PORT |= (1<<LCD_E);
	_delay_us(64);
	
	LCD_CTRL_PORT &= ~(1<<LCD_E);
	_delay_us(220);
	LCD_CTRL_PORT |= (1<<LCD_E);
	_delay_us(64);
	
	LCD_DATA_PORT &= ~(1<<LCD_DB4);
	
	LCD_CTRL_PORT &= ~(1<<LCD_E);
	_delay_us(220);
	LCD_CTRL_PORT |= (1<<LCD_E);
	_delay_us(64);
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
	LCD_CTRL_PORT &= ~(1<<LCD_RS); // set RS to low --> bytes interpreted as commands 
	LCD_CTRL_PORT |= (1<<LCD_E);
	
	// send upper nibble
	LCD_DATA_PORT = (data & 0xF0)>>4;
	LCD_CTRL_PORT &= ~(1<<LCD_E);
	_delay_us(220);
	LCD_CTRL_PORT |= (1<<LCD_E);
	_delay_us(220);
	
	// send lower nibble
	LCD_DATA_PORT = (data & 0x0F);
	LCD_CTRL_PORT &= ~(1<<LCD_E);
	_delay_us(220);
	LCD_CTRL_PORT |= (1<<LCD_E);
	_delay_us(220);
}

void LCD_putc(char c){
	LCD_CTRL_PORT |= (1<<LCD_RS);
	LCD_CTRL_PORT |= (1<<LCD_E);
	
	// send upper nibble
	LCD_DATA_PORT = (c & 0xF0)>>4;
	LCD_CTRL_PORT &= ~(1<<LCD_E);
	_delay_us(220);
	LCD_CTRL_PORT |= (1<<LCD_E);
	_delay_us(220);
		
	// send lower nibble
	LCD_DATA_PORT = (c & 0x0F);
	LCD_CTRL_PORT &= ~(1<<LCD_E);
	_delay_us(220);
	LCD_CTRL_PORT |= (1<<LCD_E);
	_delay_us(220);
}

void LCD_puts(char *s){
	while(*s != '\0'){
		LCD_putc(*s);
		s++;
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
	
	if(line == 1){
		data = 0x80 + 0x00 + character;
		LCD_write(data);
	}
	else if(line == 2){
		data = 0x80 + 0x40 + character;
		LCD_write(data);
	}
	else if(line == 3){
		data = 0x80 + 0x10 + character;
		LCD_write(data);
	}
	else{
		data = 0x80 + 0x50 + character;
		LCD_write(data);
	}
}