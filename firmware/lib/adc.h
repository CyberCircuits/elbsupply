#ifndef ADC_H
#define ADC_H

void ADC_init(void);
uint16_t ADC_readRaw(uint8_t channel);
uint16_t ADC_readPSUOutV(void);
uint16_t ADC_readPSUOutI(void);

#endif /* LCD_H */
