/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led_animation.h"
#include "led_transition_manager.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

#define DEBUG_BUFFER_SIZE 100
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Custom printf-like function
// void UART_printf(const char* fmt, ...)
// {
//     char     buffer[DEBUG_BUFFER_SIZE];
//     uint32_t timestamp = HAL_GetTick();
//     va_list  args;
//     va_start(args, fmt);
//     vsnprintf(buffer, DEBUG_BUFFER_SIZE, fmt, args);
//     va_end(args);
//     HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
// }

void UART_printf(const char* fmt, ...)
{
    char     buffer[DEBUG_BUFFER_SIZE];
    char     timestampBuffer[20];       // Buffer to hold the timestamp string
    uint32_t timestamp = HAL_GetTick(); // Get the current tick count (milliseconds)

    // Create a formatted string with the timestamp at the beginning
    snprintf(timestampBuffer, sizeof(timestampBuffer), "[%lu ms] ", timestamp);

    // Append the timestamp to the message buffer
    strncpy(buffer, timestampBuffer, sizeof(buffer)); // Copy timestamp to buffer

    // Prepare the rest of the message
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer + strlen(timestampBuffer), DEBUG_BUFFER_SIZE - strlen(timestampBuffer), fmt,
              args); // Append the formatted message to buffer
    va_end(args);

    // Transmit the complete message with timestamp over UART
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
}

void PWM_SetDutyCycle_CH1(uint16_t dutyCycle)
{
    // PA5
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, dutyCycle);
}

void PWM_SetDutyCycle_CH2(uint16_t dutyCycle)
{
    // PA1
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, dutyCycle);
}

void PWM_SetDutyCycle_CH3(uint16_t dutyCycle)
{
    // PB10
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, dutyCycle);
}

void PWM_SetDutyCycle_CH4(uint16_t dutyCycle)
{
    // PB11
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, dutyCycle);
}

void PWM_Start(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
}

void PWM_Stop(void)
{
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);
}

#define MAX_DUTY_CYCLE 1000

// Initialize RGB configuration with function pointers
// PWM_Single_t ledPwmConfig = {
//     .led = {.setDutyCycle = PWM_SetDutyCycle_CH1}
// };

// PWM_Dual_t ledPwmConfig = {
//     .led1 = {.setDutyCycle = PWM_SetDutyCycle_CH1},
//     .led2 = {.setDutyCycle = PWM_SetDutyCycle_CH2}
// };

PWM_RGB_t ledPwmConfig = {
    .Red   = {.setDutyCycle = PWM_SetDutyCycle_CH1},
    .Green = {.setDutyCycle = PWM_SetDutyCycle_CH2},
    .Blue  = {.setDutyCycle = PWM_SetDutyCycle_CH3},
};

// Initialize LED Color Full Brightness
// const DualColor_t Color = {.Color1 = 255, .Color2 = 255};

const RGB_Color_t Color2 = {.R = 0, .G = 255, .B = 255};

const RGB_Color_t Color = {.R = 0, .G = 0, .B = 255};

// Initialize LED Controller with function pointers
LED_Controller_t LEDController = {.Start        = PWM_Start,
                                  .Stop         = PWM_Stop,
                                  .PwmConfig    = &ledPwmConfig,
                                  .LedType      = LED_TYPE_RGB,
                                  .MaxDutyCycle = MAX_DUTY_CYCLE};

const RGB_Color_t Red    = {.R = 255, .G = 0, .B = 0};
const RGB_Color_t Green  = {.R = 0, .G = 255, .B = 0};
const RGB_Color_t Blue   = {.R = 0, .G = 0, .B = 255};
const RGB_Color_t Purple = {.R = 255, .G = 0, .B = 255};
const RGB_Color_t Yellow = {.R = 255, .G = 255, .B = 0};
const RGB_Color_t Cyan   = {.R = 0, .G = 255, .B = 255};

// Initialize Flash Animation
const LED_Animation_Flash_t globalFlashConfig = {
    .color       = &Red,
    .onTimeMs    = 100,
    .offTimeMs   = 300,
    .repeatTimes = -1 // Repeat ten times
};

const LED_Animation_Blink_t globalBlinkConfig = {
    .color       = &Cyan,
    .periodMs    = 500,
    .repeatTimes = 20 // Repeat ten times
};

const LED_Animation_Solid_t globalSolidConfig = {
    .color           = &Purple,
    .executionTimeMs = 0,
};

const LED_Animation_Breath_t globalBreathConfig = {
    .color = &Yellow, .riseTimeMs = 500, .fallTimeMs = 1000, .repeatTimes = -1, .invert = true};

const LED_Animation_Breath_t globalBreath2Config = {
    .color = &Green, .riseTimeMs = 1000, .fallTimeMs = 1000, .repeatTimes = 5, .invert = false};

const LED_Animation_FadeIn_t globalFadeInConfig = {.color = &Color, .durationMs = 1000, .repeatTimes = 1};

const LED_Animation_FadeOut_t globalFadeOutConfig = {.color = &Color, .durationMs = 1000, .repeatTimes = 1};

const LED_Animation_Pulse_t globalPulseConfig = {
    .color         = &Cyan,
    .riseTimeMs    = 300,
    .holdOnTimeMs  = 200,
    .fallTimeMs    = 300,
    .holdOffTimeMs = 200,
    .repeatTimes   = 10,
};

static const void* AlternatingColors[] = {&Red, &Green, &Blue, &Purple, &Yellow, &Cyan};

// Initialize the alternating colors animation
const LED_Animation_AlternatingColors_t globalAlternatingColorsConfig = {
    .colors      = (void*)AlternatingColors,
    .colorCount  = sizeof(AlternatingColors) / sizeof(AlternatingColors[0]),
    .durationMs  = 1000, // 500 ms for each color
    .repeatTimes = 2,    // Repeat 3 times
};

const LED_Animation_ColorCycle_t globalColorCycleConfig = {.colors = (void*)AlternatingColors,
                                                           .colorCount =
                                                               sizeof(AlternatingColors) / sizeof(AlternatingColors[0]),
                                                           .transitionMs   = 300,
                                                           .holdTimeMs     = 700,
                                                           .repeatTimes    = 2,
                                                           .leaveLastColor = true};

// Global LED Handle for RGB LED
LED_Handle_t            MyLed;
LED_Transition_Handle_t TransitionsHandle;

#define MAX_TRANSITIONS (9)
const LED_Transition_Config_t transitionMapping[MAX_TRANSITIONS] = {

    {.StartAnim      = &globalSolidConfig,
     .TargetAnim     = &globalFlashConfig,
     .TransitionType = LED_TRANSITION_INTERPOLATE,
     .Duration       = 200},

    {.StartAnim      = &globalBreathConfig,
     .TargetAnim     = &globalBreath2Config,
     .TransitionType = LED_TRANSITION_INTERPOLATE},

    {.StartAnim      = &globalBreath2Config,
     .TargetAnim     = &globalBlinkConfig,
     .TransitionType = LED_TRANSITION_AT_CLEAN_ENTRY},

    {.StartAnim = &globalBlinkConfig, .TargetAnim = &globalSolidConfig, .TransitionType = LED_TRANSITION_INTERPOLATE},
    {.StartAnim      = &globalSolidConfig,
     .TargetAnim     = &globalBlinkConfig,
     .TransitionType = LED_TRANSITION_INTERPOLATE,
     .Duration       = 200},

    {.StartAnim = &globalBlinkConfig, .TargetAnim = &globalBreath2Config, .TransitionType = LED_TRANSITION_INTERPOLATE},
    {.StartAnim      = &globalBreath2Config,
     .TargetAnim     = &globalBreathConfig,
     .TransitionType = LED_TRANSITION_INTERPOLATE},

    {.StartAnim = &globalBreathConfig, .TargetAnim = &globalPulseConfig, .TransitionType = LED_TRANSITION_INTERPOLATE},
    {.StartAnim = &globalPulseConfig, .TargetAnim = &globalFlashConfig, .TransitionType = LED_TRANSITION_INTERPOLATE},

};

const char* LED_GetAnimationName(const void* Animation)
{
    if (Animation == &globalFlashConfig)
    {
        return "Flash";
    }
    else if (Animation == &globalBlinkConfig)
    {
        return "Blink";
    }
    else if (Animation == &globalSolidConfig)
    {
        return "Solid";
    }
    else if (Animation == &globalBreathConfig)
    {
        return "Breath";
    }
    else if (Animation == &globalBreath2Config)
    {
        return "Breath2";
    }
    else if (Animation == &globalFadeInConfig)
    {
        return "FadeIn";
    }
    else if (Animation == &globalFadeOutConfig)
    {
        return "FadeOut";
    }
    else if (Animation == &globalPulseConfig)
    {
        return "Pulse";
    }
    else if (Animation == &globalAlternatingColorsConfig)
    {
        return "AlternatingColors";
    }
    else if (Animation == &globalColorCycleConfig)
    {
        return "ColorCycle";
    }
    else if (Animation == NULL)
    {
        return "Off";
    }
    else
    {
        return "Unknown";
    }
}

/** @brief Array of LED animation type names for debugging purposes. */
const char* LED_AnimationTypeNames[] = {
    "Type INVALID",            // LED_ANIMATION_TYPE_INVALID
    "Type None",               // LED_ANIMATION_TYPE_NONE
    "Type Off",                // LED_ANIMATION_TYPE_OFF
    "Type Solid",              // LED_ANIMATION_TYPE_SOLID
    "Type Blink",              // LED_ANIMATION_TYPE_BLINK
    "Type Flash",              // LED_ANIMATION_TYPE_FLASH
    "Type Breath",             // LED_ANIMATION_TYPE_BREATH
    "Type Pulse",              // LED_ANIMATION_TYPE_PULSE
    "Type Fade In",            // LED_ANIMATION_TYPE_FADE_IN
    "Type Fade Out",           // LED_ANIMATION_TYPE_FADE_OUT
    "Type Alternating Colors", // LED_ANIMATION_TYPE_ALTERNATING_COLORS
    "Type Color Cycle"         // LED_ANIMATION_TYPE_COLOR_CYCLE
};

void LED_Complete_Callback(LED_Animation_Type_t animationType, LED_Status_t status, const void* Animation)
{
    // Print a separator for clarity in the output
    // Switch statement to handle different status cases
    switch (status)
    {
    case LED_STATUS_ANIMATION_STARTED:

        UART_printf("Animation [%s] Started : %s\r\n", LED_AnimationTypeNames[animationType],
                    LED_GetAnimationName(Animation));
        break;

    case LED_STATUS_ANIMATION_COMPLETED:
        UART_printf("Animation [%s] Completed : %s\r\n", LED_AnimationTypeNames[animationType],
                    LED_GetAnimationName(Animation));
        break;

    case LED_STATUS_ANIMATION_STOPPED:
        UART_printf("Animation [%s] Stopped : %s\r\n", LED_AnimationTypeNames[animationType],
                    LED_GetAnimationName(Animation));
        break;

    case LED_STATUS_ANIMATION_TRANSITION_STARTED:
        UART_printf("----------------------------------------------------------------\r\n");

        UART_printf("Animation [%s] Transition Started : %s\r\n", LED_AnimationTypeNames[animationType],
                    LED_GetAnimationName(Animation));
        break;

    case LED_STATUS_ANIMATION_TRANSITION_COMPLETED:
        UART_printf("Animation [%s] Transition Completed : %s\r\n", LED_AnimationTypeNames[animationType],
                    LED_GetAnimationName(Animation));
        break;

    case LED_STATUS_ANIMATION_TRANSITION_SKIPPED:
        UART_printf("Animation [%s] Transition Skipped : %s\r\n", LED_AnimationTypeNames[animationType],
                    LED_GetAnimationName(Animation));
        break;

    default:
        if (IS_LED_ERROR_STATUS(status))
        {
            UART_printf("Animation [%s] Failed, Error %d\r\n", LED_AnimationTypeNames[animationType], status);
        }
        else
        {
            UART_printf("Animation [%s] Unknown Status %d\r\n", LED_AnimationTypeNames[animationType], status);
        }
        break;
    }

    // Print a separator to mark the end of the message
}

void InitDWT(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // Enable the trace and debug blocks
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;            // Enable the cycle count
    DWT->CYCCNT = 0;                                // Reset the cycle counter
}

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void        SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
#if 1
int main(void)
{

    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU
     * Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the
     * Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_TIM2_Init();
    /* USER CODE BEGIN 2 */
    InitDWT();

    // Initialize the LED Handle with the controller, LED type
    LED_Animation_Init(&MyLed, &LEDController, LED_Complete_Callback);

    // Intialize the LED Transition Handle
    LED_Transition_Init(&TransitionsHandle, &MyLed);

    // Set the blink animation for the single LED
    //    LED_Animation_SetSolid(&MyLed, &globalSolidConfig);
    // LED_Animation_SetFlash(&MyLed, &globalFlashConfig);
    // LED_Animation_SetBlink(&MyLed, &globalBlinkConfig);
    // LED_Animation_SetFadeIn(&MyLed, &globalFadeInConfig);
    // LED_Animation_SetFadeOut(&MyLed, &globalFadeOutConfig);
    // LED_Animation_SetBreath(&MyLed, &globalBreathConfig);
    // LED_Animation_SetPulse(&MyLed, &globalPulseConfig);
    // LED_Animation_SetAlternatingColors(&MyLed, &globalAlternatingColorsConfig);
    // LED_Animation_SetColorCycle(&MyLed, &globalColorCycleConfig);

    // LED_Transition_SetMapping(&TransitionsHandle, transitionMapping, MAX_TRANSITIONS);

    // LED_Animation_Start(&MyLed);

    /* USER CODE END 2 */
    static uint32_t lastTick = 0;
    bool            send     = false;
    uint8_t         counter  = 0;
    /* Infinite loop */
    /* USER CODE BEGIN WHILE */

    // LED_Transition_ExecAnimation(&TransitionsHandle, &globalColorCycleConfig, LED_ANIMATION_TYPE_COLOR_CYCLE);

    // LED_Transition_ExecAnimation(&TransitionsHandle, &globalSolidConfig, LED_ANIMATION_TYPE_SOLID);

    // LED_Transition_ToPulse(&TransitionsHandle, &globalPulseConfig, LED_TRANSITION_INTERPOLATE, 200);
    // LED_Transition_ToPulse(&TransitionsHandle, &globalPulseConfig, LED_TRANSITION_IMMINENT, 0);
    // LED_Transition_ToBlink(&TransitionsHandle, &globalBlinkConfig, LED_TRANSITION_IMMINENT, 0);

    while (1)
    {
        // LED_Transition_ToPulse(&TransitionsHandle, &globalPulseConfig, LED_TRANSITION_IMMINENT, 0);
        // LED_Transition_ToSolid(&TransitionsHandle, &globalSolidConfig, LED_TRANSITION_IMMINENT, 0);
        // LED_Transition_ToBlink(&TransitionsHandle, &globalBlinkConfig, LED_TRANSITION_IMMINENT, 0);
        // LED_Transition_ToColorCycle(&TransitionsHandle, &globalColorCycleConfig, LED_TRANSITION_INTERPOLATE, 300);

        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        // LED_Animation_Update(&MyLed, HAL_GetTick());
        LED_Transition_Update(&TransitionsHandle, HAL_GetTick());

#if 1

        if (HAL_GetTick() - lastTick > 5000)
        {
            lastTick = HAL_GetTick();

            switch (counter)
            {
            case 0:
                // Start the LED Animation
                LED_Transition_ExecuteWithMap(&TransitionsHandle, &globalSolidConfig, LED_ANIMATION_TYPE_SOLID);
                break;
            case 1:
                // Transition from solid to breath
                LED_Transition_ExecuteWithMap(&TransitionsHandle, &globalBlinkConfig, LED_ANIMATION_TYPE_BLINK);
                break;
            case 2:
                // Transition from breath to breath2
                LED_Transition_ExecuteWithMap(&TransitionsHandle, &globalBreath2Config, LED_ANIMATION_TYPE_BREATH);
                break;
            case 3:
                // Transition from breath2 to blink
                LED_Transition_ExecuteWithMap(&TransitionsHandle, &globalBreathConfig, LED_ANIMATION_TYPE_BREATH);
                break;
            case 4:
                // Transition from blink to solid
                LED_Transition_ExecuteWithMap(&TransitionsHandle, &globalPulseConfig, LED_ANIMATION_TYPE_PULSE);
                break;
            case 5:
                LED_Transition_ExecuteWithMap(&TransitionsHandle, &globalFlashConfig, LED_ANIMATION_TYPE_FLASH);
                break;
            case 6:
                LED_Transition_ExecuteWithMap(&TransitionsHandle, &globalAlternatingColorsConfig,
                                              LED_ANIMATION_TYPE_ALTERNATING_COLORS);
                break;
            case 7:
                LED_Transition_ExecuteWithMap(&TransitionsHandle, &globalColorCycleConfig,
                                              LED_ANIMATION_TYPE_COLOR_CYCLE);
                break;
            case 8:
                LED_Transition_ToOff(&TransitionsHandle, LED_TRANSITION_INTERPOLATE, 200);
                break;
            case 9:
                UART_printf("Check if LED is off\r\n");
                if (LED_Transition_IsLEDOff(&TransitionsHandle))
                {
                    UART_printf("LED is off\r\n");
                }
                LED_Transition_ToOff(&TransitionsHandle, LED_TRANSITION_INTERPOLATE, 200);
                break;

            case 10:
                LED_Transition_ToPulse(&TransitionsHandle, &globalPulseConfig, LED_TRANSITION_INTERPOLATE, 200);
                break;
            case 11:
                LED_Transition_ToPulse(&TransitionsHandle, &globalPulseConfig, LED_TRANSITION_IMMINENT, 0);
                break;
            case 12:
                LED_Status_t status = LED_STATUS_SUCCESS;
                UART_printf("Transition to Blink\r\n");
                status =
                    LED_Transition_ToBlink(&TransitionsHandle, &globalBlinkConfig, LED_TRANSITION_INTERPOLATE, 500);

                if (status != LED_STATUS_SUCCESS)
                {
                    UART_printf("Error %d\r\n", status);
                }
                else
                {
                    UART_printf("Success\r\n");
                }

                UART_printf("Transition to Solid\r\n");
                status =
                    LED_Transition_ToSolid(&TransitionsHandle, &globalSolidConfig, LED_TRANSITION_INTERPOLATE, 500);
                if (status != LED_STATUS_SUCCESS)
                {
                    UART_printf("Error %d\r\n", status);
                }
                else
                {
                    UART_printf("Success\r\n");
                }

                UART_printf("Force Stop Transition\r\n");
                LED_Transition_Stop(&TransitionsHandle);

                UART_printf("Transition to Flash\r\n");
                status =
                    LED_Transition_ToFlash(&TransitionsHandle, &globalFlashConfig, LED_TRANSITION_INTERPOLATE, 500);
                if (status != LED_STATUS_SUCCESS)
                {
                    UART_printf("Error %d\r\n", status);
                }
                else
                {
                    UART_printf("Success\r\n");
                }

                break;

            default:
                break;
            }

            counter = (counter + 1) % 13;
        }
#endif
    }
    /* USER CODE END 3 */
}
#else
#include "pwm_led.h"

PWMLED_Handle_t pwmLedHandle;

// Command the LED to breath
PWMLED_Breath_Handle_t breathCmd = {.Duty = 255, .PeriodTicks = 1000, .HoldInTicks = 200, .HoldOutTicks = 200};

void updateHardwarePWMDuty(uint16_t dutyValue)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, dutyValue);
}

int main(void)
{

    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU
     * Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the
     * Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_TIM2_Init();
    /* USER CODE BEGIN 2 */
    InitDWT();

    pwmLedHandle.MaxPulse     = 1000;            // Set according to your PWM resolution
    pwmLedHandle.ActiveConfig = LED_Active_HIGH; // Set this based on your LED configuration

    PWMLED_Init(&pwmLedHandle);

    PWMLED_Command_Breath(&pwmLedHandle, &breathCmd, true);

    PWM_Start();

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        updateHardwarePWMDuty(pwmLedHandle.PWMDuty);

        /* USER CODE END 3 */
    }
}
#endif

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = 1;
    RCC_OscInitStruct.PLL.PLLN            = 10;
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ            = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR            = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void)
{

    /* USER CODE BEGIN TIM2_Init 0 */

    /* USER CODE END TIM2_Init 0 */

    TIM_ClockConfigTypeDef  sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig      = {0};
    TIM_OC_InitTypeDef      sConfigOC          = {0};

    /* USER CODE BEGIN TIM2_Init 1 */

    /* USER CODE END TIM2_Init 1 */
    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 79;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 999;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_OC_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_TIMING;
    if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM2_Init 2 */

    /* USER CODE END TIM2_Init 2 */
    HAL_TIM_MspPostInit(&htim2);
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{

    /* USER CODE BEGIN USART2_Init 0 */

    /* USER CODE END USART2_Init 0 */

    /* USER CODE BEGIN USART2_Init 1 */

    /* USER CODE END USART2_Init 1 */
    huart2.Instance                    = USART2;
    huart2.Init.BaudRate               = 115200;
    huart2.Init.WordLength             = UART_WORDLENGTH_8B;
    huart2.Init.StopBits               = UART_STOPBITS_1;
    huart2.Init.Parity                 = UART_PARITY_NONE;
    huart2.Init.Mode                   = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling           = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN USART2_Init 2 */

    /* USER CODE END USART2_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* USER CODE BEGIN MX_GPIO_Init_1 */
    /* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin : B1_Pin */
    GPIO_InitStruct.Pin  = B1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

    /* USER CODE BEGIN MX_GPIO_Init_2 */
    /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state
     */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line
       number, ex: printf("Wrong parameters value: file %s on line %d\r\n",
       file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
