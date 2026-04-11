#include "stm32f1xx_hal.h"
#include  "stdio.h"

extern UART_HandleTypeDef huart1;
int fputc(int ch, FILE *f)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF); 
  return ch;
}


