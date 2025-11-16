/**
  ******************************************************************************
  * @file    afsk_demod.c
  * @brief   AFSK Demodulator Implementation (Bell 202 Standard)
  ******************************************************************************
  * @attention
  *
  * This module implements AFSK demodulation using Goertzel algorithm
  * Optimized with CMSIS-DSP for ARM Cortex-M4F
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "afsk_demod.h"
#include <math.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#ifndef PI
#define PI 3.14159265358979323846f
#endif

/* Private function prototypes -----------------------------------------------*/
static void AFSK_CalculateCoefficients(AFSK_Demod_HandleTypeDef *hafsk);
static void AFSK_GoertzelUpdate(float32_t sample, float32_t coeff, 
                                 float32_t *q1, float32_t *q2);
static float32_t AFSK_GoertzelMagnitude(float32_t q1, float32_t q2, float32_t coeff);
static void AFSK_PLLUpdate(AFSK_Demod_HandleTypeDef *hafsk, bool transition);

#if USE_CMSIS_DSP
static void AFSK_InitFIRFilters(AFSK_Demod_HandleTypeDef *hafsk);
static void AFSK_DesignBandpassFilter(float32_t *coeffs, uint16_t num_taps,
                                       float32_t center_freq, float32_t bandwidth);
static float32_t AFSK_GoertzelCMSIS(float32_t *buffer, uint16_t freq);
#endif

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Calculate Goertzel algorithm coefficients
  * @param  hafsk: Pointer to AFSK demodulator handle
  * @retval None
  */
static void AFSK_CalculateCoefficients(AFSK_Demod_HandleTypeDef *hafsk)
{
    /* Goertzel coefficient: coeff = 2 * cos(2π * freq / sampleRate) */
    float32_t omega_mark = (2.0f * PI * AFSK_MARK_FREQ) / AFSK_SAMPLE_RATE;
    float32_t omega_space = (2.0f * PI * AFSK_SPACE_FREQ) / AFSK_SAMPLE_RATE;
    
    hafsk->mark_coeff = 2.0f * cosf(omega_mark);
    hafsk->space_coeff = 2.0f * cosf(omega_space);
}

/**
  * @brief  Update Goertzel filter state
  * @param  sample: Input sample
  * @param  coeff: Goertzel coefficient
  * @param  q1: Pointer to q1 state variable
  * @param  q2: Pointer to q2 state variable
  * @retval None
  */
static void AFSK_GoertzelUpdate(float32_t sample, float32_t coeff, 
                                 float32_t *q1, float32_t *q2)
{
    float32_t q0 = coeff * (*q1) - (*q2) + sample;
    *q2 = *q1;
    *q1 = q0;
}

/**
  * @brief  Calculate Goertzel magnitude
  * @param  q1: q1 state variable
  * @param  q2: q2 state variable
  * @param  coeff: Goertzel coefficient
  * @retval Magnitude squared
  */
static float32_t AFSK_GoertzelMagnitude(float32_t q1, float32_t q2, float32_t coeff)
{
    /* magnitude^2 = q1^2 + q2^2 - q1*q2*coeff */
    float32_t real = q1 - q2 * coeff / 2.0f;
    float32_t imag = q2 * sinf(acosf(coeff / 2.0f));
    return real * real + imag * imag;
}

/**
  * @brief  Update PLL for bit synchronization
  * @param  hafsk: Pointer to AFSK demodulator handle
  * @param  transition: true if bit transition detected
  * @retval None
  */
static void AFSK_PLLUpdate(AFSK_Demod_HandleTypeDef *hafsk, bool transition)
{
    if (transition) {
        /* Adjust PLL phase based on transition timing */
        if (hafsk->pll_phase < 0x8000) {
            /* Phase is early, slow down clock */
            hafsk->pll_dphase -= 1;
        } else {
            /* Phase is late, speed up clock */
            hafsk->pll_dphase += 1;
        }
        
        /* Limit PLL frequency adjustment range */
        int16_t nominal = 0x10000 / SAMPLES_PER_BIT;
        if (hafsk->pll_dphase < nominal - 100) {
            hafsk->pll_dphase = nominal - 100;
        }
        if (hafsk->pll_dphase > nominal + 100) {
            hafsk->pll_dphase = nominal + 100;
        }
    }
}

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  Initialize AFSK demodulator
  * @param  hafsk: Pointer to AFSK demodulator handle
  * @retval 0 if success, -1 otherwise
  */
int32_t AFSK_Init(AFSK_Demod_HandleTypeDef *hafsk)
{
    if (hafsk == NULL) {
        return -1;
    }
    
    /* Calculate Goertzel coefficients */
    AFSK_CalculateCoefficients(hafsk);
    
#if USE_CMSIS_DSP
    /* Initialize FIR filters for bandpass filtering */
    AFSK_InitFIRFilters(hafsk);
    hafsk->buffer_index = 0;
#endif
    
    /* Reset state */
    AFSK_Reset(hafsk);
    
    return 0;
}

/**
  * @brief  Reset AFSK demodulator state
  * @param  hafsk: Pointer to AFSK demodulator handle
  * @retval None
  */
void AFSK_Reset(AFSK_Demod_HandleTypeDef *hafsk)
{
    hafsk->mark_q1 = hafsk->mark_q2 = 0.0f;
    hafsk->space_q1 = hafsk->space_q2 = 0.0f;
    hafsk->sample_counter = 0;
    hafsk->current_bit = 0;
    hafsk->bit_ready = false;
    hafsk->pll_phase = 0;
    hafsk->pll_dphase = 0x10000 / SAMPLES_PER_BIT;
    hafsk->mark_energy = 0;
    hafsk->space_energy = 0;
    hafsk->total_energy = 0;
    hafsk->carrier_detected = false;
    hafsk->carrier_lock_count = 0;
    
#if USE_CMSIS_DSP
    memset(hafsk->fir_mark_state, 0, sizeof(hafsk->fir_mark_state));
    memset(hafsk->fir_space_state, 0, sizeof(hafsk->fir_space_state));
    hafsk->buffer_index = 0;
#endif
}

/**
  * @brief  Process a single sample
  * @param  hafsk: Pointer to AFSK demodulator handle
  * @param  sample: Input sample (0 or 1)
  * @retval true if a bit is ready, false otherwise
  */
bool AFSK_ProcessSample(AFSK_Demod_HandleTypeDef *hafsk, uint8_t sample)
{
    /* Convert sample to float (-1 or +1) */
    float32_t fsample = (sample == 0) ? -1.0f : 1.0f;
    
#if USE_CMSIS_DSP
    /* Apply FIR bandpass filters */
    float32_t mark_filtered, space_filtered;
    arm_fir_f32(&hafsk->fir_mark, &fsample, &mark_filtered, 1);
    arm_fir_f32(&hafsk->fir_space, &fsample, &space_filtered, 1);
    
    /* Store in buffers for Goertzel */
    hafsk->mark_buffer[hafsk->buffer_index] = mark_filtered;
    hafsk->space_buffer[hafsk->buffer_index] = space_filtered;
    hafsk->buffer_index++;
#else
    /* Update Goertzel filters */
    AFSK_GoertzelUpdate(fsample, hafsk->mark_coeff, &hafsk->mark_q1, &hafsk->mark_q2);
    AFSK_GoertzelUpdate(fsample, hafsk->space_coeff, &hafsk->space_q1, &hafsk->space_q2);
#endif
    
    /* Update PLL phase */
    hafsk->pll_phase += hafsk->pll_dphase;
    
    /* Increment sample counter */
    hafsk->sample_counter++;
    
    /* Bit decision point */
    if (hafsk->pll_phase >= 0x10000) {
        hafsk->pll_phase -= 0x10000;
        
        /* Calculate Mark and Space energy */
        float32_t mark_mag, space_mag;
        
#if USE_CMSIS_DSP
        mark_mag = AFSK_GoertzelCMSIS(hafsk->mark_buffer, AFSK_MARK_FREQ);
        space_mag = AFSK_GoertzelCMSIS(hafsk->space_buffer, AFSK_SPACE_FREQ);
        hafsk->buffer_index = 0;
#else
        mark_mag = AFSK_GoertzelMagnitude(hafsk->mark_q1, hafsk->mark_q2, hafsk->mark_coeff);
        space_mag = AFSK_GoertzelMagnitude(hafsk->space_q1, hafsk->space_q2, hafsk->space_coeff);
#endif
        
        /* Bit decision: Mark > Space -> bit 1, else -> bit 0 */
        uint8_t new_bit = (mark_mag > space_mag) ? 1 : 0;
        
        /* Detect transition for PLL adjustment */
        bool transition = (new_bit != hafsk->current_bit);
        AFSK_PLLUpdate(hafsk, transition);
        
        hafsk->current_bit = new_bit;
        hafsk->bit_ready = true;
        
        /* Update energy statistics */
        hafsk->mark_energy = (uint16_t)mark_mag;
        hafsk->space_energy = (uint16_t)space_mag;
        hafsk->total_energy = hafsk->mark_energy + hafsk->space_energy;
        
        /* Carrier detection */
        if (hafsk->total_energy > CARRIER_DETECT_THR) {
            if (hafsk->carrier_lock_count < 255) {
                hafsk->carrier_lock_count++;
            }
            if (hafsk->carrier_lock_count > 5) {
                hafsk->carrier_detected = true;
            }
        } else {
            if (hafsk->carrier_lock_count > 0) {
                hafsk->carrier_lock_count--;
            }
            if (hafsk->carrier_lock_count == 0) {
                hafsk->carrier_detected = false;
            }
        }
        
        /* Reset Goertzel state for next bit period */
        hafsk->mark_q1 = hafsk->mark_q2 = 0.0f;
        hafsk->space_q1 = hafsk->space_q2 = 0.0f;
        hafsk->sample_counter = 0;
        
        return true;
    }
    
    return false;
}

/**
  * @brief  Get demodulated bit
  * @param  hafsk: Pointer to AFSK demodulator handle
  * @retval Demodulated bit value (0 or 1)
  */
uint8_t AFSK_GetBit(AFSK_Demod_HandleTypeDef *hafsk)
{
    hafsk->bit_ready = false;
    return hafsk->current_bit;
}

/**
  * @brief  Get signal quality
  * @param  hafsk: Pointer to AFSK demodulator handle
  * @retval Signal quality (0-100)
  */
uint8_t AFSK_GetSignalQuality(AFSK_Demod_HandleTypeDef *hafsk)
{
    if (hafsk->total_energy == 0) {
        return 0;
    }
    
    /* Signal quality based on Mark and Space energy difference */
    uint16_t diff = (hafsk->mark_energy > hafsk->space_energy) ?
                    (hafsk->mark_energy - hafsk->space_energy) :
                    (hafsk->space_energy - hafsk->mark_energy);
    
    uint8_t quality = (diff * 100) / (hafsk->total_energy + 1);
    
    /* Limit to 0-100 range */
    if (quality > 100) {
        quality = 100;
    }
    
    return quality;
}

/**
  * @brief  Check if carrier is detected
  * @param  hafsk: Pointer to AFSK demodulator handle
  * @retval true if carrier detected, false otherwise
  */
bool AFSK_IsCarrierDetected(AFSK_Demod_HandleTypeDef *hafsk)
{
    return hafsk->carrier_detected;
}

/* CMSIS-DSP optimized functions ---------------------------------------------*/
#if USE_CMSIS_DSP

/**
  * @brief  Initialize FIR bandpass filters
  * @param  hafsk: Pointer to AFSK demodulator handle
  * @retval None
  */
static void AFSK_InitFIRFilters(AFSK_Demod_HandleTypeDef *hafsk)
{
    /* Design Mark frequency bandpass filter (2200Hz ± 200Hz) */
    AFSK_DesignBandpassFilter(hafsk->fir_mark_coeffs, 32, AFSK_MARK_FREQ, 400);
    
    /* Design Space frequency bandpass filter (1200Hz ± 200Hz) */
    AFSK_DesignBandpassFilter(hafsk->fir_space_coeffs, 32, AFSK_SPACE_FREQ, 400);
    
    /* Initialize FIR filter instances */
    arm_fir_init_f32(&hafsk->fir_mark, 32, hafsk->fir_mark_coeffs, 
                     hafsk->fir_mark_state, 1);
    arm_fir_init_f32(&hafsk->fir_space, 32, hafsk->fir_space_coeffs, 
                     hafsk->fir_space_state, 1);
}

/**
  * @brief  Design bandpass filter using windowed FIR method
  * @param  coeffs: Pointer to coefficient array
  * @param  num_taps: Number of filter taps
  * @param  center_freq: Center frequency in Hz
  * @param  bandwidth: Bandwidth in Hz
  * @retval None
  */
static void AFSK_DesignBandpassFilter(float32_t *coeffs, uint16_t num_taps,
                                       float32_t center_freq, float32_t bandwidth)
{
    /* Normalized frequencies */
    float32_t fc1 = (center_freq - bandwidth/2.0f) / AFSK_SAMPLE_RATE;
    float32_t fc2 = (center_freq + bandwidth/2.0f) / AFSK_SAMPLE_RATE;
    
    for (uint16_t i = 0; i < num_taps; i++) {
        float32_t n = (float32_t)i - (num_taps - 1) / 2.0f;
        
        /* Ideal bandpass filter impulse response */
        float32_t h;
        if (n == 0.0f) {
            h = 2.0f * (fc2 - fc1);
        } else {
            h = (sinf(2.0f * PI * fc2 * n) - sinf(2.0f * PI * fc1 * n)) / (PI * n);
        }
        
        /* Hamming window */
        float32_t window = 0.54f - 0.46f * cosf(2.0f * PI * i / (num_taps - 1));
        
        coeffs[i] = h * window;
    }
    
    /* Normalize coefficients */
    float32_t sum = 0.0f;
    for (uint16_t i = 0; i < num_taps; i++) {
        sum += coeffs[i];
    }
    if (sum != 0.0f) {
        for (uint16_t i = 0; i < num_taps; i++) {
            coeffs[i] /= sum;
        }
    }
}

/**
  * @brief  Calculate Goertzel magnitude using CMSIS-DSP
  * @param  buffer: Pointer to sample buffer
  * @param  freq: Target frequency in Hz
  * @retval Magnitude
  */
static float32_t AFSK_GoertzelCMSIS(float32_t *buffer, uint16_t freq)
{
    float32_t omega = (2.0f * PI * freq) / AFSK_SAMPLE_RATE;
    float32_t coeff = 2.0f * cosf(omega);
    
    float32_t q0, q1 = 0.0f, q2 = 0.0f;
    
    /* Goertzel iteration using CMSIS-DSP */
    for (uint8_t i = 0; i < SAMPLES_PER_BIT; i++) {
        q0 = coeff * q1 - q2 + buffer[i];
        q2 = q1;
        q1 = q0;
    }
    
    /* Calculate magnitude */
    float32_t real = q1 - q2 * coeff / 2.0f;
    float32_t imag = q2 * sinf(omega);
    
    float32_t magnitude;
    arm_sqrt_f32(real * real + imag * imag, &magnitude);
    
    return magnitude;
}

#endif /* USE_CMSIS_DSP */

