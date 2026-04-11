#include "stm32f1xx_hal.h"
#include  "stdio.h"

extern UART_HandleTypeDef huart1;
extern uint8_t aRxBuffer[64];
extern uint8_t rx_data_len;
extern volatile uint8_t rx_done;

int fputc(int ch, FILE *f)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == &huart1)
    {
        if(aRxBuffer[rx_data_len] == '\n')
        {
            rx_done = 1;
        }
        else
        {
            if(rx_data_len < 63)
            {
                rx_data_len++;
            }
        }
        HAL_UART_Receive_IT(&huart1, aRxBuffer + rx_data_len, 1);
    }
}
