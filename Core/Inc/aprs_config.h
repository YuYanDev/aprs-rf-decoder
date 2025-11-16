/**
  ******************************************************************************
  * @file    aprs_config.h
  * @brief   APRS Decoder Configuration File
  ******************************************************************************
  */

#ifndef __APRS_CONFIG_H
#define __APRS_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* RF Configuration ----------------------------------------------------------*/
#define RF_FREQUENCY        434.0f      /* MHz - APRS frequency */
#define RF_BITRATE          26.4f       /* kbps - Sampling rate */
#define RF_DEVIATION        3.0f        /* kHz - FSK frequency deviation */

/* AFSK Parameters -----------------------------------------------------------*/
#define AFSK_MARK_FREQ      2200        /* Hz - Mark frequency (logic 1) */
#define AFSK_SPACE_FREQ     1200        /* Hz - Space frequency (logic 0) */
#define AFSK_BAUD_RATE      1200        /* bps - Baud rate */
#define AFSK_SAMPLE_RATE    26400       /* Hz - Sampling frequency */

/* Sample Calculations -------------------------------------------------------*/
#define SAMPLES_PER_BIT     (AFSK_SAMPLE_RATE / AFSK_BAUD_RATE)   /* 22 */
#define SAMPLES_PER_MARK    (AFSK_SAMPLE_RATE / AFSK_MARK_FREQ)   /* 12 */
#define SAMPLES_PER_SPACE   (AFSK_SAMPLE_RATE / AFSK_SPACE_FREQ)  /* 22 */

/* AX.25 Protocol Parameters -------------------------------------------------*/
#define AX25_FLAG           0x7E        /* Frame flag */
#define AX25_MIN_FRAME_LEN  18          /* Minimum frame length */
#define AX25_MAX_FRAME_LEN  330         /* Maximum frame length */
#define AX25_ADDR_LEN       7           /* Address field length */
#define AX25_CONTROL        0x03        /* UI frame control field */
#define AX25_PID            0xF0        /* No layer 3 protocol */

/* Buffer Configuration ------------------------------------------------------*/
#define RX_BUFFER_SIZE      512         /* Receive buffer size (bytes) */
#define SAMPLE_BUFFER_SIZE  256         /* Sample buffer size */
#define BIT_BUFFER_SIZE     (AX25_MAX_FRAME_LEN * 8 + 64)

/* DMA Configuration ---------------------------------------------------------*/
#define USE_DMA             1           /* Enable DMA transfer */
#define DMA_BUFFER_SIZE     128         /* DMA buffer size */

/* DSP Configuration ---------------------------------------------------------*/
/* Auto-detect MCU capabilities */
#if defined(STM32L412xx) || defined(STM32F401xE) || defined(STM32F411xE)
  #define HAS_FPU           1
  #define HAS_DSP           1
  #define USE_CMSIS_DSP     1
#else
  #define HAS_FPU           0
  #define HAS_DSP           0
  #define USE_CMSIS_DSP     0
#endif

/* Signal Processing Parameters ----------------------------------------------*/
#define CORRELATION_WINDOW  22          /* Correlation window size */
#define PLL_LOCK_THRESHOLD  16          /* PLL lock threshold */
#define CARRIER_DETECT_THR  10          /* Carrier detection threshold */

/* UART Configuration --------------------------------------------------------*/
#define UART_BAUDRATE       9600        /* UART baud rate for APRS output */
#define DEBUG_BAUDRATE      115200      /* Debug UART baud rate */

/* Debug Configuration -------------------------------------------------------*/
#define DEBUG_ENABLED       1           /* Enable debug output */

#if DEBUG_ENABLED
  extern UART_HandleTypeDef huart2;  /* Debug UART handle */
  #define DEBUG_PRINTF(...)  printf(__VA_ARGS__)
#else
  #define DEBUG_PRINTF(...)
#endif

/* Performance Statistics ----------------------------------------------------*/
#define ENABLE_STATISTICS   1           /* Enable statistics */

#ifdef __cplusplus
}
#endif

#endif /* __APRS_CONFIG_H */

