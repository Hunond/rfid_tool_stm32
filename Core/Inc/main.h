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

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Relay1_Pin GPIO_PIN_2
#define Relay1_GPIO_Port GPIOE
#define K1buttEXTI_Pin GPIO_PIN_3
#define K1buttEXTI_GPIO_Port GPIOE
#define K1buttEXTI_EXTI_IRQn EXTI3_IRQn
#define K0buttEXTI_Pin GPIO_PIN_4
#define K0buttEXTI_GPIO_Port GPIOE
#define K0buttEXTI_EXTI_IRQn EXTI4_IRQn
#define LED1_Pin GPIO_PIN_6
#define LED1_GPIO_Port GPIOA
#define LED2_Pin GPIO_PIN_7
#define LED2_GPIO_Port GPIOA
#define F_CS_Pin GPIO_PIN_0
#define F_CS_GPIO_Port GPIOB
#define Relay2_Pin GPIO_PIN_8
#define Relay2_GPIO_Port GPIOC
#define CS_Pin GPIO_PIN_0
#define CS_GPIO_Port GPIOD
#define RST_Pin GPIO_PIN_1
#define RST_GPIO_Port GPIOD
#define F_SCK_Pin GPIO_PIN_3
#define F_SCK_GPIO_Port GPIOB
#define F_MISO_Pin GPIO_PIN_4
#define F_MISO_GPIO_Port GPIOB
#define F_MOSI_Pin GPIO_PIN_5
#define F_MOSI_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
extern volatile uint8_t cdc_rx_buf[64];
extern volatile uint8_t cdc_rx_len;
extern volatile uint8_t cdc_rx_ready;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
