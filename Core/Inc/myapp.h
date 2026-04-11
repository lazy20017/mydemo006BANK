#ifndef __MYAPP_H
#define __MYAPP_H

#include "stm32f1xx_hal.h"

/* ================== 按键相关 ================== */
extern volatile uint8_t key1_pressed;   // Key1 中断标志
extern volatile uint8_t key2_pressed;   // Key2 中断标志
extern volatile uint32_t key1_last_tick; // Key1 上次触发时间
extern volatile uint32_t key2_last_tick; // Key2 上次触发时间

#define KEY_DEBOUNCE_MS  500  // 防抖时间（毫秒）

/* ================== UART FIFO 接收缓冲区 ================== */
#define UART_RX_FIFO_SIZE 256

uint16_t uart_fifo_available(void);                                    // 获取FIFO中可用字节数
uint8_t uart_fifo_read(uint8_t *byte);                                 // 从FIFO读取一个字节，返回0表示空
int uart_fifo_read_all(uint8_t *buf, int max_len);                     // 读取所有可用数据，返回读取的字节数

/* ================== 测试函数 ================== */
void test_active_buzzer(void);                                        // 有源蜂鸣器测试 - 播放摩斯码SOS
void test_passive_buzzer_success(void);                               // 无源蜂鸣器正确乐谱 (1-3-5-i)
void test_passive_buzzer_error(void);                                  // 无源蜂鸣器错误乐谱 (#4-4-5.)
void test_keys(void);                                                 // 按键测试（中断5秒）
void test_led_pwm(uint32_t frequency, uint32_t duration_sec);          // LED PWM测试
void test_led2(void);                                                 // LED2测试 (PE5)
void run_all_tests(void);                                              // 综合测试入口

/* ================== 蜂鸣器基础函数（定义在main.c中）==================== */
void BeepPlay(int tone, int volumeLevel);                              // 蜂鸣器发声基础函数

/* ================== 摩斯密码游戏函数 ================== */
void morse_game_init(void);            // 初始化游戏
void morse_play_current(void);         // 播放当前题目的摩斯码
void morse_new_question(void);         // 初始化新游戏/新题目
void morse_handle_input(uint8_t ch);   // 收到串口答案字符处理
void morse_key1_action(void);          // Key1处理（启动/重播）
void morse_key2_reset(void);           // Key2处理（复位）

#endif
