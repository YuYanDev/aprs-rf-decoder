/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body for APRS Decoder
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "aprs_config.h"
#include "sx127x.h"
#include "afsk_demod.h"
#include "nrzi_decoder.h"
#include "ax25_parser.h"
#include <stdio.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;  /* APRS output UART */
UART_HandleTypeDef huart2;  /* Debug UART */
TIM_HandleTypeDef htim2;    /* Sampling timer */

/* APRS Decoder handles */
SX127x_HandleTypeDef hsx127x;
AFSK_Demod_HandleTypeDef hafsk;
NRZI_Decoder_HandleTypeDef hnrzi;
AX25_Parser_HandleTypeDef hax25;

/* Statistics */
typedef struct {
    uint32_t frames_received;
    uint32_t frames_valid;
    uint32_t frames_crc_error;
    uint32_t bytes_received;
} Decoder_Stats_TypeDef;

Decoder_Stats_TypeDef decoder_stats = {0};

/* State machine */
typedef enum {
    STATE_IDLE,
    STATE_SYNC,
    STATE_RECEIVING,
    STATE_COMPLETE
} Decoder_State_TypeDef;

Decoder_State_TypeDef decoder_state = STATE_IDLE;
bool frame_available = false;
uint32_t last_stats_time = 0;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
static void APRS_ProcessSample(void);
static void APRS_PrintFrame(AX25_Frame_TypeDef *frame);
static void APRS_PrintStatistics(void);

/* Private user code ---------------------------------------------------------*/

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_TIM2_Init();

    /* Initialize APRS decoder components */
    printf("\r\n");
    printf("╔════════════════════════════════════════╗\r\n");
    printf("║   SX1276/SX1278 APRS Decoder v2.0    ║\r\n");
    printf("╚════════════════════════════════════════╝\r\n");
    printf("\r\n");

#if defined(STM32F401xE)
    printf("MCU: STM32F401 @ %lu MHz\r\n", HAL_RCC_GetHCLKFreq() / 1000000);
#elif defined(STM32F411xE)
    printf("MCU: STM32F411 @ %lu MHz\r\n", HAL_RCC_GetHCLKFreq() / 1000000);
#elif defined(STM32L412xx)
    printf("MCU: STM32L412 @ %lu MHz\r\n", HAL_RCC_GetHCLKFreq() / 1000000);
#else
    printf("MCU: STM32 @ %lu MHz\r\n", HAL_RCC_GetHCLKFreq() / 1000000);
#endif

#if HAS_FPU
    printf("FPU: Enabled\r\n");
#endif
#if USE_CMSIS_DSP
    printf("DSP: CMSIS-DSP Enabled\r\n");
#endif

    /* Initialize SX127x */
    printf("\r\nInitializing SX127x...\r\n");
    hsx127x.hspi = &hspi1;
    hsx127x.nss_port = SX127X_NSS_GPIO_PORT;
    hsx127x.nss_pin = SX127X_NSS_PIN;
    hsx127x.reset_port = SX127X_RESET_GPIO_PORT;
    hsx127x.reset_pin = SX127X_RESET_PIN;
    hsx127x.dio0_port = SX127X_DIO0_GPIO_PORT;
    hsx127x.dio0_pin = SX127X_DIO0_PIN;
    hsx127x.dio2_port = SX127X_DIO2_GPIO_PORT;
    hsx127x.dio2_pin = SX127X_DIO2_PIN;

    if (SX127x_Init(&hsx127x) != SX127X_OK) {
        printf("ERROR: SX127x not found!\r\n");
        Error_Handler();
    }
    printf("SX127x detected (v0x%02X)\r\n", hsx127x.chip_version);

    /* Configure for FSK mode */
    if (SX127x_BeginFSK(&hsx127x, RF_FREQUENCY, RF_BITRATE, RF_DEVIATION) != SX127X_OK) {
        printf("ERROR: SX127x configuration failed!\r\n");
        Error_Handler();
    }
    printf("Frequency: %.2f MHz\r\n", RF_FREQUENCY);
    printf("Bitrate: %.1f kbps\r\n", RF_BITRATE);

    /* Initialize AFSK demodulator */
    printf("\r\nInitializing AFSK demodulator...\r\n");
    if (AFSK_Init(&hafsk) != 0) {
        printf("ERROR: AFSK init failed!\r\n");
        Error_Handler();
    }
    printf("Sample rate: %d Hz\r\n", AFSK_SAMPLE_RATE);

    /* Initialize NRZI decoder */
    NRZI_Init(&hnrzi);

    /* Initialize AX.25 parser */
    AX25_Init(&hax25);

    /* Start receiving */
    SX127x_ReceiveDirect(&hsx127x);

    /* Start sampling timer */
    HAL_TIM_Base_Start_IT(&htim2);

    printf("\r\n========================================\r\n");
    printf("System ready! Listening for APRS...\r\n");
    printf("========================================\r\n\r\n");

    last_stats_time = HAL_GetTick();

    /* Infinite loop */
    while (1)
    {
        /* Check if frame is available */
        if (frame_available) {
            AX25_Frame_TypeDef *frame = AX25_GetFrame(&hax25);
            
            if (frame != NULL && frame->valid) {
                APRS_PrintFrame(frame);
            }
            
            frame_available = false;
            decoder_state = STATE_IDLE;
        }

        /* Print statistics every 10 seconds */
        if (HAL_GetTick() - last_stats_time >= 10000) {
            APRS_PrintStatistics();
            last_stats_time = HAL_GetTick();
        }

        /* Small delay to prevent CPU hogging */
        HAL_Delay(1);
    }
}

/**
  * @brief  Timer period elapsed callback (sampling timer)
  * @param  htim: TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        APRS_ProcessSample();
    }
}

/**
  * @brief  Process one sample from SX127x DIO2 pin
  * @retval None
  */
static void APRS_ProcessSample(void)
{
    static uint16_t sync_timeout = 0;
    static uint16_t byte_timeout = 0;
    static uint8_t flag_count = 0;

    /* Read sample from DIO2 */
    uint8_t sample = SX127x_ReadDIO2(&hsx127x);

    /* AFSK demodulation */
    if (AFSK_ProcessSample(&hafsk, sample)) {
        uint8_t bit = AFSK_GetBit(&hafsk);

        /* NRZI decoding */
        if (NRZI_ProcessBit(&hnrzi, bit)) {
            uint8_t byte = NRZI_GetByte(&hnrzi);

            /* State machine */
            switch (decoder_state) {
                case STATE_IDLE:
                case STATE_SYNC:
                    if (NRZI_IsFlagDetected(&hnrzi)) {
                        flag_count++;
                        if (flag_count >= 1) {
                            decoder_state = STATE_RECEIVING;
                            AX25_StartFrame(&hax25);
                            byte_timeout = 0;
                        }
                    }
                    break;

                case STATE_RECEIVING:
                    if (NRZI_IsFlagDetected(&hnrzi)) {
                        /* End of frame */
                        if (AX25_EndFrame(&hax25)) {
                            decoder_stats.frames_received++;
                            decoder_stats.frames_valid++;
                            frame_available = true;
                            decoder_state = STATE_COMPLETE;
                        } else {
                            decoder_stats.frames_received++;
                            decoder_stats.frames_crc_error++;
                            decoder_state = STATE_IDLE;
                            flag_count = 0;
                        }
                    } else {
                        /* Normal data byte */
                        AX25_AddByte(&hax25, byte);
                        decoder_stats.bytes_received++;
                        byte_timeout = 0;
                    }
                    break;

                case STATE_COMPLETE:
                    /* Wait for frame to be processed */
                    if (NRZI_IsFlagDetected(&hnrzi)) {
                        if (!frame_available) {
                            decoder_state = STATE_SYNC;
                            flag_count = 1;
                        }
                    }
                    break;
            }

            /* Timeout handling */
            byte_timeout++;
            if (decoder_state == STATE_RECEIVING && byte_timeout > 20 * SAMPLES_PER_BIT) {
                decoder_state = STATE_IDLE;
                flag_count = 0;
            }
        }
    }

    /* Carrier detection */
    if (decoder_state == STATE_IDLE) {
        if (AFSK_IsCarrierDetected(&hafsk)) {
            decoder_state = STATE_SYNC;
            sync_timeout = 0;
            flag_count = 0;
            NRZI_Reset(&hnrzi);
        }
    }
}

/**
  * @brief  Print received APRS frame
  * @param  frame: Pointer to AX.25 frame
  * @retval None
  */
static void APRS_PrintFrame(AX25_Frame_TypeDef *frame)
{
    char callsign[16];

    printf("\r\n");
    printf("╔════════════════════════════════════════╗\r\n");
    printf("║       APRS Frame Received!           ║\r\n");
    printf("╚════════════════════════════════════════╝\r\n");

    /* Source callsign */
    printf("From: ");
    memset(callsign, 0, sizeof(callsign));
    strncpy(callsign, frame->source.callsign, 6);
    printf("%s", callsign);
    if (frame->source.ssid > 0) {
        printf("-%d", frame->source.ssid);
    }
    printf("\r\n");

    /* Destination callsign */
    printf("To: ");
    memset(callsign, 0, sizeof(callsign));
    strncpy(callsign, frame->destination.callsign, 6);
    printf("%s", callsign);
    if (frame->destination.ssid > 0) {
        printf("-%d", frame->destination.ssid);
    }
    printf("\r\n");

    /* Information field */
    printf("Info: ");
    for (uint16_t i = 0; i < frame->info_len; i++) {
        printf("%c", frame->info[i]);
    }
    printf("\r\n");

    /* Signal quality */
    printf("Quality: %d%%\r\n", AFSK_GetSignalQuality(&hafsk));
    printf("----------------------------------------\r\n\r\n");

    /* Send to APRS UART */
    char aprs_str[512];
    int pos = 0;
    
    /* Format: SOURCE>DEST:INFO */
    strncpy(callsign, frame->source.callsign, 6);
    callsign[6] = '\0';
    pos += sprintf(aprs_str + pos, "%s", callsign);
    if (frame->source.ssid > 0) {
        pos += sprintf(aprs_str + pos, "-%d", frame->source.ssid);
    }
    pos += sprintf(aprs_str + pos, ">");
    
    strncpy(callsign, frame->destination.callsign, 6);
    callsign[6] = '\0';
    pos += sprintf(aprs_str + pos, "%s", callsign);
    if (frame->destination.ssid > 0) {
        pos += sprintf(aprs_str + pos, "-%d", frame->destination.ssid);
    }
    pos += sprintf(aprs_str + pos, ":");
    
    for (uint16_t i = 0; i < frame->info_len && pos < 500; i++) {
        aprs_str[pos++] = frame->info[i];
    }
    aprs_str[pos++] = '\r';
    aprs_str[pos++] = '\n';
    
    HAL_UART_Transmit(&huart1, (uint8_t*)aprs_str, pos, 1000);
}

/**
  * @brief  Print statistics
  * @retval None
  */
static void APRS_PrintStatistics(void)
{
    printf("\r\n┌─── Statistics ─────────────────────┐\r\n");
    printf("│ Frames Received: %lu\r\n", decoder_stats.frames_received);
    printf("│ Valid Frames: %lu\r\n", decoder_stats.frames_valid);
    printf("│ CRC Errors: %lu\r\n", decoder_stats.frames_crc_error);
    printf("│ Bytes Received: %lu\r\n", decoder_stats.bytes_received);
    printf("└────────────────────────────────────┘\r\n\r\n");
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    /* This function should be generated by STM32CubeMX */
    /* Configure for maximum speed based on MCU type */
    /* STM32F401: 84 MHz */
    /* STM32F411: 100 MHz */
    /* STM32L412: 80 MHz */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure SX127x GPIO pins */
    /* NSS Pin */
    HAL_GPIO_WritePin(SX127X_NSS_GPIO_PORT, SX127X_NSS_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = SX127X_NSS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SX127X_NSS_GPIO_PORT, &GPIO_InitStruct);

    /* RESET Pin */
    GPIO_InitStruct.Pin = SX127X_RESET_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SX127X_RESET_GPIO_PORT, &GPIO_InitStruct);

    /* DIO0 and DIO2 as inputs */
    GPIO_InitStruct.Pin = SX127X_DIO0_PIN | SX127X_DIO2_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SX127X_DIO0_GPIO_PORT, &GPIO_InitStruct);
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;  /* ~5 MHz */
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief USART1 Initialization Function (APRS output)
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = UART_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief USART2 Initialization Function (Debug)
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = DEBUG_BAUDRATE;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief TIM2 Initialization Function (Sampling timer at 26.4 kHz)
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
    /* Configure timer for 26.4 kHz interrupt rate */
    /* Assuming system clock is 84 MHz (F401) */
    uint32_t timer_clock = HAL_RCC_GetPCLK1Freq() * 2;  /* APB1 timer clock */
    uint32_t prescaler = 0;  /* No prescaler */
    uint32_t period = (timer_clock / AFSK_SAMPLE_RATE) - 1;

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = prescaler;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = period;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  ch: character to send
  * @param  f: file pointer
  * @retval character sent
  */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        /* Blink LED or hang */
    }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */

