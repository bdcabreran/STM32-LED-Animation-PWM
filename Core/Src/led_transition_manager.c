#include "led_transition_manager.h"

#define DEBUG_BUFFER_SIZE 100
#define LED_TRANSITION_DBG 1

#if LED_TRANSITION_DBG
#include "stm32l4xx_hal.h"
#include <stdio.h>
extern UART_HandleTypeDef huart2;
#define LED_TRANSITION_DBG_MSG(fmt, ...)                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        static char dbgBuff[DEBUG_BUFFER_SIZE];                                                                        \
        snprintf(dbgBuff, DEBUG_BUFFER_SIZE, (fmt), ##__VA_ARGS__);                                                    \
        HAL_UART_Transmit(&huart2, (uint8_t*)dbgBuff, strlen(dbgBuff), 1000);                                          \
    } while (0)
#else
#define LED_TRANSITION_DBG_MSG(fmt, ...)                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#endif

LED_Status_t LED_Transition_Init(LED_Transition_Handle_t* this, LED_Handle_t* LedHandle)
{
    if (this == NULL || LedHandle == NULL)
    {
        return LED_STATUS_ERROR_NULL_POINTER;
    }

    this->transitionMap  = NULL;
    this->mapSize        = 0;
    this->LedHandle      = LedHandle;
    this->state          = LED_TRANSITION_STATE_IDLE;
    this->targetAnimData = NULL;
    this->targetAnimType = LED_ANIMATION_TYPE_INVALID;

    LED_TRANSITION_DBG_MSG("LED Transition Manager Initialized\r\n");
    return LED_STATUS_SUCCESS;
}

LED_Status_t LED_Transition_SetMapping(LED_Transition_Handle_t* this, const void* transitionsMap, uint32_t mapSize)
{
    if (this == NULL || transitionsMap == NULL || mapSize == 0)
    {
        return LED_STATUS_ERROR_NULL_POINTER;
    }

    this->transitionMap = transitionsMap;
    this->mapSize       = mapSize;

    LED_TRANSITION_DBG_MSG("LED Transition Mapping Set\r\n");
    return LED_STATUS_SUCCESS;
}

static void LED_Transition_SetNextState(LED_Transition_Handle_t* this, LED_Transition_State_t nextState)
{
    if (IS_VALID_TRANSITION_STATE(nextState))
    {
        this->state = nextState;
    }
}

static LED_Status_t LED_Transition_StateIdle(LED_Transition_Handle_t* this)
{
    /*Do nothing*/
    return LED_STATUS_SUCCESS;
}

/**
 * @brief Sets up the duration for the given transition type.
 *
 * @param transitionType The type of LED transition.
 * @param duration The specified duration for the transition. If zero, a default value is used.
 * @return The duration to be used for the transition.
 */
static uint16_t LED_Transition_SetupDuration(LED_Transition_Type_t transitionType, uint16_t duration)
{
    switch (transitionType)
    {
    case LED_TRANSITION_IMMINENT:
        // Perform imminent transition
        return 0;

    case LED_TRANSITION_INTERPOLATE:
        // Perform interpolation transition
        return (duration == 0) ? DEFAULT_TRANSITION_INTERPOLATE_TIME_MS : duration;

    case LED_TRANSITION_UPON_COMPLETION:
        // Perform upon completion transition
        return (duration == 0) ? DEFAULT_TRANSITION_UPON_COMPLETION_TIMEOUT_MS : duration;

    case LED_TRANSITION_AT_CLEAN_ENTRY:
        // Perform at clean entry transition
        return (duration == 0) ? DEFAULT_TRANSITION_CLEAN_ENTRY_TIMEOUT_MS : duration;

    default:
        // Handle invalid transition type
        return 0;
    }
}

static bool LED_Transition_FindTransition(LED_Transition_Handle_t* this)
{
    for (uint32_t i = 0; i < this->mapSize; i++)
    {
        LED_Transition_Config_t* transition = (LED_Transition_Config_t*)this->transitionMap + i;
        if (transition->StartAnim == this->LedHandle->animationData && transition->EndAnim == this->targetAnimData)
        {
            this->transitionType = transition->TransitionType;
            this->Duration       = LED_Transition_SetupDuration(transition->TransitionType, transition->Duration);

            LED_TRANSITION_DBG_MSG(
                "Transition found in the map, %d, duration %d\r\n", this->transitionType, this->Duration);

            return true;
        }
    }
    return false;
}

static void LED_Transition_SetDefaultType(LED_Transition_Handle_t* this)
{
    LED_TRANSITION_DBG_MSG("Transition not found in the map, using default transition type\r\n");
    this->transitionType = LED_TRANSITION_INTERPOLATE;
    this->Duration       = DEFAULT_TRANSITION_INTERPOLATE_TIME_MS;
}

static void LED_Transition_HandleInterpolate(LED_Transition_Handle_t* this)
{
    uint32_t colorCount = CalculateColorCount(this->LedHandle->controller->LedType);
    LED_TRANSITION_DBG_MSG("LED Color Count: %d\r\n", colorCount);

    LED_Animation_GetCurrentColor(this->LedHandle, this->currentColor, colorCount);
    LED_Animation_GetTargetColor(this->targetAnimData, this->targetAnimType, this->targetColor, colorCount);

    bool startHigh = LED_Animation_ShouldStartHigh(this->targetAnimType, this->targetAnimData);

    if (!startHigh)
    {
        LED_TRANSITION_DBG_MSG("Target Animation starts low, setting target color to 0\r\n");
        for (uint8_t i = 0; i < colorCount; i++)
        {
            this->targetColor[i] = 0;
        }
    }

    // Print Color Current and Target
    for (uint8_t i = 0; i < colorCount; i++)
    {
        LED_TRANSITION_DBG_MSG("Color Current: %d, Target: %d\r\n", this->currentColor[i], this->targetColor[i]);
    }

    // ensure the LED PWM is active
    this->LedHandle->controller->Start();
}

static void LED_Transition_CallCallbackIfExists(LED_Transition_Handle_t* this, LED_Status_t Status)
{
    if (this->LedHandle->callback != NULL)
    {
        this->LedHandle->callback(this->LedHandle->animationType, Status);
    }
}

static LED_Status_t LED_Transition_StateSetup(LED_Transition_Handle_t* this, uint32_t tick)
{
    bool transitionFound = LED_Transition_FindTransition(this);

    if (!transitionFound)
    {
        LED_Transition_SetDefaultType(this);
    }

    if (this->transitionType == LED_TRANSITION_INTERPOLATE)
    {
        LED_Transition_HandleInterpolate(this);
    }

    this->lastTick = tick;

    LED_Transition_CallCallbackIfExists(this, LED_STATUS_ANIMATION_TRANSITION_STARTED);

    LED_Transition_SetNextState(this, LED_TRANSITION_STATE_ONGOING);

    return LED_STATUS_SUCCESS;
}

bool AreColorsOff(uint8_t* colors, uint8_t colorCount)
{
    for (uint8_t i = 0; i < colorCount; i++)
    {
        if (colors[i] != 0)
        {
            return false;
        }
    }
    return true;
}

static void PerformLinearInterpolation(LED_Transition_Handle_t* this, uint32_t elapsed)
{
    // Get Color Count
    uint8_t colorCount = CalculateColorCount(this->LedHandle->controller->LedType);

    // Calculate the interpolated color
    uint32_t timeInCycle = elapsed % this->Duration;

    // Calculate interpolation factor (t) based on elapsed time and scale it
    uint32_t fraction_scaled = (timeInCycle * 1000) / this->Duration;

    // Ensure fraction_scaled is between 0 and 1000
    if (fraction_scaled > 1000)
    {
        fraction_scaled = 1000;
    }

    uint8_t* currentColor = this->currentColor;
    uint8_t* targetColor  = this->targetColor;
    uint8_t  interpolatedColor[colorCount];

    for (uint8_t i = 0; i < colorCount; i++)
    {
        interpolatedColor[i] =
            currentColor[i] + ((int32_t)(fraction_scaled * (int32_t)(targetColor[i] - currentColor[i])) / 1000);
    }

#if 0
    // Print interpolated color
    LED_TRANSITION_DBG_MSG("Interpolated Color (Linear): ");
    for (uint8_t i = 0; i < colorCount; i++)
    {
        LED_TRANSITION_DBG_MSG("%d ", interpolatedColor[i]);
    }
    LED_TRANSITION_DBG_MSG("\r\n");
#endif

    // Update the LED handle with the interpolated color
    LED_Animation_ExecuteColorSetting(this->LedHandle, interpolatedColor);
}

static void PerformQuadraticInterpolation(LED_Transition_Handle_t* this, uint32_t elapsed)
{
    // Get Color Count
    uint8_t colorCount = CalculateColorCount(this->LedHandle->controller->LedType);

    // Calculate the interpolated color
    uint32_t timeInCycle = elapsed % this->Duration;

    // Calculate interpolation factor (t) based on elapsed time and scale it
    uint32_t fraction_scaled = (timeInCycle * 1000) / this->Duration;

    // Ensure fraction_scaled is between 0 and 1000
    if (fraction_scaled > 1000)
    {
        fraction_scaled = 1000;
    }

    // Convert the scaled fraction to a float between 0.0 and 1.0
    float t = fraction_scaled / 1000.0f;

    // Apply quadratic interpolation by squaring the interpolation factor
    float t_squared = t * t;

    uint8_t* currentColor = this->currentColor;
    uint8_t* targetColor  = this->targetColor;
    uint8_t  interpolatedColor[colorCount];

    for (uint8_t i = 0; i < colorCount; i++)
    {
        interpolatedColor[i] = currentColor[i] + (int32_t)((targetColor[i] - currentColor[i]) * t_squared);
    }

// Print interpolated color
#if 0
    LED_TRANSITION_DBG_MSG("Interpolated Color (Quadratic): ");
    for (uint8_t i = 0; i < colorCount; i++)
    {
        LED_TRANSITION_DBG_MSG("%d ", interpolatedColor[i]);
    }
    LED_TRANSITION_DBG_MSG("\r\n");
#endif

    // Update the LED handle with the interpolated color
    LED_Animation_ExecuteColorSetting(this->LedHandle, interpolatedColor);
}

static LED_Status_t LED_Transition_StateCompleted(LED_Transition_Handle_t* this)
{
    LED_Transition_CallCallbackIfExists(this, LED_STATUS_ANIMATION_TRANSITION_COMPLETED);

    LED_Animation_SetAnimation(this->LedHandle, this->targetAnimData, this->targetAnimType);
    LED_Animation_Start(this->LedHandle);

    this->targetAnimData = NULL;
    this->targetAnimType = LED_ANIMATION_TYPE_INVALID;
    this->transitionType = LED_TRANSITION_INVALID;
    LED_Transition_SetNextState(this, LED_TRANSITION_STATE_IDLE);
}

static LED_Status_t LED_Transition_StateOngoing(LED_Transition_Handle_t* this, uint32_t tick)
{
    uint32_t elapsed = tick - this->lastTick;

    // Execute the transition
    switch (this->transitionType)
    {
    case LED_TRANSITION_IMMINENT:
    {
        LED_TRANSITION_DBG_MSG("Transitioning Immediately\r\n");
        LED_Transition_SetNextState(this, LED_TRANSITION_STATE_COMPLETED);
    }
    break;

    case LED_TRANSITION_INTERPOLATE:
    {
        if (elapsed >= this->Duration)
        {
            LED_TRANSITION_DBG_MSG("Interpolation Completed\r\n");

            // Transition to idle state
            LED_Transition_SetNextState(this, LED_TRANSITION_STATE_COMPLETED);
        }
        else
        {
#if USE_LINEAR_INTERPOLATION
            PerformLinearInterpolation(this, elapsed);
#endif

#if USE_QUADRATIC_INTERPOLATION
            PerformQuadraticInterpolation(this, elapsed);
#endif
        }
    }
    break;

    case LED_TRANSITION_UPON_COMPLETION:
    {
        // safe guard to prevent infinite loop
        if (elapsed > this->Duration)
        {
            LED_TRANSITION_DBG_MSG("Upon Completion Error, Timeout\r\n");
            LED_Transition_SetNextState(this, LED_TRANSITION_STATE_COMPLETED);
        }
        else if (this->LedHandle->isRunning == false)
        {
            LED_TRANSITION_DBG_MSG("Transitioning on Completion\r\n");
            LED_Transition_SetNextState(this, LED_TRANSITION_STATE_COMPLETED);
        }
    }
    break;

    case LED_TRANSITION_AT_CLEAN_ENTRY:
    {
        uint8_t colorCount = CalculateColorCount(this->LedHandle->animationType);
        uint8_t color[colorCount];
        LED_Animation_GetCurrentColor(this->LedHandle, color, &colorCount);

        if (AreColorsOff(color, colorCount))
        {
            LED_TRANSITION_DBG_MSG("Transitioning on Off\r\n");
            LED_Transition_SetNextState(this, LED_TRANSITION_STATE_COMPLETED);
        }
        // safe guard to prevent infinite loop
        else if (elapsed > this->Duration)
        {
            LED_TRANSITION_DBG_MSG("Clean Entry Error, Timeout\r\n");
            LED_Transition_SetNextState(this, LED_TRANSITION_STATE_COMPLETED);
        }
    }
    break;
    }

    return LED_STATUS_SUCCESS;
}

LED_Status_t LED_Transition_Update(LED_Transition_Handle_t* this, uint32_t tick)
{
    if (this == NULL)
    {
        return LED_STATUS_ERROR_NULL_POINTER;
    }
    switch (this->state)
    {
    case LED_TRANSITION_STATE_IDLE:
        LED_Transition_StateIdle(this);
        break;
    case LED_TRANSITION_STATE_SETUP:
        LED_Transition_StateSetup(this, tick);
        break;
    case LED_TRANSITION_STATE_ONGOING:
        LED_Transition_StateOngoing(this, tick);
        break;
    case LED_TRANSITION_STATE_COMPLETED:
        LED_Transition_StateCompleted(this);
        break;
    default:
        break;
    }

    if (this->transitionType != LED_TRANSITION_INTERPOLATE)
    {
        LED_Animation_Update(this->LedHandle, tick);
    }

    return LED_STATUS_SUCCESS;
}

LED_Status_t
LED_Transition_ExecAnimation(LED_Transition_Handle_t* this, const void* animData, LED_Animation_Type_t animType)
{
    if (this == NULL || animData == NULL)
    {
        return LED_STATUS_ERROR_NULL_POINTER;
    }

    if (this->state == LED_TRANSITION_STATE_IDLE)
    {
        this->targetAnimData = animData;
        this->targetAnimType = animType;
        LED_Transition_SetNextState(this, LED_TRANSITION_STATE_SETUP);
        return LED_STATUS_SUCCESS;
    }
    else
    {
        return LED_STATUS_ERROR_BUSY;
    }
}
