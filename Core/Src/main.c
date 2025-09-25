/* USER CODE BEGIN Header */
/**
  ******************************************************************************
 * @file    main.c
 * @author  MCD Application Team
 * @brief   BLE application with BLE core
 *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  @verbatim
  ==============================================================================
                    ##### IMPORTANT NOTE #####
  ==============================================================================

  This application requests having the stm32wb5x_BLE_Stack_fw.bin binary
  flashed on the Wireless Coprocessor.
  If it is not the case, you need to use STM32CubeProgrammer to load the appropriate
  binary.

  All available binaries are located under following directory:
  /Projects/STM32_Copro_Wireless_Binaries

  Refer to UM2237 to learn how to use/install STM32CubeProgrammer.
  Refer to /Projects/STM32_Copro_Wireless_Binaries/ReleaseNote.html for the
  detailed procedure to change the Wireless Coprocessor binary.

  @endverbatim
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/

/* USER CODE END PD */

/* Clock Configuration Selection */
#define CLOCK_CONFIG_HSE_LSE_ONLY 0 // 外部高速 + 外部低速: HSE + LSE
#define CLOCK_CONFIG_HSI_LSE_ONLY 1 // 内部高速 + 外部低速: HSI + LSE
#if defined(STM32WB15xx) || defined(STM32WB10xx)
  // 可支持 LSI
#define CLOCK_CONFIG_HSE_LSI_ONLY 0 // 外部高速 + 内部低速: HSE + LSI
#define CLOCK_CONFIG_HSI_LSI_ONLY 0 // 内部高速 + 内部低速: HSI + LSI
#define RF_WAKEUP_CLK_LSI1 1 // 选择 LSI1 作为 RF 唤醒时钟
#else
#define CLOCK_CONFIG_HSE_LSI_ONLY 0 // 外部高速 + 内部低速: HSE + LSI
#define CLOCK_CONFIG_HSI_LSI_ONLY 0 // 内部高速 + 内部低速: HSI + LSI
#endif
#define CLOCK_CONFIG_HSI_INCLUDED 0 // 启用HSI48 (USB/RNG) 实验模式
#define CLOCK_CONFIG_USE_HSE_PLL 0  // HSE 经 PLL 作为 SYSCLK
#define CLOCK_CONFIG_USE_HSI_PLL 1  // HSI 经 PLL 作为 SYSCLK




/* ============ 低速时钟 (LSE / LSI) 选择宏 ============
 * 规则: 仅允许 LSE 或 LSI 一种低速源被选用
 */
#if ((CLOCK_CONFIG_HSE_LSE_ONLY)+(CLOCK_CONFIG_HSI_LSE_ONLY)) && ((CLOCK_CONFIG_HSE_LSI_ONLY)+(CLOCK_CONFIG_HSI_LSI_ONLY))
#error "不可同时选择 LSE 与 LSI 组合"
#endif

/* ================= 编译期配置一致性检查 ================= */
#if ((CLOCK_CONFIG_HSE_LSE_ONLY)+(CLOCK_CONFIG_HSI_LSE_ONLY)+(CLOCK_CONFIG_HSI_INCLUDED)+(CLOCK_CONFIG_HSE_LSI_ONLY)+(CLOCK_CONFIG_HSI_LSI_ONLY)) != 1
#error "必须且只允许一个主配置宏为1: HSE_LSE_ONLY / HSI_LSE_ONLY / HSI_INCLUDED / HSE_LSI_ONLY / HSI_LSI_ONLY"
#endif

#if (CLOCK_CONFIG_HSE_LSE_ONLY==1 || CLOCK_CONFIG_HSE_LSI_ONLY==1) && (CLOCK_CONFIG_USE_HSI_PLL==1)
#error "选择 HSE* 模式时不应启用 CLOCK_CONFIG_USE_HSI_PLL"
#endif
#if (CLOCK_CONFIG_HSI_LSE_ONLY==1 || CLOCK_CONFIG_HSI_LSI_ONLY==1 || CLOCK_CONFIG_HSI_INCLUDED==1) && (CLOCK_CONFIG_USE_HSE_PLL==1)
#error "选择 HSI* 模式时不应启用 CLOCK_CONFIG_USE_HSE_PLL"
#endif
#if (CLOCK_CONFIG_HSI_INCLUDED==1) && (CLOCK_CONFIG_USE_HSE_PLL==1) && (CLOCK_CONFIG_USE_HSI_PLL==1)
#error "HSI_INCLUDED 模式下不应同时打开 HSE 与 HSI 的 PLL 宏"
#endif

/* ============ 统一低速时钟宏 ============ */
#if (CLOCK_CONFIG_HSE_LSE_ONLY || CLOCK_CONFIG_HSI_LSE_ONLY || CLOCK_CONFIG_HSI_INCLUDED)
#define LOW_SPEED_IS_LSE 1
#else
#define LOW_SPEED_IS_LSE 0
#endif

#if LOW_SPEED_IS_LSE
#define LSE_STATE_CONFIG RCC_LSE_ON
#define LSI_STATE_CONFIG RCC_LSI_OFF
#define RF_WAKEUP_CLK  RCC_RFWKPCLKSOURCE_LSE
#else
#define LSE_STATE_CONFIG RCC_LSE_OFF
#define LSI_STATE_CONFIG RCC_LSI_ON
#ifdef RF_WAKEUP_CLK_LSI1
#define RF_WAKEUP_CLK  RCC_OSCILLATORTYPE_LSI1
#else
#define RF_WAKEUP_CLK  RCC_OSCILLATORTYPE_LSI2
#endif
#endif

/* ============ SMPS 时钟源选择 ============
 * 原代码固定使用 HSI 作为 SMPS 时钟，但在 HSE_LSE_ONLY 模式下已关闭 HSI，
 * 会导致 SMPS 时钟不可用 -> 无线子系统可能异常，表现为 BLE 不广播/不连接。
 * 规则:
 *  - 如果 HSE 打开且 HSI 关闭: 使用 HSE 作为 SMPS 时钟
 *  - 其余情况: 使用 HSI (因为 HSI 仍保持开启)
 */
#if (defined(HSE_STATE_CONFIG) && (HSE_STATE_CONFIG==RCC_HSE_ON) && defined(HSI_STATE_CONFIG) && (HSI_STATE_CONFIG==RCC_HSI_OFF))
#define SMPS_CLK_SRC_CONFIG RCC_SMPSCLKSOURCE_HSE
#else
#define SMPS_CLK_SRC_CONFIG RCC_SMPSCLKSOURCE_HSI
#endif

/* 根据配置选择不同的时钟设置 */
#if (CLOCK_CONFIG_HSE_LSE_ONLY == 1)
// 模式1: HSE(32MHz)+LSE(32.768kHz)
#define OSC_TYPE_CONFIG (RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE)
#define HSE_STATE_CONFIG RCC_HSE_ON
#define HSI_STATE_CONFIG RCC_HSI_OFF
#define HSI48_CONFIG_ENABLE 0
#define SYSCLK_SOURCE_CONFIG RCC_SYSCLKSOURCE_HSE
#define FLASH_LATENCY_CONFIG FLASH_LATENCY_2
#define CONFIG_COMMENT "/* 配置: HSE+LSE */"
#elif (CLOCK_CONFIG_HSI_LSE_ONLY == 1)
// 模式2: HSI(16MHz/PLL后32MHz)+LSE
#define OSC_TYPE_CONFIG (RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSE)
#define HSE_STATE_CONFIG RCC_HSE_OFF
#define HSI_STATE_CONFIG RCC_HSI_ON
#define HSI48_CONFIG_ENABLE 0
#define SYSCLK_SOURCE_CONFIG RCC_SYSCLKSOURCE_HSI
#define FLASH_LATENCY_CONFIG FLASH_LATENCY_2
#define CONFIG_COMMENT "/* 配置: HSI+LSE */"
#elif (CLOCK_CONFIG_HSE_LSI_ONLY == 1)
// 模式3: HSE + LSI (去掉外部32k, 牺牲RTC精度)
#define OSC_TYPE_CONFIG (RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSI1)
#define HSE_STATE_CONFIG RCC_HSE_ON
#define HSI_STATE_CONFIG RCC_HSI_OFF
#define HSI48_CONFIG_ENABLE 0
#define SYSCLK_SOURCE_CONFIG RCC_SYSCLKSOURCE_HSE
#define FLASH_LATENCY_CONFIG FLASH_LATENCY_2
#define CONFIG_COMMENT "/* 配置: HSE+LSI (低成本, RTC精度下降) */"
#elif (CLOCK_CONFIG_HSI_LSI_ONLY == 1)
// 模式4: HSI + LSI (最低BOM, RTC误差最大)
#define OSC_TYPE_CONFIG (RCC_OSCILLATORTYPE_HSI  | RCC_OSCILLATORTYPE_LSI1)
#define HSE_STATE_CONFIG RCC_HSE_OFF
#define HSI_STATE_CONFIG RCC_HSI_ON
#define HSI48_CONFIG_ENABLE 0
#define SYSCLK_SOURCE_CONFIG RCC_SYSCLKSOURCE_HSI
#define FLASH_LATENCY_CONFIG FLASH_LATENCY_2
#define CONFIG_COMMENT "/* 配置: HSI+LSI (最小BOM) */"
#elif CLOCK_CONFIG_HSI_INCLUDED
// 模式5: HSI + HSI48 (+ LSE 默认) 测试/需要RNG
#define OSC_TYPE_CONFIG (RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_HSI | (LOW_SPEED_IS_LSE?RCC_OSCILLATORTYPE_LSE:RCC_OSCILLATORTYPE_LSI) | RCC_OSCILLATORTYPE_HSE)
#define HSE_STATE_CONFIG RCC_HSE_ON
#define HSI_STATE_CONFIG RCC_HSI_ON
#define HSI48_CONFIG_ENABLE 1
#define SYSCLK_SOURCE_CONFIG RCC_SYSCLKSOURCE_HSI
#define FLASH_LATENCY_CONFIG FLASH_LATENCY_2
#define CONFIG_COMMENT "/* 配置: HSI+HSI48 (+低速) */"
#else
#error "请选择一个有效的时钟配置!"
#endif

#if CLOCK_CONFIG_USE_HSE_PLL && (CLOCK_CONFIG_HSE_LSE_ONLY || CLOCK_CONFIG_HSE_LSI_ONLY)
// PLL: HSE -> PLL -> 32MHz
#define PLL_STATE_CONFIG RCC_PLL_ON
#define PLL_SOURCE_CONFIG RCC_PLLSOURCE_HSE
#define PLL_M_CONFIG RCC_PLLM_DIV2
#define PLL_N_CONFIG 8
#define PLL_P_CONFIG RCC_PLLP_DIV4
#define PLL_R_CONFIG RCC_PLLR_DIV4
#define PLL_Q_CONFIG RCC_PLLQ_DIV4
#undef SYSCLK_SOURCE_CONFIG
#define SYSCLK_SOURCE_CONFIG RCC_SYSCLKSOURCE_PLLCLK
#undef FLASH_LATENCY_CONFIG
#define FLASH_LATENCY_CONFIG FLASH_LATENCY_1
#elif CLOCK_CONFIG_USE_HSI_PLL && (CLOCK_CONFIG_HSI_LSE_ONLY || CLOCK_CONFIG_HSI_LSI_ONLY || CLOCK_CONFIG_HSI_INCLUDED)
// PLL: HSI -> PLL -> 32MHz
#define PLL_STATE_CONFIG RCC_PLL_ON
#define PLL_SOURCE_CONFIG RCC_PLLSOURCE_HSI
#define PLL_M_CONFIG RCC_PLLM_DIV1
#define PLL_N_CONFIG 8
#define PLL_P_CONFIG RCC_PLLP_DIV4
#define PLL_R_CONFIG RCC_PLLR_DIV4
#define PLL_Q_CONFIG RCC_PLLQ_DIV4
#undef SYSCLK_SOURCE_CONFIG
#define SYSCLK_SOURCE_CONFIG RCC_SYSCLKSOURCE_PLLCLK
#undef FLASH_LATENCY_CONFIG
#define FLASH_LATENCY_CONFIG FLASH_LATENCY_1
#else
#define PLL_STATE_CONFIG RCC_PLL_NONE
#endif

/* 模式快速说明:
 * 1 HSE+LSE: 精准主频+精准RTC, 功耗最低 (需2晶振)
 * 2 HSI+LSE: 减少高速晶振, RTC精度保留
 * 3 HSE+LSI: 保留高速精度, 放弃RTC精度
 * 4 HSI+LSI: 最低成本, RTC漂移大
 * 5 HSI+HSI48(+LSE/LSI): 需要RNG/USB
 * 选择 PLL 宏可把主频固定到 32MHz
 */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
IPCC_HandleTypeDef hipcc;
UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_lpuart1_tx;
DMA_HandleTypeDef hdma_usart1_tx;

// RNG_HandleTypeDef hrng; // 不需要RNG

RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_RTC_Init(void);
static void MX_IPCC_Init(void);
// static void MX_RNG_Init(void); // 不需要RNG
static void MX_RF_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  /* Config code for STM32_WPAN (HSE Tuning must be done before system clock configuration) */
  MX_APPE_Config();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* IPCC initialisation */
  MX_IPCC_Init();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_RTC_Init();
  #if CLOCK_CONFIG_HSI_INCLUDED
  MX_RNG_Init(); // 不需要RNG，链接器分析显示未使用，且关闭了HSI48
  #endif
  MX_RF_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */
  /* Init code for STM32_WPAN */

  MX_APPE_Init();

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    MX_APPE_Process();

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

  /** Configure LSE Drive Capability
   */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Configure the main internal regulator output voltage
   */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  // CONFIG_COMMENT

  RCC_OscInitStruct.OscillatorType = OSC_TYPE_CONFIG;
  RCC_OscInitStruct.HSEState = HSE_STATE_CONFIG; // 使用抽象后的宏自动控制开启/关闭HSE
  RCC_OscInitStruct.LSEState = LSE_STATE_CONFIG; // 支持 LSE
  RCC_OscInitStruct.HSIState = HSI_STATE_CONFIG;
  RCC_OscInitStruct.LSIState = LSI_STATE_CONFIG;
#if HSI48_CONFIG_ENABLE
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
#endif

  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;

#if CLOCK_CONFIG_USE_HSE_PLL || CLOCK_CONFIG_USE_HSI_PLL
  // 使用PLL配置
  RCC_OscInitStruct.PLL.PLLState = PLL_STATE_CONFIG;
  RCC_OscInitStruct.PLL.PLLSource = PLL_SOURCE_CONFIG;
  RCC_OscInitStruct.PLL.PLLM = PLL_M_CONFIG;
  RCC_OscInitStruct.PLL.PLLN = PLL_N_CONFIG;
  RCC_OscInitStruct.PLL.PLLP = PLL_P_CONFIG;
  RCC_OscInitStruct.PLL.PLLR = PLL_R_CONFIG;
  RCC_OscInitStruct.PLL.PLLQ = PLL_Q_CONFIG;
#else
  // 不使用PLL
  RCC_OscInitStruct.PLL.PLLState = PLL_STATE_CONFIG;
#endif
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource, HCLK, PCLK1 and PCLK2 clocks dividers
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK4 | RCC_CLOCKTYPE_HCLK2 | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = SYSCLK_SOURCE_CONFIG; /* 使用宏定义的时钟源 */
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK2Divider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK4Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_CONFIG) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief Peripherals Common Clock Configuration
 * @retval None
 */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
   */
  PeriphClkInitStruct.PeriphClockSelection   = RCC_PERIPHCLK_SMPS | RCC_PERIPHCLK_RFWAKEUP;
  PeriphClkInitStruct.RFWakeUpClockSelection = RF_WAKEUP_CLK;          /* 依据低速时钟宏选择 LSE 或 LSI */
  PeriphClkInitStruct.SmpsClockSelection     = SMPS_CLK_SRC_CONFIG;    /* HSE_LSE_ONLY 模式下使用 HSE */
  PeriphClkInitStruct.SmpsDivSelection       = RCC_SMPSCLKDIV_RANGE1;  /* 分频保持默认 */

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN Smps */

  /* USER CODE END Smps */
}

/**
 * @brief IPCC Initialization Function
 * @param None
 * @retval None
 */
static void MX_IPCC_Init(void)
{

  /* USER CODE BEGIN IPCC_Init 0 */

  /* USER CODE END IPCC_Init 0 */

  /* USER CODE BEGIN IPCC_Init 1 */

  /* USER CODE END IPCC_Init 1 */
  hipcc.Instance = IPCC;
  if (HAL_IPCC_Init(&hipcc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IPCC_Init 2 */

  /* USER CODE END IPCC_Init 2 */
}

/**
 * @brief LPUART1 Initialization Function - DISABLED
 * @param None
 * @retval None
 */

void MX_LPUART1_UART_Init(void)
{
#if 0 // LPUART1功能已禁用
  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  hlpuart1.FifoMode = UART_FIFOMODE_DISABLE;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */
#endif
}

/**
 * @brief USART1 Initialization Function - DISABLED
 * @param None
 * @retval None
 */

void MX_USART1_UART_Init(void)
{
#if 0 // USART1功能已禁用
  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_8;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */
#endif
}

/**
 * @brief RF Initialization Function
 * @param None
 * @retval None
 */
static void MX_RF_Init(void)
{

  /* USER CODE BEGIN RF_Init 0 */

  /* USER CODE END RF_Init 0 */

  /* USER CODE BEGIN RF_Init 1 */

  /* USER CODE END RF_Init 1 */
  /* USER CODE BEGIN RF_Init 2 */

  /* USER CODE END RF_Init 2 */
}

/**
 * @brief RNG Initialization Function - DISABLED
 * @param None
 * @retval None
 */
#if 0 // RNG功能已禁用 - 不需要HSI48时钟
static void MX_RNG_Init(void)
{

  /* USER CODE BEGIN RNG_Init 0 */

  /* USER CODE END RNG_Init 0 */

  /* USER CODE BEGIN RNG_Init 1 */

  /* USER CODE END RNG_Init 1 */
  hrng.Instance = RNG;
  hrng.Init.ClockErrorDetection = RNG_CED_ENABLE;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG_Init 2 */

  /* USER CODE END RNG_Init 2 */

}
#endif

/**
 * @brief RTC Initialization Function
 * @param None
 * @retval None
 */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
   */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = CFG_RTC_ASYNCH_PRESCALER;
  hrtc.Init.SynchPrediv = CFG_RTC_SYNCH_PRESCALER;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the WakeUp
   */
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */
}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  // /* DMA1_Channel4_IRQn interrupt configuration - DISABLED (UART related) */
  //  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 15, 0);
  //  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  // /* DMA2_Channel4_IRQn interrupt configuration - DISABLED (UART related) */
  //  HAL_NVIC_SetPriority(DMA2_Channel4_IRQn, 15, 0);
  //  HAL_NVIC_EnableIRQ(DMA2_Channel4_IRQn);
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

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
