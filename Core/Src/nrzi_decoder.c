/**
  ******************************************************************************
  * @file    nrzi_decoder.c
  * @brief   NRZI Decoder with Bit Stuffing Implementation
  ******************************************************************************
  * @attention
  *
  * This module implements NRZI decoding and bit de-stuffing for AX.25
  * NRZI: No transition = 1, Transition = 0
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "nrzi_decoder.h"
#include <string.h>

/* Private function prototypes -----------------------------------------------*/
static uint8_t NRZI_Decode(NRZI_Decoder_HandleTypeDef *hnrzi, uint8_t bit);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Decode NRZI bit
  * @param  hnrzi: Pointer to NRZI decoder handle
  * @param  bit: Input bit (0 or 1)
  * @retval Decoded bit
  */
static uint8_t NRZI_Decode(NRZI_Decoder_HandleTypeDef *hnrzi, uint8_t bit)
{
    /* NRZI: If bit equals last bit, output 1; otherwise output 0 */
    uint8_t decoded = (bit == hnrzi->last_bit) ? 1 : 0;
    hnrzi->last_bit = bit;
    return decoded;
}

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  Initialize NRZI decoder
  * @param  hnrzi: Pointer to NRZI decoder handle
  * @retval 0 if success, -1 otherwise
  */
int32_t NRZI_Init(NRZI_Decoder_HandleTypeDef *hnrzi)
{
    if (hnrzi == NULL) {
        return -1;
    }
    
    NRZI_Reset(hnrzi);
    return 0;
}

/**
  * @brief  Reset NRZI decoder state
  * @param  hnrzi: Pointer to NRZI decoder handle
  * @retval None
  */
void NRZI_Reset(NRZI_Decoder_HandleTypeDef *hnrzi)
{
    hnrzi->last_bit = 0;
    hnrzi->bit_buffer = 0;
    hnrzi->bit_count = 0;
    hnrzi->ones_count = 0;
    hnrzi->decoded_byte = 0;
    hnrzi->byte_ready = false;
    hnrzi->flag_detected = false;
    hnrzi->bit_counter = 0;
}

/**
  * @brief  Process a single bit
  * @param  hnrzi: Pointer to NRZI decoder handle
  * @param  bit: Input bit (0 or 1)
  * @retval true if a byte is ready, false otherwise
  */
bool NRZI_ProcessBit(NRZI_Decoder_HandleTypeDef *hnrzi, uint8_t bit)
{
    /* NRZI decode */
    uint8_t decoded_bit = NRZI_Decode(hnrzi, bit);
    
    /* Update flag detection window (shift in new bit) */
    hnrzi->bit_buffer = (hnrzi->bit_buffer << 1) | decoded_bit;
    
    /* Detect frame flag 0x7E = 01111110 */
    if (hnrzi->bit_buffer == AX25_FLAG) {
        hnrzi->flag_detected = true;
        /* Frame flag is not data, reset receiver */
        hnrzi->decoded_byte = 0;
        hnrzi->bit_count = 0;
        hnrzi->ones_count = 0;
        hnrzi->byte_ready = false;
        return false;
    }
    
    hnrzi->flag_detected = false;
    
    /* Handle bit stuffing */
    if (decoded_bit == 1) {
        hnrzi->ones_count++;
        
        /* If 6 consecutive ones, frame error (should have stuffing bit after 5 ones) */
        if (hnrzi->ones_count > 6) {
            /* Frame error, reset */
            NRZI_Reset(hnrzi);
            return false;
        }
    } else {  /* decoded_bit == 0 */
        /* If preceded by 5 ones, this 0 is a stuffing bit, discard it */
        if (hnrzi->ones_count == 5) {
            hnrzi->ones_count = 0;
            return false;  /* Don't add stuffing bit to data */
        }
        hnrzi->ones_count = 0;
    }
    
    /* Normal data bit, add to receive buffer */
    /* AX.25 uses LSB first */
    hnrzi->decoded_byte >>= 1;
    if (decoded_bit) {
        hnrzi->decoded_byte |= 0x80;
    }
    
    hnrzi->bit_count++;
    hnrzi->bit_counter++;
    
    /* Complete byte received */
    if (hnrzi->bit_count >= 8) {
        hnrzi->byte_ready = true;
        hnrzi->bit_count = 0;
        return true;
    }
    
    return false;
}

/**
  * @brief  Get decoded byte
  * @param  hnrzi: Pointer to NRZI decoder handle
  * @retval Decoded byte value
  */
uint8_t NRZI_GetByte(NRZI_Decoder_HandleTypeDef *hnrzi)
{
    hnrzi->byte_ready = false;
    return hnrzi->decoded_byte;
}

/**
  * @brief  Check if frame flag was detected
  * @param  hnrzi: Pointer to NRZI decoder handle
  * @retval true if flag detected, false otherwise
  */
bool NRZI_IsFlagDetected(NRZI_Decoder_HandleTypeDef *hnrzi)
{
    return hnrzi->flag_detected;
}

