/**
  ******************************************************************************
  * @file    ax25_parser.c
  * @brief   AX.25 Frame Parser Implementation
  ******************************************************************************
  * @attention
  *
  * This module implements AX.25 UI frame parsing with CRC-16-CCITT
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "ax25_parser.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define CRC_POLYNOMIAL  0x8408  /* CRC-16-CCITT polynomial (reversed) */
#define CRC_INIT        0xFFFF  /* CRC initial value */
#define CRC_GOOD        0xF0B8  /* CRC residual for valid frame */

/* Private function prototypes -----------------------------------------------*/
static void AX25_ParseAddress(uint8_t *buffer, AX25_Address_TypeDef *address);
static void AX25_UpdateCRC(AX25_Parser_HandleTypeDef *hax25, uint8_t byte);
static bool AX25_CheckCRC(AX25_Parser_HandleTypeDef *hax25);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Parse AX.25 address field
  * @param  buffer: Pointer to 7-byte address buffer
  * @param  address: Pointer to address structure
  * @retval None
  */
static void AX25_ParseAddress(uint8_t *buffer, AX25_Address_TypeDef *address)
{
    /* AX.25 address format: each character is left-shifted by 1 bit */
    memset(address->callsign, 0, sizeof(address->callsign));
    
    for (int i = 0; i < 6; i++) {
        char c = buffer[i] >> 1;  /* Right shift 1 bit to decode */
        if (c != ' ') {  /* Remove padding spaces */
            address->callsign[i] = c;
        }
    }
    
    /* SSID is in bits 4-1 of the 7th byte */
    address->ssid = (buffer[6] >> 1) & 0x0F;
}

/**
  * @brief  Update CRC with new byte
  * @param  hax25: Pointer to AX.25 parser handle
  * @param  byte: Byte to add to CRC
  * @retval None
  */
static void AX25_UpdateCRC(AX25_Parser_HandleTypeDef *hax25, uint8_t byte)
{
    hax25->crc ^= byte;
    
    for (int i = 0; i < 8; i++) {
        if (hax25->crc & 0x0001) {
            hax25->crc = (hax25->crc >> 1) ^ CRC_POLYNOMIAL;
        } else {
            hax25->crc = hax25->crc >> 1;
        }
    }
}

/**
  * @brief  Check CRC validity
  * @param  hax25: Pointer to AX.25 parser handle
  * @retval true if CRC is valid, false otherwise
  */
static bool AX25_CheckCRC(AX25_Parser_HandleTypeDef *hax25)
{
    /* AX.25 uses CRC-16-CCITT */
    /* Correct frame should have CRC residual equal to CRC_GOOD */
    return (hax25->crc == CRC_GOOD);
}

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  Initialize AX.25 parser
  * @param  hax25: Pointer to AX.25 parser handle
  * @retval 0 if success, -1 otherwise
  */
int32_t AX25_Init(AX25_Parser_HandleTypeDef *hax25)
{
    if (hax25 == NULL) {
        return -1;
    }
    
    AX25_Reset(hax25);
    return 0;
}

/**
  * @brief  Reset AX.25 parser state
  * @param  hax25: Pointer to AX.25 parser handle
  * @retval None
  */
void AX25_Reset(AX25_Parser_HandleTypeDef *hax25)
{
    memset(&hax25->current_frame, 0, sizeof(AX25_Frame_TypeDef));
    memset(hax25->raw_buffer, 0, sizeof(hax25->raw_buffer));
    hax25->raw_buffer_pos = 0;
    hax25->crc = CRC_INIT;
}

/**
  * @brief  Start new frame
  * @param  hax25: Pointer to AX.25 parser handle
  * @retval None
  */
void AX25_StartFrame(AX25_Parser_HandleTypeDef *hax25)
{
    AX25_Reset(hax25);
}

/**
  * @brief  Add byte to current frame
  * @param  hax25: Pointer to AX.25 parser handle
  * @param  byte: Byte to add
  * @retval true if frame is complete, false otherwise
  */
bool AX25_AddByte(AX25_Parser_HandleTypeDef *hax25, uint8_t byte)
{
    /* Prevent buffer overflow */
    if (hax25->raw_buffer_pos >= AX25_MAX_FRAME_LEN) {
        return false;
    }
    
    /* Store byte and update CRC */
    hax25->raw_buffer[hax25->raw_buffer_pos++] = byte;
    AX25_UpdateCRC(hax25, byte);
    
    /* Check minimum frame length */
    if (hax25->raw_buffer_pos < AX25_MIN_FRAME_LEN) {
        return false;
    }
    
    /* Frame is not complete until endFrame is called */
    return false;
}

/**
  * @brief  End current frame and verify CRC
  * @param  hax25: Pointer to AX.25 parser handle
  * @retval true if frame is valid, false otherwise
  */
bool AX25_EndFrame(AX25_Parser_HandleTypeDef *hax25)
{
    /* Check frame length */
    if (hax25->raw_buffer_pos < AX25_MIN_FRAME_LEN) {
        hax25->current_frame.valid = false;
        return false;
    }
    
    /* Verify CRC */
    hax25->current_frame.valid = AX25_CheckCRC(hax25);
    if (!hax25->current_frame.valid) {
#if DEBUG_ENABLED
        DEBUG_PRINTF("AX.25 CRC Error\r\n");
#endif
        return false;
    }
    
    /* Parse address fields */
    uint16_t pos = 0;
    
    /* Destination address */
    AX25_ParseAddress(&hax25->raw_buffer[pos], &hax25->current_frame.destination);
    pos += AX25_ADDR_LEN;
    
    /* Source address */
    AX25_ParseAddress(&hax25->raw_buffer[pos], &hax25->current_frame.source);
    pos += AX25_ADDR_LEN;
    
    /* Parse digipeater path (check address extension bit) */
    hax25->current_frame.num_digipeaters = 0;
    while ((hax25->raw_buffer[pos - 1] & 0x01) == 0 && 
           hax25->current_frame.num_digipeaters < 8) {
        AX25_ParseAddress(&hax25->raw_buffer[pos], 
                         &hax25->current_frame.digipeaters[hax25->current_frame.num_digipeaters]);
        hax25->current_frame.num_digipeaters++;
        pos += AX25_ADDR_LEN;
        
        /* Prevent overflow */
        if (pos >= hax25->raw_buffer_pos - 2) {
            break;
        }
    }
    
    /* Control field */
    if (pos < hax25->raw_buffer_pos - 2) {
        hax25->current_frame.control = hax25->raw_buffer[pos++];
    }
    
    /* PID field */
    if (pos < hax25->raw_buffer_pos - 2) {
        hax25->current_frame.pid = hax25->raw_buffer[pos++];
    }
    
    /* Information field (remove last 2 bytes which are CRC) */
    hax25->current_frame.info_len = 0;
    while (pos < hax25->raw_buffer_pos - 2 && hax25->current_frame.info_len < 256) {
        hax25->current_frame.info[hax25->current_frame.info_len++] = 
            hax25->raw_buffer[pos++];
    }
    
    return true;
}

/**
  * @brief  Get parsed frame
  * @param  hax25: Pointer to AX.25 parser handle
  * @retval Pointer to parsed frame
  */
AX25_Frame_TypeDef* AX25_GetFrame(AX25_Parser_HandleTypeDef *hax25)
{
    return &hax25->current_frame;
}

