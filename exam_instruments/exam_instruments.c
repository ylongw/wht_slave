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
#define EXAM_UART               huart7      // RS485 UART

/* Square wave output pin - using LED1 (PG9) which is easily accessible */
#define EXAM_SQUARE_WAVE_PORT   GPIOG
#define EXAM_SQUARE_WAVE_PIN    GPIO_PIN_9

/* Private variables ---------------------------------------------------------*/
static char cmd_buffer[EXAM_CMD_BUFFER_SIZE];
static uint8_t cmd_index = 0;
static uint8_t uart_rx_byte = 0;

/* Private function prototypes -----------------------------------------------*/
static void exam_print_boot_message(void);
static void exam_process_command(void);
static void exam_uart_send_string(const char* str);
static void exam_configure_square_wave_timer(void);

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  Initialize exam instruments firmware
 * @retval None
 */
void exam_instruments_init(void) {
    // Print boot message
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
    
    // Start UART receive interrupt
    HAL_UART_Receive_IT(&EXAM_UART, &uart_rx_byte, 1);
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
 * @brief  Print boot message over UART
 * @retval None
 */
static void exam_print_boot_message(void) {
    char msg[128];
    
    // Enable RS485 transmit
    HAL_GPIO_WritePin(RS485_CTRL_GPIO_Port, RS485_CTRL_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
    
    // Build boot message with version and build date
    snprintf(msg, sizeof(msg), "\r\n=== BOOT OK ===\r\n");
    exam_uart_send_string(msg);
    
    snprintf(msg, sizeof(msg), "Version: %s\r\n", EXAM_VERSION_STRING);
    exam_uart_send_string(msg);
    
    snprintf(msg, sizeof(msg), "Build: %s %s\r\n", __DATE__, __TIME__);
    exam_uart_send_string(msg);
    
    snprintf(msg, sizeof(msg), "FW Ver: %d.%d.%d\r\n", 
             FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH);
    exam_uart_send_string(msg);
    
    snprintf(msg, sizeof(msg), "Square Wave: 1 kHz on LED1 (PG9)\r\n");
    exam_uart_send_string(msg);
    
    snprintf(msg, sizeof(msg), "Ready for commands (PING)\r\n");
    exam_uart_send_string(msg);
    
    // Switch back to receive mode
    HAL_Delay(1);
    HAL_GPIO_WritePin(RS485_CTRL_GPIO_Port, RS485_CTRL_Pin, GPIO_PIN_RESET);
}

/**
 * @brief  Send string over UART
 * @param  str: Null-terminated string to send
 * @retval None
 */
static void exam_uart_send_string(const char* str) {
    HAL_UART_Transmit(&EXAM_UART, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

/**
 * @brief  Process received command
 * @retval None
 */
static void exam_process_command(void) {
    // Null-terminate the command
    cmd_buffer[cmd_index] = '\0';
    
    // Trim trailing CR/LF
    while (cmd_index > 0 && (cmd_buffer[cmd_index - 1] == '\r' || 
                              cmd_buffer[cmd_index - 1] == '\n')) {
        cmd_index--;
        cmd_buffer[cmd_index] = '\0';
    }
    
    // Check for PING command
    if (strcmp(cmd_buffer, "PING") == 0 || strcmp(cmd_buffer, "ping") == 0) {
        // Enable transmit
        HAL_GPIO_WritePin(RS485_CTRL_GPIO_Port, RS485_CTRL_Pin, GPIO_PIN_SET);
        HAL_Delay(1);
        
        // Send PONG response
        exam_uart_send_string("PONG\r\n");
        
        // Switch back to receive
        HAL_Delay(1);
        HAL_GPIO_WritePin(RS485_CTRL_GPIO_Port, RS485_CTRL_Pin, GPIO_PIN_RESET);
    }
    
    // Reset command buffer
    cmd_index = 0;
    memset(cmd_buffer, 0, sizeof(cmd_buffer));
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
    if (huart->Instance == EXAM_UART.Instance) {
        // Store received byte
        if (cmd_index < EXAM_CMD_BUFFER_SIZE - 1) {
            cmd_buffer[cmd_index++] = uart_rx_byte;
            
            // Check for command termination (CR or LF)
            if (uart_rx_byte == '\r' || uart_rx_byte == '\n') {
                exam_process_command();
            }
        } else {
            // Buffer overflow, reset
            cmd_index = 0;
        }
        
        // Restart receive
        HAL_UART_Receive_IT(&EXAM_UART, &uart_rx_byte, 1);
    }
}
#endif
#endif
