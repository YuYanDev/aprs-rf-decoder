/**
  ******************************************************************************
  * @file    ax25_parser.h
  * @brief   AX.25 Frame Parser Header
  ******************************************************************************
  */

#ifndef __AX25_PARSER_H
#define __AX25_PARSER_H

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
  * @brief  AX.25 Address Structure
  */
typedef struct {
    char callsign[7];              /* Callsign (max 6 characters) */
    uint8_t ssid;                  /* SSID (0-15) */
} AX25_Address_TypeDef;

/**
  * @brief  AX.25 Frame Structure
  */
typedef struct {
    AX25_Address_TypeDef destination;     /* Destination address */
    AX25_Address_TypeDef source;          /* Source address */
    AX25_Address_TypeDef digipeaters[8];  /* Digipeater path */
    uint8_t num_digipeaters;              /* Number of digipeaters */
    uint8_t control;                      /* Control field */
    uint8_t pid;                          /* Protocol ID */
    uint8_t info[256];                    /* Information field */
    uint16_t info_len;                    /* Information length */
    bool valid;                           /* CRC valid flag */
} AX25_Frame_TypeDef;

/**
  * @brief  AX.25 Parser Handle Structure
  */
typedef struct {
    AX25_Frame_TypeDef current_frame;     /* Current frame being parsed */
    uint8_t raw_buffer[AX25_MAX_FRAME_LEN]; /* Raw byte buffer */
    uint16_t raw_buffer_pos;              /* Buffer position */
    uint16_t crc;                         /* CRC accumulator */
} AX25_Parser_HandleTypeDef;

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize AX.25 parser
  * @param  hax25: Pointer to AX.25 parser handle
  * @retval 0 if success, -1 otherwise
  */
int32_t AX25_Init(AX25_Parser_HandleTypeDef *hax25);

/**
  * @brief  Reset AX.25 parser state
  * @param  hax25: Pointer to AX.25 parser handle
  * @retval None
  */
void AX25_Reset(AX25_Parser_HandleTypeDef *hax25);

/**
  * @brief  Start new frame
  * @param  hax25: Pointer to AX.25 parser handle
  * @retval None
  */
void AX25_StartFrame(AX25_Parser_HandleTypeDef *hax25);

/**
  * @brief  Add byte to current frame
  * @param  hax25: Pointer to AX.25 parser handle
  * @param  byte: Byte to add
  * @retval true if frame is complete, false otherwise
  */
bool AX25_AddByte(AX25_Parser_HandleTypeDef *hax25, uint8_t byte);

/**
  * @brief  End current frame and verify CRC
  * @param  hax25: Pointer to AX.25 parser handle
  * @retval true if frame is valid, false otherwise
  */
bool AX25_EndFrame(AX25_Parser_HandleTypeDef *hax25);

/**
  * @brief  Get parsed frame
  * @param  hax25: Pointer to AX.25 parser handle
  * @retval Pointer to parsed frame
  */
AX25_Frame_TypeDef* AX25_GetFrame(AX25_Parser_HandleTypeDef *hax25);

#ifdef __cplusplus
}
#endif

#endif /* __AX25_PARSER_H */

