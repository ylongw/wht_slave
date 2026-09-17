/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : exam_instruments.c
 * @brief          : Exam instruments firmware implementation
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "exam_instruments.h"
#include "main.h"
#include "tim.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define EXAM_CMD_BUFFER_SIZE    32
#define LED_BLINK_INTERVAL_MS   500

/* TTL UART (3.3V logic) - UART4 on PA0/PA1 */
#define EXAM_TTL_UART           huart4      // DEBUG_UART, TTL levels

/* RS-232 UART - USART1 on PA9/PA10 */
#define EXAM_RS232_UART         huart1      // RS-232 levels

/* Square wave output pin - using LED1 (PG9) which is easily accessible */
#define EXAM_SQUARE_WAVE_PORT   GPIOG
#define EXAM_SQUARE_WAVE_PIN    GPIO_PIN_9

/* Private variables ---------------------------------------------------------*/
static char cmd_buffer_ttl[EXAM_CMD_BUFFER_SIZE];
static uint8_t cmd_index_ttl = 0;

static char cmd_buffer_rs232[EXAM_CMD_BUFFER_SIZE];
static uint8_t cmd_index_rs232 = 0;

static uint32_t last_led_blink_tick = 0;

/* Private function prototypes -----------------------------------------------*/
static void exam_print_boot_message(void);
static void exam_process_uart_rx(void);
static int exam_check_for_ping(const char* buffer, uint8_t length);
static void exam_uart_send_string(UART_HandleTypeDef *huart, const char* str);
static void exam_configure_square_wave_timer(void);

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  Initialize exam instruments firmware
 * @retval None
 */
void exam_instruments_init(void) {
    // Print boot message on both UARTs
    exam_print_boot_message();
    
    // Configure square wave output GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOG_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = EXAM_SQUARE_WAVE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;  // TIM2_CH2 on PG9
    HAL_GPIO_Init(EXAM_SQUARE_WAVE_PORT, &GPIO_InitStruct);
    
    // Configure TIM2 for 1 kHz square wave
    exam_configure_square_wave_timer();
    
    // Initialize LED blink timer
    last_led_blink_tick = HAL_GetTick();
    
    // Initialize command buffers
    memset(cmd_buffer_ttl, 0, sizeof(cmd_buffer_ttl));
    memset(cmd_buffer_rs232, 0, sizeof(cmd_buffer_rs232));
}

/**
 * @brief  Main exam instruments run loop
 * @retval None
 */
void exam_instruments_run(void) {
    // Main loop - tight polling for UART RX with LED blink on tick
    while (1) {
        // Poll both UARTs for data
        exam_process_uart_rx();
        
        // Blink RUN LED every 500ms
        uint32_t current_tick = HAL_GetTick();
        if (current_tick - last_led_blink_tick >= LED_BLINK_INTERVAL_MS) {
            HAL_GPIO_TogglePin(RUN_LED_GPIO_Port, RUN_LED_Pin);
            last_led_blink_tick = current_tick;
        }
    }
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Print boot message over both UARTs
 * @retval None
 */
static void exam_print_boot_message(void) {
    char msg[128];
    
    // Build boot messages
    snprintf(msg, sizeof(msg), "\r\n=== BOOT OK ===\r\n");
    exam_uart_send_string(&EXAM_TTL_UART, msg);
    exam_uart_send_string(&EXAM_RS232_UART, msg);
    HAL_Delay(5);
    
    snprintf(msg, sizeof(msg), "Version: %s\r\n", EXAM_VERSION_STRING);
    exam_uart_send_string(&EXAM_TTL_UART, msg);
    exam_uart_send_string(&EXAM_RS232_UART, msg);
    HAL_Delay(5);
    
    snprintf(msg, sizeof(msg), "Build: %s %s\r\n", __DATE__, __TIME__);
    exam_uart_send_string(&EXAM_TTL_UART, msg);
    exam_uart_send_string(&EXAM_RS232_UART, msg);
    HAL_Delay(5);
    
    snprintf(msg, sizeof(msg), "FW Ver: %d.%d.%d\r\n", 
             FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH);
    exam_uart_send_string(&EXAM_TTL_UART, msg);
    exam_uart_send_string(&EXAM_RS232_UART, msg);
    HAL_Delay(5);
    
    snprintf(msg, sizeof(msg), "Square Wave: 1 kHz on LED1 (PG9)\r\n");
    exam_uart_send_string(&EXAM_TTL_UART, msg);
    exam_uart_send_string(&EXAM_RS232_UART, msg);
    HAL_Delay(5);
    
    snprintf(msg, sizeof(msg), "TTL UART: UART4 (PA0/PA1) 115200 8N1\r\n");
    exam_uart_send_string(&EXAM_TTL_UART, msg);
    exam_uart_send_string(&EXAM_RS232_UART, msg);
    HAL_Delay(5);
    
    snprintf(msg, sizeof(msg), "RS-232: USART1 (PA9/PA10) 115200 8N1\r\n");
    exam_uart_send_string(&EXAM_TTL_UART, msg);
    exam_uart_send_string(&EXAM_RS232_UART, msg);
    HAL_Delay(5);
    
    snprintf(msg, sizeof(msg), "Ready for commands (PING)\r\n");
    exam_uart_send_string(&EXAM_TTL_UART, msg);
    exam_uart_send_string(&EXAM_RS232_UART, msg);
    HAL_Delay(5);
}

/**
 * @brief  Poll both UARTs for received data and process commands
 * @retval None
 */
static void exam_process_uart_rx(void) {
    uint8_t rx_byte;
    
    // Check TTL UART (UART4) for data
    if (__HAL_UART_GET_FLAG(&EXAM_TTL_UART, UART_FLAG_RXNE)) {
        // Read byte from data register
        rx_byte = (uint8_t)(EXAM_TTL_UART.Instance->DR & 0xFF);
        
        // Add to buffer if space available
        if (cmd_index_ttl < EXAM_CMD_BUFFER_SIZE - 1) {
            cmd_buffer_ttl[cmd_index_ttl++] = rx_byte;
            
            // Check if we have a complete PING command (with or without CR/LF)
            if (exam_check_for_ping(cmd_buffer_ttl, cmd_index_ttl)) {
                exam_uart_send_string(&EXAM_TTL_UART, "PONG\r\n");
                cmd_index_ttl = 0;
                memset(cmd_buffer_ttl, 0, sizeof(cmd_buffer_ttl));
            }
            // Also check for newline to reset buffer (non-PING commands)
            else if (rx_byte == '\r' || rx_byte == '\n') {
                cmd_index_ttl = 0;
                memset(cmd_buffer_ttl, 0, sizeof(cmd_buffer_ttl));
            }
        } else {
            // Buffer full, reset
            cmd_index_ttl = 0;
            memset(cmd_buffer_ttl, 0, sizeof(cmd_buffer_ttl));
        }
    }
    
    // Check RS-232 UART (USART1) for data
    if (__HAL_UART_GET_FLAG(&EXAM_RS232_UART, UART_FLAG_RXNE)) {
        // Read byte from data register
        rx_byte = (uint8_t)(EXAM_RS232_UART.Instance->DR & 0xFF);
        
        // Add to buffer if space available
        if (cmd_index_rs232 < EXAM_CMD_BUFFER_SIZE - 1) {
            cmd_buffer_rs232[cmd_index_rs232++] = rx_byte;
            
            // Check if we have a complete PING command (with or without CR/LF)
            if (exam_check_for_ping(cmd_buffer_rs232, cmd_index_rs232)) {
                exam_uart_send_string(&EXAM_RS232_UART, "PONG\r\n");
                cmd_index_rs232 = 0;
                memset(cmd_buffer_rs232, 0, sizeof(cmd_buffer_rs232));
            }
            // Also check for newline to reset buffer (non-PING commands)
            else if (rx_byte == '\r' || rx_byte == '\n') {
                cmd_index_rs232 = 0;
                memset(cmd_buffer_rs232, 0, sizeof(cmd_buffer_rs232));
            }
        } else {
            // Buffer full, reset
            cmd_index_rs232 = 0;
            memset(cmd_buffer_rs232, 0, sizeof(cmd_buffer_rs232));
        }
    }
}

/**
 * @brief  Check if buffer contains "PING" (case-insensitive, with or without trailing whitespace)
 * @param  buffer: Command buffer to check
 * @param  length: Current length of buffer
 * @retval 1 if PING found, 0 otherwise
 */
static int exam_check_for_ping(const char* buffer, uint8_t length) {
    // Need at least 4 characters for "PING"
    if (length < 4) {
        return 0;
    }
    
    // Check if buffer ends with PING (or ping) - case insensitive
    // Can be followed by nothing, spaces, CR, or LF
    uint8_t ping_len = 0;
    
    // Find the last non-whitespace position
    int last_char_pos = length - 1;
    while (last_char_pos >= 0 && (buffer[last_char_pos] == ' ' || 
                                   buffer[last_char_pos] == '\r' || 
                                   buffer[last_char_pos] == '\n' ||
                                   buffer[last_char_pos] == '\t')) {
        last_char_pos--;
    }
    
    // Check if we have exactly "PING" or "ping"
    if (last_char_pos == 3) {
        // Check all 4 characters (case insensitive)
        if ((buffer[0] == 'P' || buffer[0] == 'p') &&
            (buffer[1] == 'I' || buffer[1] == 'i') &&
            (buffer[2] == 'N' || buffer[2] == 'n') &&
            (buffer[3] == 'G' || buffer[3] == 'g')) {
            return 1;
        }
    }
    
    return 0;
}

/**
 * @brief  Send string over UART
 * @param  huart: UART handle
 * @param  str: Null-terminated string to send
 * @retval None
 */
static void exam_uart_send_string(UART_HandleTypeDef *huart, const char* str) {
    HAL_UART_Transmit(huart, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

/**
 * @brief  Configure TIM2 for 1 kHz 50% duty cycle square wave on PG9
 * @retval None
 */
static void exam_configure_square_wave_timer(void) {
    TIM_OC_InitTypeDef sConfigOC = {0};
    
    // TIM2 is already initialized, we just need to reconfigure it
    // TIM2 clock = 90 MHz (APB1 * 2), prescaler to get 1 MHz timer clock
    // Then period for 1 kHz output
    
    htim2.Init.Prescaler = 90 - 1;          // 90 MHz / 90 = 1 MHz timer clock
    htim2.Init.Period = 1000 - 1;            // 1 MHz / 1000 = 1 kHz PWM
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    
    // Configure PWM channel 2 (TIM2_CH2 = PG9 = LED1)
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 500;                   // 50% duty cycle
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) {
        Error_Handler();
    }
    
    // Start PWM output
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}
