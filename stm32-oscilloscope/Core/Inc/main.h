/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_tx;

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

extern UART_HandleTypeDef huart1;

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TFT_DC_Pin GPIO_PIN_2
#define TFT_DC_GPIO_Port GPIOA
#define TFT_RST_Pin GPIO_PIN_3
#define TFT_RST_GPIO_Port GPIOA
#define TFT_CS_Pin GPIO_PIN_4
#define TFT_CS_GPIO_Port GPIOA
#define TFT_SCL_Pin GPIO_PIN_5
#define TFT_SCL_GPIO_Port GPIOA
#define TFT_BLK_Pin GPIO_PIN_6
#define TFT_BLK_GPIO_Port GPIOA
#define TFT_SDA_Pin GPIO_PIN_7
#define TFT_SDA_GPIO_Port GPIOA
#define BTN_1_Pin GPIO_PIN_12
#define BTN_1_GPIO_Port GPIOB
#define BTN_2_Pin GPIO_PIN_13
#define BTN_2_GPIO_Port GPIOB
#define BTN_3_Pin GPIO_PIN_14
#define BTN_3_GPIO_Port GPIOB
#define BTN_4_Pin GPIO_PIN_15
#define BTN_4_GPIO_Port GPIOB
#define ENC_SW1_Pin GPIO_PIN_11
#define ENC_SW1_GPIO_Port GPIOA
#define ENC_SW2_Pin GPIO_PIN_12
#define ENC_SW2_GPIO_Port GPIOA
#define ENC1_A_Pin GPIO_PIN_4
#define ENC1_A_GPIO_Port GPIOB
#define ENC1_B_Pin GPIO_PIN_5
#define ENC1_B_GPIO_Port GPIOB
#define ENC2_A_Pin GPIO_PIN_6
#define ENC2_A_GPIO_Port GPIOB
#define ENC2_B_Pin GPIO_PIN_7
#define ENC2_B_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
