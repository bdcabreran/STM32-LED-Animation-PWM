#include "led_transition_manager.h"

#define DEBUG_BUFFER_SIZE 100
#define LED_TRANSITION_DBG 0

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
    this->transitionType = LED_TRANSITION_INVALID;

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

static bool LED_Transition_FindMapTransition(LED_Transition_Handle_t* this)
{
    for (uint32_t i = 0; i < this->mapSize; i++)
    {
        LED_Transition_Config_t* transition = (LED_Transition_Config_t*)this->transitionMap + i;
        if (transition->StartAnim == this->LedHandle->animationData && transition->TargetAnim == this->targetAnimData)
        {
            this->transitionType = transition->TransitionType;
            this->Duration       = LED_Transition_SetupDuration(transition->TransitionType, transition->Duration);

            LED_TRANSITION_DBG_MSG("MAP Transition found, %d, duration %d\r\n", this->transitionType, this->Duration);

            return true;
        }
    }
    return false;
}

static bool LED_Transition_FindRegularTransition(LED_Transition_Handle_t* this)
{
    if (IS_VALID_LED_ANIMATION_TYPE(this->targetAnimType))
    {
        if (IS_VALID_TRANSITION_TYPE(this->transitionType))
        {
            LED_TRANSITION_DBG_MSG("Regular Transition found, %d, duration %d\r\n", this->transitionType,
                                   this->Duration);
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

/**
 * @brief Prepares the LED handle for an interpolation transition.
 *
 * This function checks if the current LED colors match the target colors.
 * If they do, the transition is skipped. Otherwise, it configures the target colors
 * and ensures that the LED PWM controller is active.
 *
 * @param handle Pointer to the LED transition handle.
 * @return True if the colors match and transition is skipped, false otherwise.
 */
static bool LED_Transition_ConfigureInterpolation(LED_Transition_Handle_t* handle)
{
    // Get the number of color channels based on the LED type
    uint32_t colorCount = LED_Animation_GetColorCount(handle->LedHandle->controller->LedType);
    LED_TRANSITION_DBG_MSG("LED Color Count: %d\r\n", colorCount);

    // Retrieve current and target colors
    LED_Animation_GetCurrentColor(handle->LedHandle, handle->currentColor, colorCount);
    LED_Animation_GetTargetColor((const void*)handle->targetAnimData, handle->targetAnimType, handle->targetColor,
                                 colorCount);

    // Check if the target animation should start at high or low intensity
    bool startHigh = LED_Animation_ShouldStartHigh(handle->targetAnimType, (const void*)handle->targetAnimData);
    LED_TRANSITION_DBG_MSG("Target Animation Start: %s\r\n", startHigh ? "High" : "Low");

    // If Target Animation starting low, set target colors to off (0x00)
    if (!startHigh)
    {
        for (uint8_t i = 0; i < colorCount; i++)
        {
            handle->targetColor[i] = 0x00;
        }
    }

    // Compare current and target colors
    bool colorsMatch = true;
    for (uint8_t i = 0; i < colorCount; i++)
    {
        if (handle->currentColor[i] != handle->targetColor[i])
        {
            colorsMatch = false; // Colors do not match, transition is needed
        }
        LED_TRANSITION_DBG_MSG("Color Channel [%d]: Current = %d, Target = %d\r\n", i, handle->currentColor[i],
                               handle->targetColor[i]);
    }

    // If colors match, skip the transition
    if (colorsMatch)
    {
        LED_TRANSITION_DBG_MSG("Colors match, skipping the transition.\r\n");
        return true; // Transition skipped
    }

    // Ensure the LED PWM is active if a transition is needed
    handle->LedHandle->controller->Start();
    LED_TRANSITION_DBG_MSG("LED PWM Controller started for interpolation transition.\r\n");

    return false; // Transition needed, colors did not match
}

static void LED_Transition_CallCallbackIfExists(LED_Transition_Handle_t* this, LED_Status_t Status)
{
    if (this->LedHandle->callback != NULL)
    {
        this->LedHandle->callback(this->targetAnimType, Status, (void*)this->targetAnimData);
    }
}

/**
 * @brief Sets up the LED transition state based on the current animation type and mapping.
 *
 * This function finds the appropriate transition type, sets up the necessary configuration,
 * and checks if an interpolation transition is required. If the colors already match, the
 * transition is skipped, and the state is moved to completion.
 *
 * @param handle Pointer to the LED transition handle.
 * @param tick   Current tick value.
 * @return LED status indicating the success or failure of the operation.
 */
static LED_Status_t LED_Transition_StateSetup(LED_Transition_Handle_t* handle, uint32_t tick)
{
    // Try to find a regular transition first
    bool transitionFound = LED_Transition_FindRegularTransition(handle);

    // If no regular transition is found, try finding a mapped transition
    if (!transitionFound)
    {
        transitionFound = LED_Transition_FindMapTransition(handle);
    }

    // If no transition is found in the map, set the default transition type
    if (!transitionFound)
    {
        LED_Transition_SetDefaultType(handle);
    }

    // Check if the transition type is interpolation and configure it
    if (handle->transitionType == LED_TRANSITION_INTERPOLATE)
    {
        bool colorsMatch = LED_Transition_ConfigureInterpolation(handle);

        if (colorsMatch)
        {
            // Colors match, no need to interpolate, move directly to completed state
            LED_TRANSITION_DBG_MSG("Colors match, transition completed immediately.\r\n");

            // Call the transition started callback
            LED_Transition_CallCallbackIfExists(handle, LED_STATUS_ANIMATION_TRANSITION_STARTED);

            // Transition is complete, so move to the completed state
            LED_Transition_SetNextState(handle, LED_TRANSITION_STATE_COMPLETED);

            return LED_STATUS_SUCCESS;
        }
    }

    // Set the last tick for tracking elapsed time
    handle->lastTick = tick;

    // Call the transition started callback
    LED_Transition_CallCallbackIfExists(handle, LED_STATUS_ANIMATION_TRANSITION_STARTED);

    // Move to the ongoing state to continue the transition
    LED_Transition_SetNextState(handle, LED_TRANSITION_STATE_ONGOING);

    return LED_STATUS_SUCCESS;
}

bool AreColorsOff(uint8_t* colors, uint8_t colorCount)
{
    for (uint8_t i = 0; i < colorCount; i++)
    {
        if (colors[i] != 0x00)
        {
            return false;
        }
    }
    return true;
}

static LED_Status_t LED_Transition_StateCompleted(LED_Transition_Handle_t* this)
{
    LED_Transition_CallCallbackIfExists(this, LED_STATUS_ANIMATION_TRANSITION_COMPLETED);

    LED_Animation_SetAnimation(this->LedHandle, (void*)this->targetAnimData, this->targetAnimType);
    LED_Animation_Start(this->LedHandle);

    this->targetAnimData = NULL;
    this->targetAnimType = LED_ANIMATION_TYPE_INVALID;
    this->transitionType = LED_TRANSITION_INVALID;
    LED_Transition_SetNextState(this, LED_TRANSITION_STATE_IDLE);

    return LED_STATUS_SUCCESS;
}

bool LED_Animation_CheckAndForceColorMatch(LED_Handle_t* ledHandle, uint8_t* targetColor)
{
    // Get the number of colors
    uint8_t colorCount = LED_Animation_GetColorCount(ledHandle->controller->LedType);
    uint8_t color[colorCount];

    // Get the current color
    LED_Animation_GetCurrentColor(ledHandle, color, colorCount);

    // Check if the colors match
    bool colorsMatch = true;
    for (uint8_t i = 0; i < colorCount; i++)
    {
        if (color[i] != targetColor[i])
        {
            colorsMatch = false;
            LED_TRANSITION_DBG_MSG("Color [%d] Current: %d, Target: %d\r\n", i, color[i], targetColor[i]);
            break;
        }
        LED_TRANSITION_DBG_MSG("Color [%d] Current: %d, Target: %d\r\n", i, color[i], targetColor[i]);
    }

    // Print result and force color if necessary
    if (colorsMatch)
    {
        LED_TRANSITION_DBG_MSG("Transition Completed, Colors Match\r\n");
    }
    else
    {
        LED_TRANSITION_DBG_MSG("Transition Completed, Colors Do Not Match\r\n");
        // Force the target color
        LED_Animation_ExecuteColorSetting(ledHandle, targetColor);
    }

    return colorsMatch;
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

            // Interpolation sometimes does not reach the target color exactly, the error is usually very small
            if (LED_Animation_CheckAndForceColorMatch(this->LedHandle, this->targetColor))
            {
                LED_TRANSITION_DBG_MSG("Colors successfully transitioned.\r\n");
            }
            else
            {
                LED_TRANSITION_DBG_MSG("Colors forced to target.\r\n");
            }

            // Transition to idle state
            LED_Transition_SetNextState(this, LED_TRANSITION_STATE_COMPLETED);
        }
        else
        {
#if USE_LINEAR_INTERPOLATION
            LED_Animation_PerformLinearInterpolation(this->LedHandle, elapsed, this->Duration, this->currentColor,
                                                     this->targetColor);
#endif

#if USE_QUADRATIC_INTERPOLATION
            LED_Animation_PerformQuadraticInterpolation(this->LedHandle, elapsed, this->Duration, this->currentColor,
                                                        this->targetColor);
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
        uint8_t colorCount = LED_Animation_GetColorCount(this->LedHandle->controller->LedType);
        uint8_t color[colorCount];
        LED_Animation_GetCurrentColor(this->LedHandle, color, colorCount);

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

    default:
        LED_TRANSITION_DBG_MSG("Invalid Transition Type\r\n");
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

LED_Status_t LED_Transition_ExecuteWithMap(LED_Transition_Handle_t* this, const void* animData,
                                           LED_Animation_Type_t animType)
{
    return LED_Transition_Execute(this, animData, animType, LED_TRANSITION_INVALID, 0);
}

LED_Status_t LED_Transition_Execute(LED_Transition_Handle_t* handle, const void* animData,
                                    LED_Animation_Type_t animType, LED_Transition_Type_t transitionType,
                                    uint16_t duration)
{
    if (handle == NULL)
    {
        return LED_STATUS_ERROR_NULL_POINTER;
    }

    // Check if the LED is already in the target state
    if (animType == LED_ANIMATION_TYPE_OFF && LED_Transition_IsLEDOff(handle))
    {
        // Notify that the transition was skipped
        if (handle->LedHandle->callback != NULL)
        {
            handle->LedHandle->callback(animType, LED_STATUS_ANIMATION_TRANSITION_SKIPPED, (void*)animData);
        }
        return LED_STATUS_SUCCESS;
    }

    if (handle->state == LED_TRANSITION_STATE_IDLE)
    {
        // If there is an ongoing led animation, stop it
        if (handle->LedHandle->isRunning)
        {
            LED_Animation_Stop(handle->LedHandle, true);
        }

        handle->targetAnimData = animData;
        handle->targetAnimType = animType;
        handle->transitionType = transitionType;
        handle->Duration       = duration;
        LED_Transition_SetNextState(handle, LED_TRANSITION_STATE_SETUP);
        return LED_STATUS_SUCCESS;
    }
    else
    {
        return LED_STATUS_ERROR_BUSY;
    }
}

bool LED_Transition_IsBusy(LED_Transition_Handle_t* handle)
{
    return handle->state != LED_TRANSITION_STATE_IDLE;
}

bool LED_Transition_Stop(LED_Transition_Handle_t* handle)
{
    if (handle->state == LED_TRANSITION_STATE_IDLE)
    {
        return true;
    }
    LED_Transition_SetNextState(handle, LED_TRANSITION_STATE_IDLE);
    return true;
}

bool LED_Transition_IsLEDOff(LED_Transition_Handle_t* handle)
{
    uint8_t colorCount = LED_Animation_GetColorCount(handle->LedHandle->controller->LedType);
    uint8_t color[colorCount];
    LED_Animation_GetCurrentColor(handle->LedHandle, color, colorCount);

    return AreColorsOff(color, colorCount);
}

LED_Status_t LED_Transition_ToOff(LED_Transition_Handle_t* handle, LED_Transition_Type_t transitionType,
                                  uint16_t duration)
{
    return LED_Transition_Execute(handle, NULL, LED_ANIMATION_TYPE_OFF, transitionType, duration);
}

LED_Status_t LED_Transition_ToBlink(LED_Transition_Handle_t* handle, const void* animData,
                                    LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_Execute(handle, animData, LED_ANIMATION_TYPE_BLINK, transitionType, duration);
}

LED_Status_t LED_Transition_ToBreath(LED_Transition_Handle_t* handle, const void* animData,
                                     LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_Execute(handle, animData, LED_ANIMATION_TYPE_BREATH, transitionType, duration);
}

LED_Status_t LED_Transition_ToSolid(LED_Transition_Handle_t* handle, const void* animData,
                                    LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_Execute(handle, animData, LED_ANIMATION_TYPE_SOLID, transitionType, duration);
}

LED_Status_t LED_Transition_ToPulse(LED_Transition_Handle_t* handle, const void* animData,
                                    LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_Execute(handle, animData, LED_ANIMATION_TYPE_PULSE, transitionType, duration);
}

LED_Status_t LED_Transition_ToFadeIn(LED_Transition_Handle_t* handle, const void* animData,
                                     LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_Execute(handle, animData, LED_ANIMATION_TYPE_FADE_IN, transitionType, duration);
}

LED_Status_t LED_Transition_ToFadeOut(LED_Transition_Handle_t* handle, const void* animData,
                                      LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_Execute(handle, animData, LED_ANIMATION_TYPE_FADE_OUT, transitionType, duration);
}

LED_Status_t LED_Transition_ToFlash(LED_Transition_Handle_t* handle, const void* animData,
                                    LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_Execute(handle, animData, LED_ANIMATION_TYPE_FLASH, transitionType, duration);
}

LED_Status_t LED_Transition_ToAlternatingColors(LED_Transition_Handle_t* handle, const void* animData,
                                                LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_Execute(handle, animData, LED_ANIMATION_TYPE_ALTERNATING_COLORS, transitionType, duration);
}

LED_Status_t LED_Transition_ToColorCycle(LED_Transition_Handle_t* handle, const void* animData,
                                         LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_Execute(handle, animData, LED_ANIMATION_TYPE_COLOR_CYCLE, transitionType, duration);
}
