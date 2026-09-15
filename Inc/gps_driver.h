#pragma once
#include "stm32f4xx_hal.h" // import hal library for f4 (it can be changed for another types)
#include  "minmea.h"

#ifndef LEN_OF_BUFFER
#define LEN_OF_BUFFER 10
#endif
#ifndef SIZE_OF_BUFFER
#define SIZE_OF_BUFFER 512
#endif
#ifndef HISTORY_LENGHT
#define HISTORY_LENGHT 10
#endif

#define DMA_MODE_NORMAL   0
#define DMA_MODE_CIRCULAR 1
#ifndef DMA_MODE
#define DMA_MODE DMA_MODE_NORMAL
#endif
#ifdef __cplusplus
extern "C" {
#endif
extern volatile bool FLAG;
typedef struct {
    float latitude;
    float longitude;
    float altitude;
    uint8_t  satellites;
    uint8_t  fix_valid;
    uint32_t timestamp; 
} GPS_Data;

HAL_StatusTypeDef gps_init(UART_HandleTypeDef *huart);
GPS_Data get_latest_pos();
void get_all_pos(GPS_Data *copy); // copy must point to an array of at least HISTORY_LENGHT elements
bool gps_is_fix_stale(uint32_t max_age_ms); // true if no fix yet, or last fix older than max_age_ms

#ifdef __cplusplus
}
#endif
