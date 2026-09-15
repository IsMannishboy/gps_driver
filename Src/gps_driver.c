#include "gps_driver.h"
#include <string.h>
static GPS_Data History[HISTORY_LENGHT]; // user array
static uint8_t counter = 0;
volatile bool FLAG = false;
static GPS_Data current_pos = {0};

GPS_Data get_latest_pos()
{
    GPS_Data copy;
    __disable_irq();
    copy = current_pos;
    __enable_irq();
    return copy;
}

void get_all_pos(GPS_Data *copy)
{
    __disable_irq();
    memcpy(copy, History, sizeof(History));
    __enable_irq();
}

bool gps_is_fix_stale(uint32_t max_age_ms)
{
    uint32_t ts;
    __disable_irq();
    ts = current_pos.timestamp;
    __enable_irq();
    if (ts == 0) return true; // no fix received yet
    return (uint32_t)(HAL_GetTick() - ts) > max_age_ms;
}

static void gps_decode_sentence(const char *line)
{
    switch (minmea_sentence_id(line, false)) {
        case MINMEA_SENTENCE_RMC: {
            struct minmea_sentence_rmc frame;
            if (!minmea_parse_rmc(&frame, line)) return;
            current_pos.latitude  = minmea_tocoord(&frame.latitude);
            current_pos.longitude = minmea_tocoord(&frame.longitude);
            current_pos.fix_valid = frame.valid;
            break;
        }
        case MINMEA_SENTENCE_GGA: {
            struct minmea_sentence_gga frame;
            if (!minmea_parse_gga(&frame, line)) return;
            current_pos.latitude   = minmea_tocoord(&frame.latitude);
            current_pos.longitude  = minmea_tocoord(&frame.longitude);
            current_pos.altitude   = minmea_tocoord(&frame.altitude);
            current_pos.satellites = (uint8_t)frame.satellites_tracked;
            current_pos.fix_valid  = frame.fix_quality > 0;
            break;
        }
        default:
            return; // other sentence types (GSA/GSV/...) or a checksum/parse failure — nothing to update
    }

    current_pos.timestamp = HAL_GetTick();
    History[counter % HISTORY_LENGHT] = current_pos;
    counter++;
    FLAG = true; // only signal "new data" once a sentence was actually decoded
}

#if DMA_MODE == DMA_MODE_CIRCULAR
//change call back end interrupt
static uint8_t BUFFER[LEN_OF_BUFFER][SIZE_OF_BUFFER];
static uint8_t index = 0;
HAL_StatusTypeDef gps_init(UART_HandleTypeDef *huart)
{
    return HAL_UARTEx_ReceiveToIdle_DMA(huart, BUFFER[index], SIZE_OF_BUFFER);
}
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance != USART2) return;

    uint8_t new_index = (1 + index) % LEN_OF_BUFFER;
    HAL_UARTEx_ReceiveToIdle_DMA(huart, BUFFER[new_index], SIZE_OF_BUFFER);

    if (huart->RxEventType == HAL_UART_RXEVENT_IDLE && Size < SIZE_OF_BUFFER) {
        BUFFER[index][Size] = '\0'; // minmea_parse_* expect a null-terminated C string
        gps_decode_sentence((const char *)BUFFER[index]);
    }
    index = new_index;
}


#else
static uint8_t BUFFER[SIZE_OF_BUFFER];




HAL_StatusTypeDef gps_init(UART_HandleTypeDef *huart)
{
    return HAL_UARTEx_ReceiveToIdle_DMA(huart, BUFFER, SIZE_OF_BUFFER);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance != USART2) return;

    if (huart->RxEventType == HAL_UART_RXEVENT_IDLE && Size < SIZE_OF_BUFFER) {
        BUFFER[Size] = '\0'; // minmea_parse_* expect a null-terminated C string
        gps_decode_sentence((const char *)BUFFER);
    }

    HAL_UARTEx_ReceiveToIdle_DMA(huart, BUFFER, SIZE_OF_BUFFER);
}

#endif

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART2) return;

    // framing/noise/overrun on the line kills the current DMA reception;
    // without this, RxEventCallback would never fire again after the first glitch
#if DMA_MODE == DMA_MODE_CIRCULAR
    index = 0;
    HAL_UARTEx_ReceiveToIdle_DMA(huart, BUFFER[index], SIZE_OF_BUFFER);
#else
    HAL_UARTEx_ReceiveToIdle_DMA(huart, BUFFER, SIZE_OF_BUFFER);
#endif
}
