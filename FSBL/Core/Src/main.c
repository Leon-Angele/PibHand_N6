/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : First Stage Bootloader (FSBL) for STM32N6
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private variables ---------------------------------------------------------*/
XSPI_HandleTypeDef hxspi2;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_GPDMA1_Init(void);
static void MX_XSPI2_Init(void);
void BOOT_Application(void);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* Unlock BSEC Access Port & Enable Debug */
  BSEC->AP_UNLOCK = 0xB4;
  BSEC->DBGCR = 0xB451B400;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_XSPI2_Init();

  /* USER CODE BEGIN 2 */

  /* --- SCHRITT 1: LED-CHECK (FSBL START) --- */
  // Wir schalten die grüne LED an, damit wir wissen: FSBL läuft!
  BSP_LED_Init(LED1);
  BSP_LED_On(LED1);
  HAL_Delay(1000); // 1 Sekunde leuchten lassen

  /* --- SCHRITT 2: XSPI KONFIGURATION (READ & WRITE) --- */
  XSPI_RegularCmdTypeDef sCommand = {0};
  XSPI_MemoryMappedTypeDef sMemMappedCfg = {0};

  // A) READ Configuration
  sCommand.OperationType      = HAL_XSPI_OPTYPE_READ_CFG;
  sCommand.InstructionMode    = HAL_XSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction        = 0x03; // Standard Read
  sCommand.AddressMode        = HAL_XSPI_ADDRESS_1_LINE;
  sCommand.AddressWidth       = HAL_XSPI_ADDRESS_32_BITS;
  sCommand.DataMode           = HAL_XSPI_DATA_1_LINE;
  sCommand.DummyCycles        = 0;
  sCommand.DQSMode            = HAL_XSPI_DQS_DISABLE;

  if (HAL_XSPI_Command(&hxspi2, &sCommand, HAL_MAX_DELAY) != HAL_OK) {
    Error_Handler();
  }

  // B) WRITE Configuration (WICHTIG: Bringt den State von 20 auf 4!)
  sCommand.OperationType      = HAL_XSPI_OPTYPE_WRITE_CFG;
  sCommand.Instruction        = 0x02; // Standard Page Program

  if (HAL_XSPI_Command(&hxspi2, &sCommand, HAL_MAX_DELAY) != HAL_OK) {
    Error_Handler();
  }

  // C) Memory-Mapped Mode aktivieren
  sMemMappedCfg.TimeOutActivation = HAL_XSPI_TIMEOUT_COUNTER_DISABLE;
  if (HAL_XSPI_MemoryMapped(&hxspi2, &sMemMappedCfg) != HAL_OK) {
    Error_Handler();
  }

  /* --- SCHRITT 3: JUMP TO APPLICATION --- */
  // LED kurz ausmachen als Zeichen für den Sprung
  BSP_LED_Off(LED1);
  HAL_Delay(200);

  BOOT_Application();

  /* USER CODE END 2 */

  /* Infinite loop */
  while (1)
  {
      // Sollte nie erreicht werden
  }
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  if (HAL_PWREx_ConfigSupply(PWR_EXTERNAL_SOURCE_SUPPLY) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
    Error_Handler();
  }

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL1.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL1.PLLM = 2;
  RCC_OscInitStruct.PLL1.PLLN = 75;
  RCC_OscInitStruct.PLL1.PLLP1 = 1;
  RCC_OscInitStruct.PLL1.PLLP2 = 1;

  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL2.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL2.PLLM = 4;
  RCC_OscInitStruct.PLL2.PLLN = 75;
  RCC_OscInitStruct.PLL2.PLLP1 = 2;
  RCC_OscInitStruct.PLL2.PLLP2 = 1;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_CPUCLK|RCC_CLOCKTYPE_HCLK
                              |RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2|RCC_CLOCKTYPE_PCLK5
                              |RCC_CLOCKTYPE_PCLK4;
  RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_IC1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_IC2_IC6_IC11;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.IC1Selection.ClockSelection = RCC_ICCLKSOURCE_PLL2;
  RCC_ClkInitStruct.IC1Selection.ClockDivider = 1;
  RCC_ClkInitStruct.IC2Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC2Selection.ClockDivider = 6;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_GPDMA1_Init(void)
{
  __HAL_RCC_GPDMA1_CLK_ENABLE();
}

static void MX_XSPI2_Init(void)
{
  XSPIM_CfgTypeDef sXspiManagerCfg = {0};

  hxspi2.Instance = XSPI2;
  hxspi2.Init.FifoThresholdByte = 4;
  hxspi2.Init.MemoryMode = HAL_XSPI_SINGLE_MEM;
  hxspi2.Init.MemoryType = HAL_XSPI_MEMTYPE_MACRONIX;
  hxspi2.Init.MemorySize = HAL_XSPI_SIZE_32GB;
  hxspi2.Init.ChipSelectHighTimeCycle = 2;
  hxspi2.Init.ClockMode = HAL_XSPI_CLOCK_MODE_0;
  hxspi2.Init.ClockPrescaler = 0;
  hxspi2.Init.SampleShifting = HAL_XSPI_SAMPLE_SHIFT_NONE;
  hxspi2.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_ENABLE;
  hxspi2.Init.MemorySelect = HAL_XSPI_CSSEL_NCS1;

  if (HAL_XSPI_Init(&hxspi2) != HAL_OK) {
    Error_Handler();
  }
  sXspiManagerCfg.nCSOverride = HAL_XSPI_CSSEL_OVR_NCS1;
  sXspiManagerCfg.IOPort = HAL_XSPIM_IOPORT_2;
  sXspiManagerCfg.Req2AckTime = 1;
  if (HAL_XSPIM_Config(&hxspi2, &sXspiManagerCfg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPION_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
}

/* USER CODE BEGIN 4 */
typedef void (*pFunction)(void);

void BOOT_Application(void)
{
  uint32_t JumpAddress;
  pFunction Jump_To_Application;

  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL  = 0;

  HAL_DeInit();
  __disable_irq();

  // Stackpointer laden
  __set_MSP(*(__IO uint32_t*) 0x70100000);

  // Jump Address (Reset Vector)
  JumpAddress = *(__IO uint32_t*) (0x70100000 + 4);
  Jump_To_Application = (pFunction) JumpAddress;

  Jump_To_Application();
}
/* USER CODE END 4 */

void Error_Handler(void)
{
  /* Wenn Fehler: LED blinkt schnell */
  __disable_irq();
  while (1)
  {
      BSP_LED_Toggle(LED1);
      for(volatile uint32_t i = 0; i < 500000; i++);
  }
}
