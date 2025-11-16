/**
  ******************************************************************************
  * @file    sx127x.c
  * @brief   SX1276/SX1278 LoRa/FSK Transceiver Driver Implementation
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "sx127x.h"
#include <math.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define SX127X_SPI_TIMEOUT      1000    /* SPI timeout in ms */
#define SX127X_WRITE_CMD        0x80    /* Write command bit */
#define SX127X_READ_CMD         0x00    /* Read command bit */

/* Private function prototypes -----------------------------------------------*/
static void SX127x_ChipSelect(SX127x_HandleTypeDef *hsx127x, bool select);
static int32_t SX127x_SPITransfer(SX127x_HandleTypeDef *hsx127x, uint8_t *tx_data, 
                                   uint8_t *rx_data, uint16_t length);

/**
  * @brief  Reset SX127x chip
  * @param  hsx127x: Pointer to SX127x handle
  * @retval None
  */
void SX127x_Reset(SX127x_HandleTypeDef *hsx127x)
{
    /* Reset pulse: LOW for 1ms */
    HAL_GPIO_WritePin(hsx127x->reset_port, hsx127x->reset_pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(hsx127x->reset_port, hsx127x->reset_pin, GPIO_PIN_SET);
    HAL_Delay(10);  /* Wait for chip to boot */
}

/**
  * @brief  Select/Deselect SX127x chip via NSS
  * @param  hsx127x: Pointer to SX127x handle
  * @param  select: true to select, false to deselect
  * @retval None
  */
static void SX127x_ChipSelect(SX127x_HandleTypeDef *hsx127x, bool select)
{
    if (select) {
        HAL_GPIO_WritePin(hsx127x->nss_port, hsx127x->nss_pin, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(hsx127x->nss_port, hsx127x->nss_pin, GPIO_PIN_SET);
    }
}

/**
  * @brief  SPI transfer function
  * @param  hsx127x: Pointer to SX127x handle
  * @param  tx_data: Pointer to transmit buffer
  * @param  rx_data: Pointer to receive buffer (can be NULL)
  * @param  length: Number of bytes to transfer
  * @retval SX127X_OK if success, SX127X_ERR_SPI otherwise
  */
static int32_t SX127x_SPITransfer(SX127x_HandleTypeDef *hsx127x, uint8_t *tx_data, 
                                   uint8_t *rx_data, uint16_t length)
{
    HAL_StatusTypeDef status;
    
    if (rx_data != NULL) {
        status = HAL_SPI_TransmitReceive(hsx127x->hspi, tx_data, rx_data, 
                                         length, SX127X_SPI_TIMEOUT);
    } else {
        status = HAL_SPI_Transmit(hsx127x->hspi, tx_data, length, SX127X_SPI_TIMEOUT);
    }
    
    return (status == HAL_OK) ? SX127X_OK : SX127X_ERR_SPI;
}

/**
  * @brief  Write to SX127x register
  * @param  hsx127x: Pointer to SX127x handle
  * @param  reg: Register address
  * @param  value: Value to write
  * @retval SX127X_OK if success, error code otherwise
  */
int32_t SX127x_WriteRegister(SX127x_HandleTypeDef *hsx127x, uint8_t reg, uint8_t value)
{
    uint8_t tx_data[2];
    int32_t result;
    
    tx_data[0] = reg | SX127X_WRITE_CMD;
    tx_data[1] = value;
    
    SX127x_ChipSelect(hsx127x, true);
    result = SX127x_SPITransfer(hsx127x, tx_data, NULL, 2);
    SX127x_ChipSelect(hsx127x, false);
    
    return result;
}

/**
  * @brief  Read from SX127x register
  * @param  hsx127x: Pointer to SX127x handle
  * @param  reg: Register address
  * @retval Register value
  */
uint8_t SX127x_ReadRegister(SX127x_HandleTypeDef *hsx127x, uint8_t reg)
{
    uint8_t tx_data[2] = {0};
    uint8_t rx_data[2] = {0};
    
    tx_data[0] = reg & ~SX127X_WRITE_CMD;
    
    SX127x_ChipSelect(hsx127x, true);
    SX127x_SPITransfer(hsx127x, tx_data, rx_data, 2);
    SX127x_ChipSelect(hsx127x, false);
    
    return rx_data[1];
}

/**
  * @brief  Burst write to SX127x registers
  * @param  hsx127x: Pointer to SX127x handle
  * @param  reg: Starting register address
  * @param  data: Pointer to data buffer
  * @param  length: Number of bytes to write
  * @retval SX127X_OK if success, error code otherwise
  */
int32_t SX127x_WriteRegisterBurst(SX127x_HandleTypeDef *hsx127x, uint8_t reg, 
                                   uint8_t *data, uint16_t length)
{
    uint8_t tx_buffer[256];
    int32_t result;
    
    if (length > 255) {
        return SX127X_ERR_SPI;
    }
    
    tx_buffer[0] = reg | SX127X_WRITE_CMD;
    memcpy(&tx_buffer[1], data, length);
    
    SX127x_ChipSelect(hsx127x, true);
    result = SX127x_SPITransfer(hsx127x, tx_buffer, NULL, length + 1);
    SX127x_ChipSelect(hsx127x, false);
    
    return result;
}

/**
  * @brief  Burst read from SX127x registers
  * @param  hsx127x: Pointer to SX127x handle
  * @param  reg: Starting register address
  * @param  data: Pointer to data buffer
  * @param  length: Number of bytes to read
  * @retval SX127X_OK if success, error code otherwise
  */
int32_t SX127x_ReadRegisterBurst(SX127x_HandleTypeDef *hsx127x, uint8_t reg, 
                                  uint8_t *data, uint16_t length)
{
    uint8_t tx_buffer[256] = {0};
    uint8_t rx_buffer[256] = {0};
    int32_t result;
    
    if (length > 255) {
        return SX127X_ERR_SPI;
    }
    
    tx_buffer[0] = reg & ~SX127X_WRITE_CMD;
    
    SX127x_ChipSelect(hsx127x, true);
    result = SX127x_SPITransfer(hsx127x, tx_buffer, rx_buffer, length + 1);
    SX127x_ChipSelect(hsx127x, false);
    
    if (result == SX127X_OK) {
        memcpy(data, &rx_buffer[1], length);
    }
    
    return result;
}

/**
  * @brief  Set operating mode
  * @param  hsx127x: Pointer to SX127x handle
  * @param  mode: Operating mode
  * @retval SX127X_OK if success, error code otherwise
  */
int32_t SX127x_SetMode(SX127x_HandleTypeDef *hsx127x, uint8_t mode)
{
    uint8_t reg_value;
    
    /* Read current OpMode register */
    reg_value = SX127x_ReadRegister(hsx127x, SX127X_REG_OP_MODE);
    
    /* Clear mode bits and set new mode */
    reg_value = (reg_value & 0xF8) | (mode & 0x07);
    
    /* Enable FSK mode (not LoRa) */
    reg_value &= ~0x80;  /* Clear LoRa mode bit */
    
    return SX127x_WriteRegister(hsx127x, SX127X_REG_OP_MODE, reg_value);
}

/**
  * @brief  Initialize SX127x transceiver
  * @param  hsx127x: Pointer to SX127x handle
  * @retval SX127X_OK if success, error code otherwise
  */
int32_t SX127x_Init(SX127x_HandleTypeDef *hsx127x)
{
    /* Reset the chip */
    SX127x_Reset(hsx127x);
    
    /* Read chip version */
    hsx127x->chip_version = SX127x_ReadRegister(hsx127x, SX127X_REG_VERSION);
    
    /* Check if chip is responding (typical version: 0x12) */
    if (hsx127x->chip_version == 0x00 || hsx127x->chip_version == 0xFF) {
        return SX127X_ERR_CHIP_NOT_FOUND;
    }
    
    /* Put chip in sleep mode */
    SX127x_SetMode(hsx127x, SX127X_MODE_SLEEP);
    HAL_Delay(10);
    
    /* Set FSK mode */
    uint8_t reg_value = SX127x_ReadRegister(hsx127x, SX127X_REG_OP_MODE);
    reg_value &= ~0x80;  /* Clear LoRa mode bit */
    SX127x_WriteRegister(hsx127x, SX127X_REG_OP_MODE, reg_value);
    
    return SX127X_OK;
}

/**
  * @brief  Configure SX127x for FSK mode
  * @param  hsx127x: Pointer to SX127x handle
  * @param  frequency: Frequency in MHz
  * @param  bitrate: Bitrate in kbps
  * @param  fdev: Frequency deviation in kHz
  * @retval SX127X_OK if success, error code otherwise
  */
int32_t SX127x_BeginFSK(SX127x_HandleTypeDef *hsx127x, float frequency, 
                        float bitrate, float fdev)
{
    /* Store parameters */
    hsx127x->frequency = frequency;
    hsx127x->bitrate = bitrate;
    hsx127x->fdev = fdev;
    
    /* Set sleep mode */
    SX127x_SetMode(hsx127x, SX127X_MODE_SLEEP);
    HAL_Delay(10);
    
    /* Set frequency */
    uint32_t frf = (uint32_t)((frequency * 1000000.0f) / SX127X_FSTEP);
    SX127x_WriteRegister(hsx127x, SX127X_REG_FRF_MSB, (uint8_t)((frf >> 16) & 0xFF));
    SX127x_WriteRegister(hsx127x, SX127X_REG_FRF_MID, (uint8_t)((frf >> 8) & 0xFF));
    SX127x_WriteRegister(hsx127x, SX127X_REG_FRF_LSB, (uint8_t)(frf & 0xFF));
    
    /* Set bitrate */
    uint16_t bitrate_reg = (uint16_t)(SX127X_FXOSC / (bitrate * 1000.0f));
    SX127x_WriteRegister(hsx127x, SX127X_REG_BITRATE_MSB, (uint8_t)((bitrate_reg >> 8) & 0xFF));
    SX127x_WriteRegister(hsx127x, SX127X_REG_BITRATE_LSB, (uint8_t)(bitrate_reg & 0xFF));
    
    /* Set frequency deviation */
    uint16_t fdev_reg = (uint16_t)((fdev * 1000.0f) / SX127X_FSTEP);
    SX127x_WriteRegister(hsx127x, SX127X_REG_FDEV_MSB, (uint8_t)((fdev_reg >> 8) & 0xFF));
    SX127x_WriteRegister(hsx127x, SX127X_REG_FDEV_LSB, (uint8_t)(fdev_reg & 0xFF));
    
    /* Set RX bandwidth (150 kHz for APRS) */
    SX127x_WriteRegister(hsx127x, SX127X_REG_RX_BW, 0x02);  /* 250 kHz */
    SX127x_WriteRegister(hsx127x, SX127X_REG_AFC_BW, 0x02);
    
    /* Enable AFC */
    SX127x_WriteRegister(hsx127x, SX127X_REG_AFC_FEI, 0x10);
    
    /* Set LNA gain to maximum */
    SX127x_WriteRegister(hsx127x, SX127X_REG_LNA, 0x23);
    
    /* Set preamble detector */
    SX127x_WriteRegister(hsx127x, SX127X_REG_PREAMBLE_DETECT, 0xAA);
    
    /* Set continuous mode without bit synchronizer for direct mode */
    SX127x_WriteRegister(hsx127x, SX127X_REG_PACKET_CONFIG_1, 0x00);
    SX127x_WriteRegister(hsx127x, SX127X_REG_PACKET_CONFIG_2, 0x00);
    
    /* Configure DIO pins */
    /* DIO2 = Data (continuous mode) */
    uint8_t dio_map = SX127x_ReadRegister(hsx127x, SX127X_REG_DIO_MAPPING_1);
    dio_map = (dio_map & 0x3F) | 0x00;  /* DIO2 = 00 (Data) */
    SX127x_WriteRegister(hsx127x, SX127X_REG_DIO_MAPPING_1, dio_map);
    
    return SX127X_OK;
}

/**
  * @brief  Start receiving in direct mode
  * @param  hsx127x: Pointer to SX127x handle
  * @retval SX127X_OK if success, error code otherwise
  */
int32_t SX127x_ReceiveDirect(SX127x_HandleTypeDef *hsx127x)
{
    /* Set continuous mode without bit synchronizer */
    SX127x_WriteRegister(hsx127x, SX127X_REG_PACKET_CONFIG_2, 0x40);
    
    /* Enter RX mode */
    return SX127x_SetMode(hsx127x, SX127X_MODE_RX);
}

/**
  * @brief  Read DIO2 pin state (for direct mode sampling)
  * @param  hsx127x: Pointer to SX127x handle
  * @retval Pin state (0 or 1)
  */
uint8_t SX127x_ReadDIO2(SX127x_HandleTypeDef *hsx127x)
{
    return (HAL_GPIO_ReadPin(hsx127x->dio2_port, hsx127x->dio2_pin) == GPIO_PIN_SET) ? 1 : 0;
}

/**
  * @brief  Read RSSI value
  * @param  hsx127x: Pointer to SX127x handle
  * @retval RSSI in dBm
  */
int16_t SX127x_GetRSSI(SX127x_HandleTypeDef *hsx127x)
{
    uint8_t rssi_reg = SX127x_ReadRegister(hsx127x, SX127X_REG_RSSI_VALUE);
    return -((int16_t)rssi_reg / 2);  /* RSSI = -RssiValue/2 dBm */
}

