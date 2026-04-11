/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint32_t delay_ms = 500;
volatile uint8_t rx_done = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
extern void run_all_tests(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */



/**
  * @brief 根据音调让蜂鸣器发出声音
  * @param 音调（频率HZ)tone,音量volumeLevel（1到10）
  * @note  
  * @retval None
  */	
void BeepPlay(int tone,int volumeLevel)   
{
//计算新频率对应的计数值，需要时钟为72MHZ，分频为3600
		int x1=0;
		if(tone==0)
			x1=1;
		else
			x1=20000/tone;
    //设置自动重装值
    __HAL_TIM_SET_AUTORELOAD(&htim4,x1); //音调
    //调整音量
	//int y1=(int)(x1>>volumeLevel);
    __HAL_TIM_SET_COMPARE(&htim4,TIM_CHANNEL_2,x1/2); //控制占空比，调整音量
    //在不使用缓冲的情况下，必须把计数值清零
    __HAL_TIM_SET_COUNTER(&htim4,0);
  
}

int notelist[]={523,587,659,698,784,880,988};
void playnote(int note,int beat)
{
	BeepPlay(notelist[note-1],2);
	HAL_Delay((int)(300*beat));
	
	BeepPlay(0,2);
	HAL_Delay(100);
}

float mymusic[][2]={
	{3,1},{3,0.5},{3,0.5},{4,1},{5,1},
	{3,1},{2,1},{2,2},
	{1,1},{1,0.5},{1,0.5},{2,1},{3,1},
	{3,1},{7,1},{7,2},
	{6,1},{3,1},{2,2},
	{6,1},{3,1},{2,2},
	{6,1},{3,1},{2,1},{1,1},
	{1,4},
};

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RTC_Init();
  MX_TIM6_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
  HAL_UART_Receive_IT(&huart1, aRxBuffer, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	//uint8_t key0_state, key1_state;
	//uint8_t key0_last = 1, key1_last = 1;
	
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOE,GPIO_PIN_5,GPIO_PIN_SET);
	
   HAL_TIM_Base_Start_IT(&htim6);
	 HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_2);
	 HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_2);
	 
	 BeepPlay(0,6);
	 int i=0;
	
	// 运行硬件综合测试
	run_all_tests();

	while (1)
  {
		i++;
		HAL_GPIO_TogglePin(GPIOE,GPIO_PIN_5);
		printf("i=%d\n\r",i);

		// FIFO模式：读指针追赶写指针
		int avail = uart_fifo_available();
		if(avail > 0)
		{
			uint8_t buf[256];
			int len = uart_fifo_read_all(buf, sizeof(buf));
			if(len > 0)
			{
				printf("recv len=%d: ", len);
				for(int k=0; k<len; k++)
				{
					printf("%02X ", buf[k]);
				}
				printf("\n\r");
			}
		}

		HAL_Delay(1000);
    
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
extern volatile uint8_t key1_pressed;
extern volatile uint8_t key2_pressed;



void HAL_RTCEx_RTCEventCallback(RTC_HandleTypeDef *hrtc)
{
	//HAL_GPIO_TogglePin(GPIOE,GPIO_PIN_5);
	
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
	//HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_5);
}

int pwmVal=0;
int myflag=1;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	int mypwmduty_max=10000;
	if(htim==&htim6)
	{
		//HAL_GPIO_TogglePin(GPIOE,GPIO_PIN_5);
		
		if(myflag==1)
		{
			pwmVal=pwmVal+264;
	
			if(pwmVal>=mypwmduty_max)
			{
				myflag=0;
				pwmVal=mypwmduty_max;
			}
			
				__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, pwmVal);    //�޸ıȽ�ֵ���޸�ռ�ձ�
			
		}
		
		
		if(myflag==0)
		{
			pwmVal=pwmVal-264;
		
			if(pwmVal<=0)
			{
				myflag=1;
				pwmVal=0;
			}
			__HAL_TIM_SetCompare(&htim3, TIM_CHANNEL_2, pwmVal);    //�޸ıȽ�ֵ���޸�ռ�ձ�
			
		}
		
	}
}



/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
