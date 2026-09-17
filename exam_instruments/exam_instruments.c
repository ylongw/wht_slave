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
static uint8_t uart_rx_byte_ttl = 0;

static char cmd_buffer_rs232[EXAM_CMD_BUFFER_SIZE];
static uint8_t cmd_index_rs232 = 0;
static uint8_t uart_rx_byte_rs232 = 0;

/* Private function prototypes -----------------------------------------------*/
static void exam_print_boot_message(void);
static void exam_process_command_ttl(void);
static void exam_process_command_rs232(void);
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
    
    // Start UART receive interrupt for both UARTs
    HAL_UART_Receive_IT(&EXAM_TTL_UART, &uart_rx_byte_ttl, 1);
    HAL_UART_Receive_IT(&EXAM_RS232_UART, &uart_rx_byte_rs232, 1);
}

/**
 * @brief  Main exam instruments run loop
 * @retval None
 */
void exam_instruments_run(void) {
    // Main loop - just keep toggling the run LED slowly to show we're alive
    while (1) {
        HAL_GPIO_TogglePin(RUN_LED_GPIO_Port, RUN_LED_Pin);
        HAL_Delay(1000);
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
 * @brief  Send string over UART
 * @param  huart: UART handle
 * @param  str: Null-terminated string to send
 * @retval None
 */
static void exam_uart_send_string(UART_HandleTypeDef *huart, const char* str) {
    HAL_UART_Transmit(huart, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

/**
 * @brief  Process received command from TTL UART
 * @retval None
 */
static void exam_process_command_ttl(void) {
    // Null-terminate the command
    cmd_buffer_ttl[cmd_index_ttl] = '\0';
    
    // Trim trailing CR/LF
    while (cmd_index_ttl > 0 && (cmd_buffer_ttl[cmd_index_ttl - 1] == '\r' || 
                                  cmd_buffer_ttl[cmd_index_ttl - 1] == '\n')) {
        cmd_index_ttl--;
        cmd_buffer_ttl[cmd_index_ttl] = '\0';
    }
    
    // Check for PING command
    if (strcmp(cmd_buffer_ttl, "PING") == 0 || strcmp(cmd_buffer_ttl, "ping") == 0) {
        exam_uart_send_string(&EXAM_TTL_UART, "PONG\r\n");
    }
    
    // Reset command buffer
    cmd_index_ttl = 0;
    memset(cmd_buffer_ttl, 0, sizeof(cmd_buffer_ttl));
}

/**
 * @brief  Process received command from RS-232 UART
 * @retval None
 */
static void exam_process_command_rs232(void) {
    // Null-terminate the command
    cmd_buffer_rs232[cmd_index_rs232] = '\0';
    
    // Trim trailing CR/LF
    while (cmd_index_rs232 > 0 && (cmd_buffer_rs232[cmd_index_rs232 - 1] == '\r' || 
                                    cmd_buffer_rs232[cmd_index_rs232 - 1] == '\n')) {
        cmd_index_rs232--;
        cmd_buffer_rs232[cmd_index_rs232] = '\0';
    }
    
    // Check for PING command
    if (strcmp(cmd_buffer_rs232, "PING") == 0 || strcmp(cmd_buffer_rs232, "ping") == 0) {
        exam_uart_send_string(&EXAM_RS232_UART, "PONG\r\n");
    }
    
    // Reset command buffer
    cmd_index_rs232 = 0;
    memset(cmd_buffer_rs232, 0, sizeof(cmd_buffer_rs232));
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

/**
 * @brief  UART receive complete callback
 * @param  huart: UART handle
 * @retval None
 */
#ifdef WHT_APP_RUN_MODE
#if WHT_APP_RUN_MODE == 6
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    // Handle TTL UART (UART4)
    if (huart->Instance == EXAM_TTL_UART.Instance) {
        if (cmd_index_ttl < EXAM_CMD_BUFFER_SIZE - 1) {
            cmd_buffer_ttl[cmd_index_ttl++] = uart_rx_byte_ttl;
            
            // Check for command termination (CR or LF)
            if (uart_rx_byte_ttl == '\r' || uart_rx_byte_ttl == '\n') {
                exam_process_command_ttl();
            }
        } else {
            // Buffer overflow, reset
            cmd_index_ttl = 0;
        }
        
        // Restart receive
        HAL_UART_Receive_IT(&EXAM_TTL_UART, &uart_rx_byte_ttl, 1);
    }
    // Handle RS-232 UART (USART1)
    else if (huart->Instance == EXAM_RS232_UART.Instance) {
        if (cmd_index_rs232 < EXAM_CMD_BUFFER_SIZE - 1) {
            cmd_buffer_rs232[cmd_index_rs232++] = uart_rx_byte_rs232;
            
            // Check for command termination (CR or LF)
            if (uart_rx_byte_rs232 == '\r' || uart_rx_byte_rs232 == '\n') {
                exam_process_command_rs232();
            }
        } else {
            // Buffer overflow, reset
            cmd_index_rs232 = 0;
        }
        
        // Restart receive
        HAL_UART_Receive_IT(&EXAM_RS232_UART, &uart_rx_byte_rs232, 1);
    }
}
#endif
#endif
