# STM32F103 RTC 开发指南：LSI 时钟源配置方案

## 一、概述

本文档描述如何在 STM32F103 微控制器上使用内部低速振荡器（LSI，40kHz）作为 RTC 时钟源，适用于没有外部 32.768kHz 晶振（LSE）的硬件设计。

### 1.1 问题背景

默认的 STM32CubeMX 配置通常使用 LSE（外部低速晶振，32.768kHz）作为 RTC 时钟源。如果硬件没有焊接 LSE 晶振，RTC 初始化会失败，导致程序卡死在 `Error_Handler()` 死循环中。

### 1.2 解决方案

使用 STM32 内部低速振荡器（LSI，典型频率 40kHz）替代 LSE 作为 RTC 时钟源，无需外部晶振。

---

## 二、硬件与软件环境

| 项目 | 说明 |
|------|------|
| MCU | STM32F103xE（或其他 STM32F1 系列） |
| 主频 | 72MHz（HSE = 8MHz × PLL_MUL9） |
| RTC 时钟源 | LSI（内部 40kHz 振荡器） |
| 开发工具 | Keil MDK-ARM |
| 固件库 | STM32CubeF1 HAL |

---

## 三、时钟配置详解

### 3.1 系统时钟配置（main.c）

```c
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    // 1. 配置振荡器 - 关闭 LSE，只启用 HSE 和 HSI
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.LSEState = RCC_LSE_OFF;           // 关键：关闭 LSE
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    // 2. 配置总线时钟
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;   // TIM2-7, I2C1-2, SPI2-3, USART2-5 使用 36MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;  // SPI1, USART1 等使用 72MHz

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }

    // 3. 配置 RTC 外设时钟 - 关键：使用 LSI
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;  // 关键：LSI 作为 RTC 时钟源
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }
}
```

### 3.2 关键配置点说明

| 配置项 | 值 | 说明 |
|--------|-----|------|
| `RCC_OscInitStruct.LSEState` | `RCC_LSE_OFF` | 禁用外部晶振，减少启动等待时间 |
| `PeriphClkInit.RTCClockSelection` | `RCC_RTCCLKSOURCE_LSI` | 指定 LSI 为 RTC 时钟源 |
| `RCC_APB1CLKDivider` | `RCC_HCLK_DIV2` | APB1 总线最大 36MHz，用于 TIM3/TIM4 PWM |

---

## 四、RTC 配置详解（rtc.c）

### 4.1 完整代码

```c
/* 头文件 */
#include "rtc.h"

/* RTC 句柄（外部声明在 rtc.h 中） */
RTC_HandleTypeDef hrtc;

/**
 * @brief  RTC 初始化函数
 */
void MX_RTC_Init(void)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef DateToUpdate = {0};

    /* 初始化 RTC 外设 */
    hrtc.Instance = RTC;
    hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;    // 自动计算预分频，实现 1 秒基准
    hrtc.Init.OutPut = RTC_OUTPUTSOURCE_ALARM;      // 输出闹钟信号（可选）
    
    if (HAL_RTC_Init(&hrtc) != HAL_OK)
    {
        Error_Handler();
    }

    /* 设置初始时间 */
    sTime.Hours = 11;
    sTime.Minutes = 0;
    sTime.Seconds = 0;
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }

    /* 设置初始日期 */
    DateToUpdate.WeekDay = RTC_WEEKDAY_MONDAY;
    DateToUpdate.Month = RTC_MONTH_MARCH;
    DateToUpdate.Date = 23;
    DateToUpdate.Year = 26;
    if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief  RTC MSP 初始化 - 在 HAL_RTC_Init() 内部被调用
 * @note   此函数在 RTC_Init() 之后自动执行
 * @param  rtcHandle: RTC 句柄指针
 */
void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{
    if (rtcHandle->Instance == RTC)
    {
        /* 使能备份域访问 */
        HAL_PWR_EnableBkUpAccess();
        
        /* 使能备份域时钟（必须先使能才能访问 RTC/BKP 寄存器） */
        __HAL_RCC_BKP_CLK_ENABLE();
        
        /* 使能 RTC 时钟 */
        __HAL_RCC_RTC_ENABLE();

        /* ========== 关键：使能 LSI ========== */
        __HAL_RCC_LSI_ENABLE();                           // 开启 LSI 振荡器
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == RESET);  // 等待 LSI 就绪

        /* 配置 RTC 中断（可选，用于闹钟/秒中断） */
        HAL_NVIC_SetPriority(RTC_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(RTC_IRQn);
        HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

        /* 配置秒中断（每秒触发一次） */
        if (HAL_RTCEx_SetSecond_IT(&hrtc) != HAL_OK)
        {
            Error_Handler();
        }
    }
}

/**
 * @brief  RTC MSP 反初始化
 */
void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{
    if (rtcHandle->Instance == RTC)
    {
        /* 关闭 RTC 时钟 */
        __HAL_RCC_RTC_DISABLE();
        
        /* 关闭中断 */
        HAL_NVIC_DisableIRQ(RTC_IRQn);
        HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn);
    }
}
```

### 4.2 LSI 配置要点

```
LSI 使能流程：
┌─────────────────────────────────────┐
│ 1. __HAL_RCC_LSI_ENABLE()           │  开启 LSI 振荡器
├─────────────────────────────────────┤
│ 2. while (!LSIRDY)                  │  等待内部就绪标志（轮询方式）
├─────────────────────────────────────┤
│ 3. RTC 时钟自动切换到 LSI            │  HAL 库自动处理
├─────────────────────────────────────┤
│ 4. 可选：__HAL_RCC_RTC_RAPID_START() │  快速启动模式（可选）
└─────────────────────────────────────┘
```

### 4.3 LSI vs LSE 对比

| 特性 | LSI | LSE |
|------|-----|-----|
| 频率 | ~40kHz（典型值，有个体差异） | 32.768kHz（精确） |
| 精度 | ±10%（温漂大） | ±20ppm（精确） |
| 外部器件 | 不需要 | 需要 32.768kHz 晶振 |
| 功耗 | 较低 | 极低 |
| 启动时间 | ~200μs | ~2秒 |
| 适用场景 | 秒级精度足够 | 需要精确日历/闹钟 |

---

## 五、主函数初始化顺序

```c
int main(void)
{
    /* 1. HAL 库初始化 */
    HAL_Init();

    /* 2. 系统时钟配置（HSE + PLL = 72MHz） */
    SystemClock_Config();

    /* 3. 初始化 GPIO */
    MX_GPIO_Init();

    /* 4. 初始化 RTC（会触发 HAL_RTC_MspInit） */
    MX_RTC_Init();

    /* 5. 初始化其他外设 */
    MX_TIM6_Init();    // 基础定时器（可选）
    MX_TIM3_Init();    // PWM 输出（蜂鸣器）
    MX_TIM4_Init();    // PWM 输出（蜂鸣器）
    MX_USART1_UART_Init();  // 串口通信

    /* 6. 用户初始化代码 */
    FIFO_Init(&rx_fifo);
    Game_Init(&game_ctx);
    srand(HAL_GetTick());
    HAL_UART_Receive_IT(&huart1, &rx_data, 1);

    /* 7. 启动 PWM */
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);

    /* 8. 主循环 */
    while (1)
    {
        /* 应用逻辑 */
    }
}
```

---

## 六、RTC 相关 HAL 库函数

### 6.1 常用函数

| 函数 | 说明 |
|------|------|
| `HAL_RTC_Init()` | 初始化 RTC |
| `HAL_RTC_SetTime()` | 设置时间 |
| `HAL_RTC_SetDate()` | 设置日期 |
| `HAL_RTC_GetTime()` | 读取时间 |
| `HAL_RTC_GetDate()` | 读取日期 |
| `HAL_RTCEx_SetSecond_IT()` | 使能秒中断 |
| `HAL_RTC_SetAlarm_IT()` | 设置闹钟中断 |

### 6.2 LSI 状态检查

```c
// 检查 LSI 是否就绪
if (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == SET)
{
    // LSI 已就绪
}

// 获取 RTC 当前时钟源
switch (__HAL_RCC_GET_RTC_SOURCE())
{
    case RCC_RTCCLKSOURCE_LSE: /* LSE */ break;
    case RCC_RTCCLKSOURCE_LSI: /* LSI */ break;
    case RCC_RTCCLKSOURCE_HSE_DIV128: /* HSE/128 */ break;
}
```

---

## 七、RTC 秒中断使用示例

### 7.1 中断服务函数（stm32f1xx_it.c）

```c
void RTC_IRQHandler(void)
{
    HAL_RTCEx_RTCIRQHandler(&hrtc);
}

void HAL_RTCEx_RTCIRQHandler(RTC_HandleTypeDef *hrtc)
{
    if (__HAL_RTC_SECOND_GET_IT_SOURCE(hrtc))
    {
        if (__HAL_RTC_SECOND_GET_FLAG(hrtc))
        {
            /* 清除秒中断标志 */
            __HAL_RTC_SECOND_CLEAR_FLAG(hrtc);
            
            /* 用户代码：每秒执行一次 */
            // 例如：更新显示、计数等
        }
    }
}
```

### 7.2 秒中断回调

```c
void HAL_RTCEx_RTCSecondEventCallback(RTC_HandleTypeDef *hrtc)
{
    /* 每次秒中断自动调用 */
    // 适合用于定时任务、心跳计数等
}
```

---

## 八、常见问题排查

### 8.1 问题：程序卡死在 RTC_Init()

**可能原因：**
- LSE 仍处于开启状态且晶振未就绪
- 备份域被写保护

**解决方案：**
1. 确认 `SystemClock_Config()` 中 `LSEState = RCC_LSE_OFF`
2. 在 `HAL_RTC_MspInit()` 中添加 `__HAL_RCC_LSI_ENABLE()` 并等待就绪

### 8.2 问题：RTC 时间不准

**可能原因：**
- LSI 频率偏差大（典型 ±10%）
- 预分频配置不正确

**解决方案：**
- 使用校准值补偿：`HAL_RTCEx_SetCalibration()`（如果芯片支持）
- 或切换到 LSE（如果精度要求高）

### 8.3 问题：RTC 掉电后时间丢失

**可能原因：**
- VBAT 引脚未连接或电压不足
- LSI 在断电后不工作（正常现象）

**解决方案：**
- 确保 VBAT 引脚连接到备用电源（如纽扣电池）
- 或在每次上电时同步时间

---

## 九、完整文件修改清单

### 9.1 需要修改的文件

| 文件 | 修改内容 |
|------|----------|
| `Core/Src/main.c` | `SystemClock_Config()`：关闭 LSE，设置 RTC 时钟源为 LSI |
| `Core/Src/rtc.c` | `HAL_RTC_MspInit()`：添加 LSI 使能和等待代码 |

### 9.2 配置检查清单

```
□ SystemClock_Config() 中 LSEState = RCC_LSE_OFF
□ SystemClock_Config() 中 RTCClockSelection = RCC_RTCCLKSOURCE_LSI
□ HAL_RTC_MspInit() 中有 __HAL_RCC_LSI_ENABLE()
□ HAL_RTC_MspInit() 中有 LSIRDY 就绪等待循环
□ 备份域时钟 BKP_CLK 已使能
□ RTC_CLK 已使能
```

---

## 十、参考寄存器

### 10.1 RCC 相关寄存器

| 寄存器 | 位 | 说明 |
|--------|-----|------|
| `RCC->CSR` | `RCC_CSR_LSION` | LSI 开启 |
| `RCC->CSR` | `RCC_CSR_LSIRDY` | LSI 就绪标志 |
| `RCC->BDCR` | `RCC_BDCR_RTCSEL` | RTC 时钟源选择 |

### 10.2 LSI 特性参数

| 参数 | 值 |
|------|-----|
| 标称频率 | 40kHz |
| 精度 | ±10%（受工艺和温度影响） |
| 启动时间 | ~200μs |
| 功耗 | ~1μA |

---

## 十一、版本信息

| 项目 | 内容 |
|------|------|
| 文档版本 | v1.0 |
| 创建日期 | 2026-04-11 |
| 适用芯片 | STM32F1 系列 |
| 验证状态 | 已在实际项目验证 |

---

*本文档可作为 STM32F1 系列使用 LSI 作为 RTC 时钟源的标准开发参考。*
