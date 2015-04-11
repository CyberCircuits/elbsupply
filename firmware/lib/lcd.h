#ifndef LCD_H
#define LCD_H

#define LCD_CTRL_DDR DDRC
#define LCD_CTRL_PORT PORTC
#define LCD_DATA_DDR DDRC
#define LCD_DATA_PORT PORTC
#define LCD_RS PC0
#define LCD_E  PC1
#define LCD_DB4 PC2
#define LCD_DB5 PC3
#define LCD_DB6 PC4
#define LCD_DB7 PC5

#define LCD_CLK_PERIOD 100

void LCD_init(uint8_t mode, uint8_t cursor);
void LCD_write(uint8_t data);
void LCD_putc(char c);
void LCD_puts(char *s);
void LCD_reset_display(void);
void LCD_reset_cursor(void);
void LCD_set_cursor(uint8_t line, uint8_t character);
void LCD_setpos(uint8_t curpos);

#endif /* LCD_H */
