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

LED_Status_t LED_Transition_init(LED_Transition_Handle_t* this, LED_Handle_t* ledHandle)
{
    if (this == NULL || ledHandle == NULL)
    {
        return LED_STATUS_ERROR_NULL_POINTER;
    }

    this->transitionMap  = NULL;
    this->mapSize        = 0;
    this->ledHandle      = ledHandle;
    this->state          = LED_TRANSITION_STATE_IDLE;
    this->targetAnimData = NULL;
    this->targetAnimType = LED_ANIMATION_TYPE_INVALID;
    this->transitionType = LED_TRANSITION_INVALID;

    LED_TRANSITION_DBG_MSG("LED Transition Manager Initialized\r\n");
    return LED_STATUS_SUCCESS;
}

LED_Status_t LED_Transition_setMapping(LED_Transition_Handle_t* this, const void* transitionsMap, uint32_t mapSize)
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

static void LED_Transition_setNextState(LED_Transition_Handle_t* this, LED_Transition_State_t nextState)
{
    if (IS_VALID_TRANSITION_STATE(nextState))
    {
        this->state = nextState;
    }
}

static LED_Status_t LED_Transition_stateIdle(LED_Transition_Handle_t* this)
{
    /* Do nothing */
    return LED_STATUS_SUCCESS;
}

/**
 * @brief Sets up the duration for the given transition type.
 *
 * @param transitionType The type of LED transition.
 * @param duration The specified duration for the transition. If zero, a default value is used.
 * @return The duration to be used for the transition.
 */
static uint16_t LED_Transition_setupDuration(LED_Transition_Type_t transitionType, uint16_t duration)
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

static bool LED_Transition_findMapTransition(LED_Transition_Handle_t* this)
{
    for (uint32_t i = 0; i < this->mapSize; i++)
    {
        LED_Transition_Config_t* transition = (LED_Transition_Config_t*)this->transitionMap + i;
        if (transition->StartAnim == this->ledHandle->animationData && transition->TargetAnim == this->targetAnimData)
        {
            this->transitionType = transition->TransitionType;
            this->Duration       = LED_Transition_setupDuration(transition->TransitionType, transition->Duration);

            LED_TRANSITION_DBG_MSG("MAP Transition found, %d, duration %d\r\n", this->transitionType, this->Duration);

            return true;
        }
    }
    return false;
}

static bool LED_Transition_findRegularTransition(LED_Transition_Handle_t* this)
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

static void LED_Transition_setDefaultType(LED_Transition_Handle_t* this)
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
 * @param this Pointer to the LED transition handle.
 * @return True if the colors match and transition is skipped, false otherwise.
 */
static bool LED_Transition_configureInterpolation(LED_Transition_Handle_t* this)
{
    // Get the number of color channels based on the LED type
    uint32_t colorCount = LED_Animation_GetColorCount(this->ledHandle->controller->LedType);
    LED_TRANSITION_DBG_MSG("LED Color Count: %d\r\n", colorCount);

    // Retrieve current and target colors
    LED_Animation_GetCurrentColor(this->ledHandle, this->currentColor, colorCount);
    LED_Animation_GetTargetColor((const void*)this->targetAnimData, this->targetAnimType, this->targetColor,
                                 colorCount);

    // Check if the target animation should start at high or low intensity
    bool startHigh = LED_Animation_ShouldStartHigh(this->targetAnimType, (const void*)this->targetAnimData);
    LED_TRANSITION_DBG_MSG("Target Animation Start: %s\r\n", startHigh ? "High" : "Low");

    // If Target Animation starting low, set target colors to off (0x00)
    if (!startHigh)
    {
        for (uint8_t i = 0; i < colorCount; i++)
        {
            this->targetColor[i] = 0x00;
        }
    }

    // Compare current and target colors
    bool colorsMatch = true;
    for (uint8_t i = 0; i < colorCount; i++)
    {
        if (this->currentColor[i] != this->targetColor[i])
        {
            colorsMatch = false; // Colors do not match, transition is needed
        }
        LED_TRANSITION_DBG_MSG("Color Channel [%d]: Current = %d, Target = %d\r\n", i, this->currentColor[i],
                               this->targetColor[i]);
    }

    // If colors match, skip the transition
    if (colorsMatch)
    {
        LED_TRANSITION_DBG_MSG("Colors match, skipping the transition.\r\n");
        return true; // Transition skipped
    }

    // Ensure the LED PWM is active if a transition is needed
    this->ledHandle->controller->Start();
    LED_TRANSITION_DBG_MSG("LED PWM Controller started for interpolation transition.\r\n");

    return false; // Transition needed, colors did not match
}

static void LED_Transition_callCallbackIfExists(LED_Transition_Handle_t* this, LED_Status_t status)
{
    if (this->ledHandle->callback != NULL)
    {
        this->ledHandle->callback(this->targetAnimType, status, (void*)this->targetAnimData);
    }
}

/**
 * @brief Sets up the LED transition state based on the current animation type and mapping.
 *
 * This function finds the appropriate transition type, sets up the necessary configuration,
 * and checks if an interpolation transition is required. If the colors already match, the
 * transition is skipped, and the state is moved to completion.
 *
 * @param this Pointer to the LED transition handle.
 * @param tick   Current tick value.
 * @return LED status indicating the success or failure of the operation.
 */
static LED_Status_t LED_Transition_stateSetup(LED_Transition_Handle_t* this, uint32_t tick)
{
    // Try to find a regular transition first
    bool transitionFound = LED_Transition_findRegularTransition(this);

    // If no regular transition is found, try finding a mapped transition
    if (!transitionFound)
    {
        transitionFound = LED_Transition_findMapTransition(this);
    }

    // If no transition is found in the map, set the default transition type
    if (!transitionFound)
    {
        LED_Transition_setDefaultType(this);
    }

    // Check if the transition type is interpolation and configure it
    if (this->transitionType == LED_TRANSITION_INTERPOLATE)
    {
        bool colorsMatch = LED_Transition_configureInterpolation(this);

        if (colorsMatch)
        {
            // Colors match, no need to interpolate, move directly to completed state
            LED_TRANSITION_DBG_MSG("Colors match, transition completed immediately.\r\n");

            // Call the transition started callback
            LED_Transition_callCallbackIfExists(this, LED_STATUS_ANIMATION_TRANSITION_STARTED);

            // Transition is complete, so move to the completed state
            LED_Transition_setNextState(this, LED_TRANSITION_STATE_COMPLETED);

            return LED_STATUS_SUCCESS;
        }
    }

    // Set the last tick for tracking elapsed time
    this->lastTick = tick;

    // Call the transition started callback
    LED_Transition_callCallbackIfExists(this, LED_STATUS_ANIMATION_TRANSITION_STARTED);

    // Move to the ongoing state to continue the transition
    LED_Transition_setNextState(this, LED_TRANSITION_STATE_ONGOING);

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

static LED_Status_t LED_Transition_stateCompleted(LED_Transition_Handle_t* this)
{
    LED_Transition_callCallbackIfExists(this, LED_STATUS_ANIMATION_TRANSITION_COMPLETED);

    LED_Animation_SetAnimation(this->ledHandle, (void*)this->targetAnimData, this->targetAnimType);
    LED_Animation_Start(this->ledHandle);

    this->targetAnimData = NULL;
    this->targetAnimType = LED_ANIMATION_TYPE_INVALID;
    this->transitionType = LED_TRANSITION_INVALID;
    LED_Transition_setNextState(this, LED_TRANSITION_STATE_IDLE);

    return LED_STATUS_SUCCESS;
}

bool LED_Animation_CheckAndForceColorMatch(LED_Handle_t* ledHandle, uint8_t* targetColor)
{
    // Get the number of colors
    uint8_t colorCount = LED_Animation_GetColorCount(ledHandle->controller->LedType);
    uint8_t color[MAX_COLOR_CHANNELS]; // Use MAX_COLOR_CHANNELS to avoid variable-length arrays

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

static LED_Status_t LED_Transition_stateOngoing(LED_Transition_Handle_t* this, uint32_t tick)
{
    uint32_t elapsed = tick - this->lastTick;

    // Execute the transition
    switch (this->transitionType)
    {
    case LED_TRANSITION_IMMINENT:
    {
        LED_TRANSITION_DBG_MSG("Transitioning Immediately\r\n");
        LED_Transition_setNextState(this, LED_TRANSITION_STATE_COMPLETED);
    }
    break;

    case LED_TRANSITION_INTERPOLATE:
    {
        if (elapsed >= this->Duration)
        {
            LED_TRANSITION_DBG_MSG("Interpolation Completed\r\n");

            // Interpolation sometimes does not reach the target color exactly, the error is usually very small
            if (LED_Animation_CheckAndForceColorMatch(this->ledHandle, this->targetColor))
            {
                LED_TRANSITION_DBG_MSG("Colors successfully transitioned.\r\n");
            }
            else
            {
                LED_TRANSITION_DBG_MSG("Colors forced to target.\r\n");
            }

            // Transition to idle state
            LED_Transition_setNextState(this, LED_TRANSITION_STATE_COMPLETED);
        }
        else
        {
#if USE_LINEAR_INTERPOLATION
            LED_Animation_PerformLinearInterpolation(this->ledHandle, elapsed, this->Duration, this->currentColor,
                                                     this->targetColor);
#endif

#if USE_QUADRATIC_INTERPOLATION
            LED_Animation_PerformQuadraticInterpolation(this->ledHandle, elapsed, this->Duration, this->currentColor,
                                                        this->targetColor);
#endif
        }
    }
    break;

    case LED_TRANSITION_UPON_COMPLETION:
    {
        // Safeguard to prevent infinite loop
        if (elapsed > this->Duration)
        {
            LED_TRANSITION_DBG_MSG("Upon Completion Error, Timeout\r\n");
            LED_Transition_setNextState(this, LED_TRANSITION_STATE_COMPLETED);
        }
        else if (this->ledHandle->isRunning == false)
        {
            LED_TRANSITION_DBG_MSG("Transitioning on Completion\r\n");
            LED_Transition_setNextState(this, LED_TRANSITION_STATE_COMPLETED);
        }
    }
    break;

    case LED_TRANSITION_AT_CLEAN_ENTRY:
    {
        uint8_t colorCount = LED_Animation_GetColorCount(this->ledHandle->controller->LedType);
        uint8_t color[MAX_COLOR_CHANNELS]; // Use MAX_COLOR_CHANNELS to avoid VLAs
        LED_Animation_GetCurrentColor(this->ledHandle, color, colorCount);

        if (AreColorsOff(color, colorCount))
        {
            LED_TRANSITION_DBG_MSG("Transitioning on Off\r\n");
            LED_Transition_setNextState(this, LED_TRANSITION_STATE_COMPLETED);
        }
        // Safeguard to prevent infinite loop
        else if (elapsed > this->Duration)
        {
            LED_TRANSITION_DBG_MSG("Clean Entry Error, Timeout\r\n");
            LED_Transition_setNextState(this, LED_TRANSITION_STATE_COMPLETED);
        }
    }
    break;

    default:
        LED_TRANSITION_DBG_MSG("Invalid Transition Type\r\n");
        break;
    }

    return LED_STATUS_SUCCESS;
}

LED_Status_t LED_Transition_update(LED_Transition_Handle_t* this, uint32_t tick)
{
    if (this == NULL)
    {
        return LED_STATUS_ERROR_NULL_POINTER;
    }
    switch (this->state)
    {
    case LED_TRANSITION_STATE_IDLE:
        LED_Transition_stateIdle(this);
        break;
    case LED_TRANSITION_STATE_SETUP:
        LED_Transition_stateSetup(this, tick);
        break;
    case LED_TRANSITION_STATE_ONGOING:
        LED_Transition_stateOngoing(this, tick);
        break;
    case LED_TRANSITION_STATE_COMPLETED:
        LED_Transition_stateCompleted(this);
        break;
    default:
        break;
    }

    if (this->transitionType != LED_TRANSITION_INTERPOLATE)
    {
        LED_Animation_Update(this->ledHandle, tick);
    }

    return LED_STATUS_SUCCESS;
}

LED_Status_t LED_Transition_executeWithMap(LED_Transition_Handle_t* this, const void* animData,
                                           LED_Animation_Type_t animType)
{
    return LED_Transition_execute(this, animData, animType, LED_TRANSITION_INVALID, 0);
}

LED_Status_t LED_Transition_execute(LED_Transition_Handle_t* this, const void* animData, LED_Animation_Type_t animType,
                                    LED_Transition_Type_t transitionType, uint16_t duration)
{
    if (this == NULL)
    {
        return LED_STATUS_ERROR_NULL_POINTER;
    }

    // Check if the LED is already in the target state
    if (animType == LED_ANIMATION_TYPE_OFF && LED_Transition_IsLEDOff(this))
    {
        // Notify that the transition was skipped
        if (this->ledHandle->callback != NULL)
        {
            this->ledHandle->callback(animType, LED_STATUS_ANIMATION_TRANSITION_SKIPPED, (void*)animData);
        }
        return LED_STATUS_SUCCESS;
    }

    if (this->state == LED_TRANSITION_STATE_IDLE)
    {
        // If there is an ongoing LED animation, stop it
        if (this->ledHandle->isRunning)
        {
            LED_Animation_Stop(this->ledHandle, true);
        }

        this->targetAnimData = animData;
        this->targetAnimType = animType;
        this->transitionType = transitionType;
        this->Duration       = duration;
        LED_Transition_setNextState(this, LED_TRANSITION_STATE_SETUP);
        return LED_STATUS_SUCCESS;
    }
    else
    {
        return LED_STATUS_ERROR_BUSY;
    }
}

bool LED_Transition_isBusy(LED_Transition_Handle_t* this)
{
    return this->state != LED_TRANSITION_STATE_IDLE;
}

bool LED_Transition_stop(LED_Transition_Handle_t* this)
{
    if (this->state == LED_TRANSITION_STATE_IDLE)
    {
        return true;
    }
    LED_Transition_setNextState(this, LED_TRANSITION_STATE_IDLE);
    return true;
}

bool LED_Transition_IsLEDOff(LED_Transition_Handle_t* this)
{
    uint8_t colorCount = LED_Animation_GetColorCount(this->ledHandle->controller->LedType);
    uint8_t color[MAX_COLOR_CHANNELS];
    LED_Animation_GetCurrentColor(this->ledHandle, color, colorCount);

    return AreColorsOff(color, colorCount);
}

LED_Status_t LED_Transition_toOff(LED_Transition_Handle_t* this, LED_Transition_Type_t transitionType,
                                  uint16_t duration)
{
    return LED_Transition_execute(this, NULL, LED_ANIMATION_TYPE_OFF, transitionType, duration);
}

LED_Status_t LED_Transition_toBlink(LED_Transition_Handle_t* this, const void* animData,
                                    LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_execute(this, animData, LED_ANIMATION_TYPE_BLINK, transitionType, duration);
}

LED_Status_t LED_Transition_toBreath(LED_Transition_Handle_t* this, const void* animData,
                                     LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_execute(this, animData, LED_ANIMATION_TYPE_BREATH, transitionType, duration);
}

LED_Status_t LED_Transition_toSolid(LED_Transition_Handle_t* this, const void* animData,
                                    LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_execute(this, animData, LED_ANIMATION_TYPE_SOLID, transitionType, duration);
}

LED_Status_t LED_Transition_toPulse(LED_Transition_Handle_t* this, const void* animData,
                                    LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_execute(this, animData, LED_ANIMATION_TYPE_PULSE, transitionType, duration);
}

LED_Status_t LED_Transition_toFadeIn(LED_Transition_Handle_t* this, const void* animData,
                                     LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_execute(this, animData, LED_ANIMATION_TYPE_FADE_IN, transitionType, duration);
}

LED_Status_t LED_Transition_toFadeOut(LED_Transition_Handle_t* this, const void* animData,
                                      LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_execute(this, animData, LED_ANIMATION_TYPE_FADE_OUT, transitionType, duration);
}

LED_Status_t LED_Transition_toFlash(LED_Transition_Handle_t* this, const void* animData,
                                    LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_execute(this, animData, LED_ANIMATION_TYPE_FLASH, transitionType, duration);
}

LED_Status_t LED_Transition_toAlternatingColors(LED_Transition_Handle_t* this, const void* animData,
                                                LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_execute(this, animData, LED_ANIMATION_TYPE_ALTERNATING_COLORS, transitionType, duration);
}

LED_Status_t LED_Transition_toColorCycle(LED_Transition_Handle_t* this, const void* animData,
                                         LED_Transition_Type_t transitionType, uint16_t duration)
{
    return LED_Transition_execute(this, animData, LED_ANIMATION_TYPE_COLOR_CYCLE, transitionType, duration);
}
