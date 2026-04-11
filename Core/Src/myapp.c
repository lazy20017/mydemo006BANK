#include "stm32f1xx_hal.h"
#include  "stdio.h"
#include "tim.h"
#include "usart.h"
#include "myapp.h"

extern UART_HandleTypeDef huart1;
extern uint8_t aRxBuffer[1];

/* ================== 按键状态标志（定义） ================== */
volatile uint8_t key1_pressed = 0;   // Key1 中断标志
volatile uint8_t key2_pressed = 0;   // Key2 中断标志
volatile uint32_t key1_last_tick = 0; // Key1 上次触发时间
volatile uint32_t key2_last_tick = 0; // Key2 上次触发时间

/* ================== UART FIFO接收缓冲区 ================== */
static uint8_t uart_rx_fifo[UART_RX_FIFO_SIZE];
static volatile uint16_t uart_rx_write = 0;   // 写指针（中断中更新）
static volatile uint16_t uart_rx_read = 0;    // 读指针（主循环中更新）

/* 获取FIFO中可用字节数 */
uint16_t uart_fifo_available(void)
{
    if(uart_rx_write >= uart_rx_read)
        return uart_rx_write - uart_rx_read;
    else
        return UART_RX_FIFO_SIZE - uart_rx_read + uart_rx_write;
}

/* 从FIFO读取一个字节，返回0表示空 */
uint8_t uart_fifo_read(uint8_t *byte)
{
    if(uart_rx_read == uart_rx_write)
        return 0;
    *byte = uart_rx_fifo[uart_rx_read];
    uart_rx_read = (uart_rx_read + 1) % UART_RX_FIFO_SIZE;
    return 1;
}

/* 读取所有可用数据，返回读取的字节数 */
int uart_fifo_read_all(uint8_t *buf, int max_len)
{
    int count = 0;
    while(uart_rx_read != uart_rx_write && count < max_len - 1)
    {
        buf[count++] = uart_rx_fifo[uart_rx_read];
        uart_rx_read = (uart_rx_read + 1) % UART_RX_FIFO_SIZE;
    }
    buf[count] = '\0';
    return count;
}

/* ================== 测试函数 ================== */

// 有源蜂鸣器测试 (PB8) + LED1同步闪烁 (PB5)
void test_active_buzzer(void)
{
    printf("\r\n=== Active Buzzer + LED1 Test ===\r\n");
    printf("Playing Morse: SOS (...---...)\r\n");
    printf("LED1(PB5) blinks with buzzer\r\n");

    // S: ... (3 dots)
    for(int i=0; i<3; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);   // 蜂鸣器ON
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);  // LED1亮（低电平）
        HAL_Delay(200);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET); // 蜂鸣器OFF
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);    // LED1灭
        HAL_Delay(200);
    }
    HAL_Delay(300);  // Letter gap

    // O: --- (3 dashes)
    for(int i=0; i<3; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
        HAL_Delay(600);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(200);
    }
    HAL_Delay(300);

    // S: ...
    for(int i=0; i<3; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
        HAL_Delay(200);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(200);
    }

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);  // 确保LED1熄灭
    printf("Done\r\n");
}

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
    printf("\r\n=== Key Test (Interrupt  5 seconds) ===\r\n");
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

// LED1测试 (PB5) - GPIO输出闪烁
void test_led_pwm(uint32_t frequency, uint32_t duration_sec)
{
    printf("\r\n=== LED1(PB5) GPIO Blink Test ===\r\n");
    printf("Freq: %d Hz, Duration: %d sec\r\n", frequency, duration_sec);

    uint32_t interval_ms = 1000 / (frequency * 2);  // 闪烁间隔

    uint32_t start_tick = HAL_GetTick();
    while(HAL_GetTick() - start_tick < duration_sec * 1000) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);  // PB5闪烁
        HAL_Delay(interval_ms);
    }

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);  // 确保熄灭
    printf("LED1 PWM Done\r\n");
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
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);  // 确保熄灭
    printf("LED2 Done\r\n");
}

// LED1与蜂鸣器同步闪烁测试
void test_led1_sync(void)
{
    printf("\r\n=== LED1 + Buzzer Sync Test ===\r\n");
    printf("LED1(PB5) blinks with buzzer (PB8)\r\n");
    printf("Playing: Letter A (. -)\r\n");

    // A: .- (dot then dash)
    // Dot (.)
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);   // 蜂鸣器ON
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);  // LED1亮
    HAL_Delay(200);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET); // 蜂鸣器OFF
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);    // LED1灭
    HAL_Delay(200);

    // Dash (-)
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_Delay(600);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_Delay(200);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);  // 确保LED1熄灭
    printf("Done\r\n");
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
    HAL_Delay(500);

    // 8. LED1+蜂鸣器同步测试
    test_led1_sync();

    printf("########################################\r\n");
    printf("######## Hardware Test Complete ########\r\n");
    printf("########################################\r\n\r\n");
}

int fputc(int ch, FILE *f)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}

/* ================== UART FIFO接收回调 ================== */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == &huart1)
    {
        uart_rx_fifo[uart_rx_write] = aRxBuffer[0];
        uart_rx_write = (uart_rx_write + 1) % UART_RX_FIFO_SIZE;
        HAL_UART_Receive_IT(&huart1, &aRxBuffer[0], 1);
    }
}

/* ================== 摩斯密码作业函数 ================== */

/* 摩斯码表：点=1，划=2，0=结束 */
const uint8_t morse_code[26][6] = {
    /* A */ {1,2,0},       /* B */ {2,1,1,1,0},   /* C */ {2,1,2,1,0},
    /* D */ {2,1,1,0},     /* E */ {1,0},         /* F */ {1,1,2,1,0},
    /* G */ {2,2,1,0},     /* H */ {1,1,1,1,0},   /* I */ {1,1,0},
    /* J */ {1,2,2,2,0},   /* K */ {2,1,2,0},     /* L */ {1,2,1,1,0},
    /* M */ {2,2,0},       /* N */ {2,1,0},       /* O */ {2,2,2,0},
    /* P */ {1,2,2,1,0},   /* Q */ {2,2,1,2,0},   /* R */ {1,2,1,0},
    /* S */ {1,1,1,0},     /* T */ {2,0},         /* U */ {1,1,2,0},
    /* V */ {1,1,1,2,0},   /* W */ {1,2,2,0},     /* X */ {2,1,1,2,0},
    /* Y */ {2,1,2,2,0},   /* Z */ {2,2,1,1,0}
};

/* 游戏状态枚举 */
typedef enum {
    GAME_IDLE,         /* 待机状态，等待启动 */
    GAME_PLAYING,      /* 播放摩斯码中 */
    GAME_WAIT_ANSWER   /* 等待玩家输入答案 */
} GameState_TypeDef;

/* 游戏状态结构体 */
typedef struct {
    GameState_TypeDef state;
    uint8_t current_char;      /* 当前题目字母(0-25) */
    uint8_t play_count;        /* 已播放次数(1-3) */
    uint8_t guess_count;       /* 已猜测次数(0-3) */
    uint8_t score;             /* 当前得分 */
    uint8_t is_busy;           /* 播放中标志 */
} GameData_TypeDef;

static GameData_TypeDef game;

/* 有源蜂鸣器发声控制 */
static void active_beep_on(void) { 
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET); 
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);  // LED1同步亮（低电平亮）
}
static void active_beep_off(void) { 
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET); 
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);    // LED1同步灭
}

/* LED2控制 - 状态灯（前置声明） */
static void led2_on(void);
static void led2_off(void);
static void led2_blink(void);
static void led2_quick_blink(void);

/* 播放一个摩斯码元素：点200ms，划600ms */
static void play_morse_element(uint8_t element)
{
    if(element == 1) {
        active_beep_on();
        HAL_Delay(200);
        active_beep_off();
        HAL_Delay(200);  /* 元素间隙 */
    } else if(element == 2) {
        active_beep_on();
        HAL_Delay(600);
        active_beep_off();
        HAL_Delay(200);
    }
}

/* 播放一个字母的摩斯码 */
static void play_morse_letter(uint8_t letter)
{
    uint8_t code;
    for(int i=0; i<6; i++) {
        code = morse_code[letter][i];
        if(code == 0) break;
        play_morse_element(code);
    }
    HAL_Delay(300);  /* 字母间隔 */
}

/* 将摩斯码转换为01字符串（点=0，划=1） */
static void morse_to_binstr(uint8_t letter, char *buf)
{
    uint8_t code;
    int j = 0;
    for(int i=0; i<6; i++) {
        code = morse_code[letter][i];
        if(code == 0) break;
        buf[j++] = (code == 1) ? '0' : '1';
    }
    buf[j] = '\0';
}

/* 播放当前题目的摩斯码 */
void morse_play_current(void)
{
    if(game.state != GAME_IDLE && game.state != GAME_WAIT_ANSWER) return;
    if(game.play_count > 3) return;

    game.state = GAME_PLAYING;
    game.is_busy = 1;

    char binstr[8];
    morse_to_binstr(game.current_char, binstr);
    printf("\r\n[Game] Morse: %s  (0=dot  1=dash)  [Letter: %c]\r\n", binstr, 'A' + game.current_char);

    play_morse_letter(game.current_char);
    game.is_busy = 0;

    if(game.state == GAME_PLAYING)  /* 未被Key2打断 */
        game.state = GAME_WAIT_ANSWER;

    led2_on();  // 等待输入时LED2亮起
    printf("[Game] Play #%d, please input answer via serial port\r\n", game.play_count);
}

/* 初始化新游戏/新题目 */
void morse_new_question(void)
{
    game.state = GAME_IDLE;
    game.current_char = HAL_GetTick() % 26;  /* 伪随机 */
    game.play_count = 0;
    game.guess_count = 0;
    led2_off();  // 新题目时LED2熄灭
    /* score保持不变 */
}

/* 初始化游戏 */
void morse_game_init(void)
{
    game.state = GAME_IDLE;
    game.score = 0;
    morse_new_question();
    printf("\r\n=== Morse Training Game Ready ===\r\n");
    printf("Press KEY1 to start, KEY2 to reset\r\n");
    printf("Current score: %d\r\n", game.score);
}

/* 检查答案是否正确 */
static uint8_t check_answer(uint8_t answer_char)
{
    return (answer_char >= 'a' && answer_char <= 'z') ?
           (answer_char - 'a') : (answer_char >= 'A' && answer_char <= 'Z') ?
           (answer_char - 'A') : 26;
}

/* 播放成功乐谱 */
static void play_success_melody(void)
{
    int notes[] = {523, 659, 784, 1046};
    int durations[] = {80, 80, 80, 150};
    for(int i=0; i<4; i++) {
        BeepPlay(notes[i], 2);
        HAL_Delay(durations[i]);
        BeepPlay(0, 2);
        HAL_Delay(50);
    }
}

/* 播放错误乐谱 */
static void play_error_melody(void)
{
    int notes[] = {370, 349, 196};
    int durations[] = {150, 300, 400};
    for(int i=0; i<3; i++) {
        BeepPlay(notes[i], 2);
        HAL_Delay(durations[i]);
        BeepPlay(0, 2);
        HAL_Delay(50);
    }
}

/* LED2控制 - 状态灯 */
static void led2_on(void) { 
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_RESET);   // 低电平亮
}
static void led2_off(void) { 
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET);    // 高电平灭
}

/* LED2闪烁（错误时/答案公布时）- 更明显的闪烁效果 */
static void led2_blink(void)
{
    for(int i=0; i<5; i++) {
        HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_5);
        HAL_Delay(150);
        HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_5);
        HAL_Delay(150);
    }
}

/* LED2快速闪烁（正确答案时）*/
static void led2_quick_blink(void)
{
    for(int i=0; i<3; i++) {
        HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_5);
        HAL_Delay(100);
        HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_5);
        HAL_Delay(100);
    }
}

/* 正确答案处理 */
static void handle_correct(void)
{
    game.score += 10;
    play_success_melody();
    led2_quick_blink();  // 正确答案时LED2快速闪烁
    led2_off();          // 然后熄灭
    char binstr[8];
    morse_to_binstr(game.current_char, binstr);
    printf("\r\n>>> Correct! Answer is %c  [Morse: %s]\r\n", 'A' + game.current_char, binstr);
    printf(">>> Score: %d\r\n", game.score);
    HAL_Delay(500);
    morse_new_question();
}

/* 错误答案处理 */
static void handle_wrong(void)
{
    game.guess_count++;
    play_error_melody();
    led2_blink();  // 错误时LED2闪烁
    char binstr[8];
    morse_to_binstr(game.current_char, binstr);

    if(game.guess_count >= 3) {
        printf("\r\n>>> Wrong! 3 attempts used.\r\n");
        printf(">>> Answer: %c  [Morse: %s]\r\n", 'A' + game.current_char, binstr);
        printf(">>> Returning to idle...\r\n");
        led2_off();  // 公布答案后LED2熄灭
        HAL_Delay(1000);
        morse_new_question();
    } else {
        printf("\r\n>>> Wrong! Attempts left: %d  [Morse: %s]\r\n", 3 - game.guess_count, binstr);
        led2_on();  // 还有机会时LED2重新亮起
    }
}

/* 收到串口答案字符 */
void morse_handle_input(uint8_t ch)
{
    if(game.state != GAME_WAIT_ANSWER) return;
    if(game.is_busy) return;

    uint8_t idx = check_answer(ch);
    if(idx == 26) return;  /* 无效字符 */

    printf("\r\n>>> Your answer: %c\r\n", (ch>='a' && ch<='z') ? ch-32 : ch);

    if(idx == game.current_char) {
        handle_correct();
    } else {
        handle_wrong();
    }
}

/* Key1处理（启动/重播） */
void morse_key1_action(void)
{
    if(game.state == GAME_IDLE) {
        morse_new_question();
        game.play_count = 1;
        printf("\r\n=== New Question #%d ===\r\n", game.score/10 + 1);
        HAL_Delay(100);
        morse_play_current();
    } else if(game.state == GAME_WAIT_ANSWER) {
        if(game.play_count < 3) {
            game.play_count++;
            printf("\r\n[Game] Replay #%d\r\n", game.play_count);
            morse_play_current();
        } else {
            printf("\r\n[Game] Replay limit reached (3/3)\r\n");
            printf("[Game] Please enter your answer via serial\r\n");
        }
    }
}

/* Key2处理（复位） */
void morse_key2_reset(void)
{
    active_beep_off();
    BeepPlay(0, 2);
    uart_rx_read = uart_rx_write;  /* 清空FIFO */
    morse_game_init();
    printf("\r\n>>> SYSTEM RESET\r\n");
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t now = HAL_GetTick();
    if(GPIO_Pin == GPIO_PIN_3)
    {
        // Key2 (PE3) 按下，防抖检测
        if(now - key2_last_tick > KEY_DEBOUNCE_MS) {
            key2_pressed = 1;
            key2_last_tick = now;
        }
    }
    else if(GPIO_Pin == GPIO_PIN_4)
    {
        // Key1 (PE4) 按下，防抖检测
        if(now - key1_last_tick > KEY_DEBOUNCE_MS) {
            key1_pressed = 1;
            key1_last_tick = now;
        }
    }
}
