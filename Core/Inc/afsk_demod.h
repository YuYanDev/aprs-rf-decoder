/**
  ******************************************************************************
  * @file    afsk_demod.h
  * @brief   AFSK Demodulator Header (Bell 202 Standard)
  ******************************************************************************
  * @attention
  *
  * This module implements AFSK demodulation using Goertzel algorithm
  * Optimized with CMSIS-DSP for ARM Cortex-M4F
  *
  ******************************************************************************
  */

#ifndef __AFSK_DEMOD_H
#define __AFSK_DEMOD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "aprs_config.h"
#include <stdint.h>
#include <stdbool.h>

#if USE_CMSIS_DSP
#include "arm_math.h"
#endif

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  AFSK Demodulator Handle Structure
  */
typedef struct {
    /* Goertzel algorithm coefficients */
    float32_t mark_coeff;
    float32_t space_coeff;
    
    /* Goertzel filter states */
    float32_t mark_q1, mark_q2;
    float32_t space_q1, space_q2;
    
    /* Sample counter */
    uint8_t sample_counter;
    
    /* Bit decision */
    uint8_t current_bit;
    bool bit_ready;
    
    /* PLL for bit synchronization */
    int16_t pll_phase;
    int16_t pll_dphase;
    
    /* Signal detection */
    uint16_t mark_energy;
    uint16_t space_energy;
    uint16_t total_energy;
    
    /* Carrier detection */
    bool carrier_detected;
    uint8_t carrier_lock_count;
    
#if USE_CMSIS_DSP
    /* CMSIS-DSP FIR filters for bandpass filtering */
    arm_fir_instance_f32 fir_mark;
    arm_fir_instance_f32 fir_space;
    float32_t fir_mark_state[64];
    float32_t fir_space_state[64];
    float32_t fir_mark_coeffs[32];
    float32_t fir_space_coeffs[32];
    
    /* Sample buffers for Goertzel */
    float32_t mark_buffer[SAMPLES_PER_BIT];
    float32_t space_buffer[SAMPLES_PER_BIT];
    uint8_t buffer_index;
#endif
} AFSK_Demod_HandleTypeDef;

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize AFSK demodulator
  * @param  hafsk: Pointer to AFSK demodulator handle
  * @retval 0 if success, -1 otherwise
  */
int32_t AFSK_Init(AFSK_Demod_HandleTypeDef *hafsk);

/**
  * @brief  Reset AFSK demodulator state
  * @param  hafsk: Pointer to AFSK demodulator handle
  * @retval None
  */
void AFSK_Reset(AFSK_Demod_HandleTypeDef *hafsk);

/**
  * @brief  Process a single sample
  * @param  hafsk: Pointer to AFSK demodulator handle
  * @param  sample: Input sample (0 or 1)
  * @retval true if a bit is ready, false otherwise
  */
bool AFSK_ProcessSample(AFSK_Demod_HandleTypeDef *hafsk, uint8_t sample);

/**
  * @brief  Get demodulated bit
  * @param  hafsk: Pointer to AFSK demodulator handle
  * @retval Demodulated bit value (0 or 1)
  */
uint8_t AFSK_GetBit(AFSK_Demod_HandleTypeDef *hafsk);

/**
  * @brief  Get signal quality
  * @param  hafsk: Pointer to AFSK demodulator handle
  * @retval Signal quality (0-100)
  */
uint8_t AFSK_GetSignalQuality(AFSK_Demod_HandleTypeDef *hafsk);

/**
  * @brief  Check if carrier is detected
  * @param  hafsk: Pointer to AFSK demodulator handle
  * @retval true if carrier detected, false otherwise
  */
bool AFSK_IsCarrierDetected(AFSK_Demod_HandleTypeDef *hafsk);

#ifdef __cplusplus
}
#endif

#endif /* __AFSK_DEMOD_H */

