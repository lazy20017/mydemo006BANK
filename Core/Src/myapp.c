#include "stm32f1xx_hal.h"
#include  "stdio.h"
#include "tim.h"

extern UART_HandleTypeDef huart1;
extern uint8_t aRxBuffer[64];
extern uint8_t rx_data_len;
extern volatile uint8_t rx_done;

/* ================== 按键状态标志 ================== */
volatile uint8_t key1_pressed = 0;  // Key1 中断标志
volatile uint8_t key2_pressed = 0;  // Key2 中断标志

// 按键中断回调函数声明 (在 main.c 中实现)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

/* ================== 测试函数 ================== */

// 有源蜂鸣器测试 (PB8) - 播放摩斯码字符
void test_active_buzzer(void)
{
    printf("\r\n=== Active Buzzer Test ===\r\n");
    printf("Playing Morse: SOS (...---...)\r\n");

    // S: ... (3 dots)
    for(int i=0; i<3; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);  // ON
        HAL_Delay(200);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET); // OFF
        HAL_Delay(200);
    }
    HAL_Delay(300);  // Letter gap

    // O: --- (3 dashes)
    for(int i=0; i<3; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
        HAL_Delay(600);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_Delay(200);
    }
    HAL_Delay(300);

    // S: ...
    for(int i=0; i<3; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
        HAL_Delay(200);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_Delay(200);
    }

    printf("Done\r\n");
}

// 蜂鸣器发声基础函数
void BeepPlay(int tone, int volumeLevel);

// 无源蜂鸣器正确乐谱 (1-3-5-i)
void test_passive_buzzer_success(void)
{
    printf("\r\n=== Passive Buzzer - Success ===\r\n");
    printf("Playing: Success (523+659+784+1046Hz)\r\n");

    int notes[] = {523, 659, 784, 1046};
    int durations[] = {80, 80, 80, 150};

    for(int i=0; i<4; i++) {
        BeepPlay(notes[i], 2);
        HAL_Delay(durations[i]);
        BeepPlay(0, 2);  // Stop
        HAL_Delay(50);
    }
    printf("Done\r\n");
}

// 无源蜂鸣器错误乐谱 (#4-4-5.)
void test_passive_buzzer_error(void)
{
    printf("\r\n=== Passive Buzzer - Error ===\r\n");
    printf("Playing: Error (370+349+196Hz)\r\n");

    int notes[] = {370, 349, 196};
    int durations[] = {150, 300, 400};

    for(int i=0; i<3; i++) {
        BeepPlay(notes[i], 2);
        HAL_Delay(durations[i]);
        BeepPlay(0, 2);
        HAL_Delay(50);
    }
    printf("Done\r\n");
}

// 按键测试 (使用中断标志)
void test_keys(void)
{
    printf("\r\n=== Key Test (Interrupt) ===\r\n");
    printf("Press keys to see output\r\n");
    printf("Key1=Start/Replay, Key2=Reset\r\n");

    key1_pressed = 0;
    key2_pressed = 0;

    uint32_t start_tick = HAL_GetTick();
    while(HAL_GetTick() - start_tick < 5000) {
        if(key1_pressed) {
            printf(">>> Key1(PE4) Pressed!\r\n");
            key1_pressed = 0;
        }
        if(key2_pressed) {
            printf(">>> Key2(PE3) Pressed!\r\n");
            key2_pressed = 0;
        }
        HAL_Delay(100);
    }
    printf("Key Test Done\r\n");
}

// LED测试 - 使用TIM3_CH2 (PB5)
void test_led_pwm(uint32_t frequency, uint32_t duration_sec)
{
    printf("\r\n=== LED PWM Test ===\r\n");
    printf("Freq: %d Hz, Duration: %d sec\r\n", frequency, duration_sec);

    uint32_t period_ticks = 10000;  // PWM周期
    uint32_t half_period_ms = 1000 / (frequency * 2);  // 半周期(高低切换)

    uint32_t start_tick = HAL_GetTick();
    uint8_t led_state = 0;

    while(HAL_GetTick() - start_tick < duration_sec * 1000) {
        // 通过改变PWM比较值实现LED闪烁
        if(led_state == 0) {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);  // 暗
        } else {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, period_ticks/2);  // 亮
        }
        led_state = !led_state;

        uint32_t tick_before = HAL_GetTick();
        while(HAL_GetTick() - tick_before < half_period_ms) {
            // 等待
        }
    }

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
    printf("LED PWM Done\r\n");
}

// LED2测试 (PE5) - GPIO输出
void test_led2(void)
{
    printf("\r\n=== LED2 Test ===\r\n");
    printf("LED2(PE5) Blink 3 sec\r\n");

    uint32_t start_tick = HAL_GetTick();
    while(HAL_GetTick() - start_tick < 3000) {
        HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_5);
        HAL_Delay(500);
    }
    printf("LED2 Done\r\n");
}

// 综合测试入口
void run_all_tests(void)
{
    printf("\r\n\r\n########################################\r\n");
    printf("######## Hardware Test Start ########\r\n");
    printf("########################################\r\n\r\n");

    // 1. 有源蜂鸣器测试
    test_active_buzzer();
    HAL_Delay(1000);

    // 2. 无源蜂鸣器 - 正确乐谱
    test_passive_buzzer_success();
    HAL_Delay(1000);

    // 3. 无源蜂鸣器 - 错误乐谱
    test_passive_buzzer_error();
    HAL_Delay(1000);

    // 4. 按键测试
    test_keys();

    // 5. LED1 PWM测试 - 1Hz (3秒)
    test_led_pwm(1, 3);

    // 6. LED1 PWM测试 - 4Hz (3秒)
    test_led_pwm(4, 3);

    // 7. LED2测试
    test_led2();

    printf("########################################\r\n");
    printf("######## Hardware Test Complete ########\r\n");
    printf("########################################\r\n\r\n");
}

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
