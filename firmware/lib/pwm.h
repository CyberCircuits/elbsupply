#ifndef PWM_H
#define PWM_H

void PWM_init(void);
void PWM_setPSUOutV(uint16_t voltage);
void PWM_setPSUOutI(uint16_t voltage);

#endif /* PWM_H */
