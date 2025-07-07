#ifndef __ADC_H
#define	__ADC_H

#include <stm32l1xx.h>
#define MAX_CONVERTD  4095
#define VREF      3300
extern __IO uint16_t ADC_ConvertedValue;
void ADC_DMA_Config(void);
#endif /* __ADC_H */

