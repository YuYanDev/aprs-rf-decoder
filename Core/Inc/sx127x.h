/**
  ******************************************************************************
  * @file    sx127x.h
  * @brief   SX1276/SX1278 LoRa/FSK Transceiver Driver
  ******************************************************************************
  * @attention
  *
  * This driver supports SX1276/SX1277/SX1278/SX1279 in FSK mode for APRS
  *
  ******************************************************************************
  */

#ifndef __SX127X_H
#define __SX127X_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* SX127x Register Addresses -------------------------------------------------*/
#define SX127X_REG_FIFO                 0x00
#define SX127X_REG_OP_MODE              0x01
#define SX127X_REG_BITRATE_MSB          0x02
#define SX127X_REG_BITRATE_LSB          0x03
#define SX127X_REG_FDEV_MSB             0x04
#define SX127X_REG_FDEV_LSB             0x05
#define SX127X_REG_FRF_MSB              0x06
#define SX127X_REG_FRF_MID              0x07
#define SX127X_REG_FRF_LSB              0x08
#define SX127X_REG_PA_CONFIG            0x09
#define SX127X_REG_PA_RAMP              0x0A
#define SX127X_REG_OCP                  0x0B
#define SX127X_REG_LNA                  0x0C
#define SX127X_REG_RX_CONFIG            0x0D
#define SX127X_REG_RSSI_CONFIG          0x0E
#define SX127X_REG_RSSI_VALUE           0x11
#define SX127X_REG_RX_BW                0x12
#define SX127X_REG_AFC_BW               0x13
#define SX127X_REG_OOK_PEAK             0x14
#define SX127X_REG_OOK_FIX              0x15
#define SX127X_REG_OOK_AVG              0x16
#define SX127X_REG_AFC_FEI              0x1A
#define SX127X_REG_AFC_MSB              0x1B
#define SX127X_REG_AFC_LSB              0x1C
#define SX127X_REG_FEI_MSB              0x1D
#define SX127X_REG_FEI_LSB              0x1E
#define SX127X_REG_PREAMBLE_DETECT      0x1F
#define SX127X_REG_RX_TIMEOUT_1         0x20
#define SX127X_REG_RX_TIMEOUT_2         0x21
#define SX127X_REG_RX_TIMEOUT_3         0x22
#define SX127X_REG_RX_DELAY             0x23
#define SX127X_REG_OSC                  0x24
#define SX127X_REG_PREAMBLE_MSB         0x25
#define SX127X_REG_PREAMBLE_LSB         0x26
#define SX127X_REG_SYNC_CONFIG          0x27
#define SX127X_REG_SYNC_VALUE_1         0x28
#define SX127X_REG_PACKET_CONFIG_1      0x30
#define SX127X_REG_PACKET_CONFIG_2      0x31
#define SX127X_REG_PAYLOAD_LENGTH       0x32
#define SX127X_REG_FIFO_THRESH          0x35
#define SX127X_REG_SEQ_CONFIG_1         0x36
#define SX127X_REG_SEQ_CONFIG_2         0x37
#define SX127X_REG_TIMER_RESOL          0x38
#define SX127X_REG_TIMER1_COEF          0x39
#define SX127X_REG_TIMER2_COEF          0x3A
#define SX127X_REG_IMAGE_CAL            0x3B
#define SX127X_REG_TEMP                 0x3C
#define SX127X_REG_LOW_BAT              0x3D
#define SX127X_REG_IRQ_FLAGS_1          0x3E
#define SX127X_REG_IRQ_FLAGS_2          0x3F
#define SX127X_REG_DIO_MAPPING_1        0x40
#define SX127X_REG_DIO_MAPPING_2        0x41
#define SX127X_REG_VERSION              0x42
#define SX127X_REG_TCXO                 0x4B
#define SX127X_REG_PA_DAC               0x4D
#define SX127X_REG_AGC_REF              0x61
#define SX127X_REG_AGC_THRESH_1         0x62
#define SX127X_REG_AGC_THRESH_2         0x63
#define SX127X_REG_AGC_THRESH_3         0x64
#define SX127X_REG_PLL                  0x70

/* Operating Modes -----------------------------------------------------------*/
#define SX127X_MODE_SLEEP               0x00
#define SX127X_MODE_STDBY               0x01
#define SX127X_MODE_FSTX                0x02
#define SX127X_MODE_TX                  0x03
#define SX127X_MODE_FSRX                0x04
#define SX127X_MODE_RX                  0x05

/* Modulation Types ----------------------------------------------------------*/
#define SX127X_MODULATION_FSK           0x00
#define SX127X_MODULATION_OOK           0x01

/* DIO Mapping ---------------------------------------------------------------*/
#define SX127X_DIO0_RX_DONE             0x00
#define SX127X_DIO0_TX_DONE             0x00
#define SX127X_DIO0_CAD_DONE            0x00
#define SX127X_DIO2_DATA                0x00  /* Direct mode data output */

/* Error Codes ---------------------------------------------------------------*/
#define SX127X_OK                       0
#define SX127X_ERR_CHIP_NOT_FOUND       -1
#define SX127X_ERR_INVALID_FREQUENCY    -2
#define SX127X_ERR_INVALID_BITRATE      -3
#define SX127X_ERR_INVALID_BANDWIDTH    -4
#define SX127X_ERR_SPI                  -5

/* Crystal Frequency ---------------------------------------------------------*/
#define SX127X_FXOSC                    32000000.0f  /* 32 MHz crystal */
#define SX127X_FSTEP                    (SX127X_FXOSC / 524288.0f)  /* 61.03515625 Hz */

/* Structures ----------------------------------------------------------------*/

/**
  * @brief  SX127x Handle Structure
  */
typedef struct {
    SPI_HandleTypeDef *hspi;          /* SPI handle */
    GPIO_TypeDef *nss_port;           /* NSS GPIO port */
    uint16_t nss_pin;                 /* NSS GPIO pin */
    GPIO_TypeDef *reset_port;         /* RESET GPIO port */
    uint16_t reset_pin;               /* RESET GPIO pin */
    GPIO_TypeDef *dio0_port;          /* DIO0 GPIO port */
    uint16_t dio0_pin;                /* DIO0 GPIO pin */
    GPIO_TypeDef *dio2_port;          /* DIO2 GPIO port */
    uint16_t dio2_pin;                /* DIO2 GPIO pin */
    float frequency;                  /* Operating frequency in MHz */
    float bitrate;                    /* FSK bitrate in kbps */
    float fdev;                       /* FSK frequency deviation in kHz */
    uint8_t chip_version;             /* Chip version */
} SX127x_HandleTypeDef;

/* Function Prototypes -------------------------------------------------------*/

/**
  * @brief  Initialize SX127x transceiver
  * @param  hsx127x: Pointer to SX127x handle
  * @retval SX127X_OK if success, error code otherwise
  */
int32_t SX127x_Init(SX127x_HandleTypeDef *hsx127x);

/**
  * @brief  Reset SX127x chip
  * @param  hsx127x: Pointer to SX127x handle
  * @retval None
  */
void SX127x_Reset(SX127x_HandleTypeDef *hsx127x);

/**
  * @brief  Configure SX127x for FSK mode
  * @param  hsx127x: Pointer to SX127x handle
  * @param  frequency: Frequency in MHz
  * @param  bitrate: Bitrate in kbps
  * @param  fdev: Frequency deviation in kHz
  * @retval SX127X_OK if success, error code otherwise
  */
int32_t SX127x_BeginFSK(SX127x_HandleTypeDef *hsx127x, float frequency, 
                        float bitrate, float fdev);

/**
  * @brief  Start receiving in direct mode
  * @param  hsx127x: Pointer to SX127x handle
  * @retval SX127X_OK if success, error code otherwise
  */
int32_t SX127x_ReceiveDirect(SX127x_HandleTypeDef *hsx127x);

/**
  * @brief  Set operating mode
  * @param  hsx127x: Pointer to SX127x handle
  * @param  mode: Operating mode
  * @retval SX127X_OK if success, error code otherwise
  */
int32_t SX127x_SetMode(SX127x_HandleTypeDef *hsx127x, uint8_t mode);

/**
  * @brief  Read DIO2 pin state (for direct mode sampling)
  * @param  hsx127x: Pointer to SX127x handle
  * @retval Pin state (0 or 1)
  */
uint8_t SX127x_ReadDIO2(SX127x_HandleTypeDef *hsx127x);

/**
  * @brief  Read RSSI value
  * @param  hsx127x: Pointer to SX127x handle
  * @retval RSSI in dBm
  */
int16_t SX127x_GetRSSI(SX127x_HandleTypeDef *hsx127x);

/**
  * @brief  Write to SX127x register
  * @param  hsx127x: Pointer to SX127x handle
  * @param  reg: Register address
  * @param  value: Value to write
  * @retval SX127X_OK if success, error code otherwise
  */
int32_t SX127x_WriteRegister(SX127x_HandleTypeDef *hsx127x, uint8_t reg, uint8_t value);

/**
  * @brief  Read from SX127x register
  * @param  hsx127x: Pointer to SX127x handle
  * @param  reg: Register address
  * @retval Register value
  */
uint8_t SX127x_ReadRegister(SX127x_HandleTypeDef *hsx127x, uint8_t reg);

/**
  * @brief  Burst write to SX127x registers
  * @param  hsx127x: Pointer to SX127x handle
  * @param  reg: Starting register address
  * @param  data: Pointer to data buffer
  * @param  length: Number of bytes to write
  * @retval SX127X_OK if success, error code otherwise
  */
int32_t SX127x_WriteRegisterBurst(SX127x_HandleTypeDef *hsx127x, uint8_t reg, 
                                   uint8_t *data, uint16_t length);

/**
  * @brief  Burst read from SX127x registers
  * @param  hsx127x: Pointer to SX127x handle
  * @param  reg: Starting register address
  * @param  data: Pointer to data buffer
  * @param  length: Number of bytes to read
  * @retval SX127X_OK if success, error code otherwise
  */
int32_t SX127x_ReadRegisterBurst(SX127x_HandleTypeDef *hsx127x, uint8_t reg, 
                                  uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif /* __SX127X_H */

