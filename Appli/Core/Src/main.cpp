/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.cpp
  * @brief          : Main program body (C++ version) for NUCLEO-N657X0-Q
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "hand/hand_config.hpp"
#include "hand/servo.hpp"
#include "hand/hand_controller.hpp"
#include "hand/serial_commander.hpp"
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
CACHEAXI_HandleTypeDef hcacheaxi;

UART_HandleTypeDef huart3;
DMA_HandleTypeDef handle_GPDMA1_Channel1;
DMA_HandleTypeDef handle_GPDMA1_Channel0;

/* USER CODE BEGIN PV */

// 1. Forward-Deklarationen
extern HandController rightHand;
extern HandController leftHand;

// 2. Der Executor: Verbindet ROS2-Befehle mit der Motor-Logik
class GripExecutor : public ICommandExecutor {
public:
    bool executeGrip(HandControl::Hand::Side side, HandControl::GripType grip) override {
        // Starte den Griff mit 1000ms weicher Übergangszeit
        if (side == HandControl::Hand::Side::Right) {
            rightHand.setTargetGrip(grip, 1000); 
        } else {
            leftHand.setTargetGrip(grip, 1000);
        }
        return true;
    }
};

// 3. Hardware-Schnittstellen instanziieren
// Port für die Servos (USART3)
Stm32UartDmaPort servoPort(&huart3);
ServoBus servoBus(servoPort);

/** * HINWEIS: Für ROS 2 über USB nutzt man am Nucleo oft USART2 (VCP).
 * Falls du diesen in CubeMX noch nicht aktiviert hast, nutzt der Commander
 * hier als Platzhalter den servoPort, damit der Code kompiliert.
 */
// extern UART_HandleTypeDef huart2; 
// Stm32UartDmaPort rosPort(&huart2);
// SerialCommander commander(rosPort);
SerialCommander commander(servoPort); 

// 4. Die Controller-Instanzen
HandController rightHand(HandControl::Hand::Side::Right, servoBus);
HandController leftHand(HandControl::Hand::Side::Left, servoBus);
GripExecutor gripExecutor;

// Puffer für den UART-Empfang (Interrupt-basiert)
uint8_t rx_byte = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void MX_GPIO_Init(void);
static void MX_GPDMA1_Init(void);
static void MX_CACHEAXI_Init(void);
static void MX_USART3_UART_Init(void);
static void SystemIsolation_Config(void);
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
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_CACHEAXI_Init();
  MX_USART3_UART_Init();
  SystemIsolation_Config();
  /* USER CODE BEGIN 2 */

  // Commander mit dem Executor verknüpfen
  commander.setExecutor(&gripExecutor);

  // Empfang für den ROS2-Port starten (Beispiel USART2 einkommentieren falls vorhanden)
  // HAL_UART_Receive_IT(&huart2, &rx_byte, 1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    // 1. Eingehende serielle Befehle verarbeiten
    commander.processCommand();

    // 2. 100Hz Control Loop für die Hände (alle 10ms)
    static uint32_t last_100hz_tick = 0;
    uint32_t current_tick = HAL_GetTick();
    
    if (current_tick - last_100hz_tick >= 10) {
        last_100hz_tick = current_tick;
        
        // Berechnet Trajektorien, checkt Telemetrie und sendet Sync-Write an Servos
        rightHand.update();
        leftHand.update();
    }

  }
  /* USER CODE END 3 */
}

/**
  * @brief CACHEAXI Initialization Function
  * @param None
  * @retval None
  */
static void MX_CACHEAXI_Init(void)
{
  /* USER CODE BEGIN CACHEAXI_Init 0 */
  /* USER CODE END CACHEAXI_Init 0 */

  /* USER CODE BEGIN CACHEAXI_Init 1 */
  /* USER CODE END CACHEAXI_Init 1 */
  hcacheaxi.Instance = CACHEAXI;
  if (HAL_CACHEAXI_Init(&hcacheaxi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CACHEAXI_Init 2 */
  /* USER CODE END CACHEAXI_Init 2 */
}

/**
  * @brief GPDMA1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPDMA1_Init(void)
{
  /* USER CODE BEGIN GPDMA1_Init 0 */
  /* USER CODE END GPDMA1_Init 0 */
  __HAL_RCC_GPDMA1_CLK_ENABLE();
  HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);
  HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);
}

/**
  * @brief RIF Initialization Function
  * @param None
  * @retval None
  */
static void SystemIsolation_Config(void)
{
  __HAL_RCC_RIFSC_CLK_ENABLE();
  RIMC_MasterConfig_t RIMC_master = {0};
  RIMC_master.MasterCID = RIF_CID_1;
  RIMC_master.SecPriv = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_NPRIV;
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_ETH1, &RIMC_master);

  if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel0,DMA_CHANNEL_SEC|DMA_CHANNEL_PRIV|DMA_CHANNEL_SRC_SEC|DMA_CHANNEL_DEST_SEC)!= HAL_OK )
  {
    Error_Handler();
  }
  if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel1,DMA_CHANNEL_SEC|DMA_CHANNEL_PRIV|DMA_CHANNEL_SRC_SEC|DMA_CHANNEL_DEST_SEC)!= HAL_OK )
  {
    Error_Handler();
  }

  HAL_GPIO_ConfigPinAttributes(GPIOA,GPIO_PIN_5,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOA,GPIO_PIN_7,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOA,GPIO_PIN_10,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOA,GPIO_PIN_11,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOB,GPIO_PIN_0,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOB,GPIO_PIN_3,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOB,GPIO_PIN_6,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOB,GPIO_PIN_7,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOB,GPIO_PIN_10,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOB,GPIO_PIN_11,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOC,GPIO_PIN_1,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOD,GPIO_PIN_2,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOD,GPIO_PIN_8,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOD,GPIO_PIN_9,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOD,GPIO_PIN_10,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOE,GPIO_PIN_3,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOH,GPIO_PIN_9,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPION,GPIO_PIN_7,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
  HAL_GPIO_ConfigPinAttributes(GPIOO,GPIO_PIN_5,GPIO_PIN_SEC|GPIO_PIN_NPRIV);
}

static void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 1000000;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8);
  HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8);
  HAL_UARTEx_DisableFifoMode(&huart3);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOO_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPION_CLK_ENABLE();

  HAL_GPIO_WritePin(PWR_EN_GPIO_Port, PWR_EN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(CAM_NRST_GPIO_Port, CAM_NRST_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, UCPD1_ISENSE_Pin|UCPD_PWR_EN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(USB1_OCP_GPIO_Port, USB1_OCP_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = PWR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(PWR_EN_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = UCPD1_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(UCPD1_INT_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = CAM_NRST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CAM_NRST_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = UCPD1_ISENSE_Pin|UCPD_PWR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = USB1_OCP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB1_OCP_GPIO_Port, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/**
 * @brief HAL UART Empfangs-Callback
 * Leitet Bytes an den Commander weiter oder signalisiert DMA-Ende an den Servo-Port.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    // A: Empfang von ROS 2 (hier beispielhaft USART2 prüfen)
    /*
    if (huart->Instance == USART2) { 
        commander.feedByte(rx_byte);
        HAL_UART_Receive_IT(huart, &rx_byte, 1); // Nächstes Byte anfordern
    }
    */
    
    // B: Telemetrie-Antwort der Servos auf USART3
    if (huart->Instance == USART3) { 
        Stm32UartDmaPort::onRxComplete(huart); //
    }
}

/**
 * @brief HAL UART Sende-Callback
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) {
        Stm32UartDmaPort::onTxComplete(huart); //
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
#ifdef __cplusplus
extern "C" {
#endif
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
#ifdef __cplusplus
}
#endif
