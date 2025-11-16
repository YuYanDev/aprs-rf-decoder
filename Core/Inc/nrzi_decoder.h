/**
  ******************************************************************************
  * @file    nrzi_decoder.h
  * @brief   NRZI Decoder with Bit Stuffing Header
  ******************************************************************************
  */

#ifndef __NRZI_DECODER_H
#define __NRZI_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "aprs_config.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  NRZI Decoder Handle Structure
  */
typedef struct {
    uint8_t last_bit;              /* Last bit value for NRZI decoding */
    uint8_t bit_buffer;            /* Bit accumulator */
    uint8_t bit_count;             /* Number of bits in buffer */
    uint8_t ones_count;            /* Consecutive ones counter */
    uint8_t decoded_byte;          /* Decoded byte */
    bool byte_ready;               /* Byte ready flag */
    bool flag_detected;            /* Frame flag (0x7E) detected */
    uint32_t bit_counter;          /* Total bits processed */
} NRZI_Decoder_HandleTypeDef;

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize NRZI decoder
  * @param  hnrzi: Pointer to NRZI decoder handle
  * @retval 0 if success, -1 otherwise
  */
int32_t NRZI_Init(NRZI_Decoder_HandleTypeDef *hnrzi);

/**
  * @brief  Reset NRZI decoder state
  * @param  hnrzi: Pointer to NRZI decoder handle
  * @retval None
  */
void NRZI_Reset(NRZI_Decoder_HandleTypeDef *hnrzi);

/**
  * @brief  Process a single bit
  * @param  hnrzi: Pointer to NRZI decoder handle
  * @param  bit: Input bit (0 or 1)
  * @retval true if a byte is ready, false otherwise
  */
bool NRZI_ProcessBit(NRZI_Decoder_HandleTypeDef *hnrzi, uint8_t bit);

/**
  * @brief  Get decoded byte
  * @param  hnrzi: Pointer to NRZI decoder handle
  * @retval Decoded byte value
  */
uint8_t NRZI_GetByte(NRZI_Decoder_HandleTypeDef *hnrzi);

/**
  * @brief  Check if frame flag was detected
  * @param  hnrzi: Pointer to NRZI decoder handle
  * @retval true if flag detected, false otherwise
  */
bool NRZI_IsFlagDetected(NRZI_Decoder_HandleTypeDef *hnrzi);

#ifdef __cplusplus
}
#endif

#endif /* __NRZI_DECODER_H */

