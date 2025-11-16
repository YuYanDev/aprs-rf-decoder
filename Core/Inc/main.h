/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"  /* Will be auto-selected based on target */

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* Private defines -----------------------------------------------------------*/
/* SX1276/SX1278 Pin Definitions */
#define SX127X_NSS_PIN       GPIO_PIN_4
#define SX127X_NSS_GPIO_PORT GPIOA
#define SX127X_DIO0_PIN      GPIO_PIN_2
#define SX127X_DIO0_GPIO_PORT GPIOA
#define SX127X_DIO2_PIN      GPIO_PIN_3
#define SX127X_DIO2_GPIO_PORT GPIOA
#define SX127X_RESET_PIN     GPIO_PIN_1
#define SX127X_RESET_GPIO_PORT GPIOA

/* UART Definitions */
#define DEBUG_UART           USART2
#define APRS_UART            USART1

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

