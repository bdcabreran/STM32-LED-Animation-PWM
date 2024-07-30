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
        this->event = LED_TRANSITION_EVT_INVALID;
    }
}

static LED_Status_t LED_Transition_StateIdle(LED_Transition_Handle_t* this)
{
    if (this->event == LED_TRANSITION_EVT_START)
    {
        LED_Transition_SetNextState(this, LED_TRANSITION_STATE_SETUP);
        LED_TRANSITION_DBG_MSG("Transitioning from IDLE to SETUP\r\n");
    }
    return LED_STATUS_SUCCESS;
}

uint8_t  currentColor[3];
uint8_t  targetColor[3];
uint32_t colorCount;

static bool LED_Transition_FindTransition(LED_Transition_Handle_t* this)
{
    for (uint32_t i = 0; i < this->mapSize; i++)
    {
        LED_Transition_Config_t* transition = (LED_Transition_Config_t*)this->transitionMap + i;
        if (transition->StartAnim == this->LedHandle->animationData && transition->EndAnim == this->targetAnimData)
        {
            LED_TRANSITION_DBG_MSG("Transition found in the map, %d\r\n", transition->TransitionType);
            this->transitionType = transition->TransitionType;
            return true;
        }
    }
    return false;
}

static void LED_Transition_SetDefaultType(LED_Transition_Handle_t* this)
{
    LED_TRANSITION_DBG_MSG("Transition not found in the map, using default transition type\r\n");
    this->transitionType = LED_TRANSITION_IMMINENT;
}

static void LED_Transition_HandleInterpolate(LED_Transition_Handle_t* this)
{
    uint32_t colorCount = CalculateColorCount(this->targetAnimType);
    LED_TRANSITION_DBG_MSG("LED Color Count: %d\r\n", colorCount);

    LED_Animation_GetCurrentColor(this->LedHandle, currentColor, colorCount);
    LED_Animation_GetTargetColor(this->targetAnimData, this->targetAnimType, targetColor, colorCount);

    for (uint8_t i = 0; i < colorCount; i++)
    {
        LED_TRANSITION_DBG_MSG("[%d] Current Color: %d, Target Color: %d\r\n", i + 1, currentColor[i], targetColor[i]);
    }

    bool startHigh = LED_Animation_ShouldStartHigh(this->targetAnimType, this->targetAnimData);

    if (!startHigh)
    {
        LED_TRANSITION_DBG_MSG("Target Animation starts low, setting target color to 0\r\n");
        targetColor[0] = targetColor[1] = targetColor[2] = 0;
    }
    else
    {
        LED_TRANSITION_DBG_MSG("Target Animation starts high\r\n");
    }

    if (currentColor[0] == targetColor[0] && currentColor[1] == targetColor[1] && currentColor[2] == targetColor[2])
    {
        LED_TRANSITION_DBG_MSG("No interpolation needed\r\n");
        this->transitionType = LED_TRANSITION_IMMINENT;
    }
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
            // LED_TRANSITION_DBG_MSG("not off\r\n");
            return false;
        }
    }
    return true;
}

/**
 * @brief Linearly interpolate between two colors.
 *
 * @param startColor The starting color.
 * @param endColor The ending color.
 * @param t The interpolation factor (0.0 to 1.0).
 * @param interpolatedColor The result of the interpolation.
 * @param colorCount The number of color components.
 */
void InterpolateColors(uint8_t* startColor, uint8_t* endColor, float t, uint8_t* interpolatedColor, uint8_t colorCount)
{
    for (uint8_t i = 0; i < colorCount; i++)
    {
        interpolatedColor[i] = (uint8_t)((1.0f - t) * startColor[i] + t * endColor[i]);
    }
}

static LED_Status_t LED_Transition_StateOngoing(LED_Transition_Handle_t* this, uint32_t tick)
{
    uint32_t elapsed = tick - this->lastTick;

    // If no animation is running start immediately
    if (this->LedHandle->isRunning == false)
    {
        LED_TRANSITION_DBG_MSG("No animation running, starting immediately\r\n");

        if (this->LedHandle->callback != NULL)
        {
            this->LedHandle->callback(this->LedHandle->animationType, LED_STATUS_ANIMATION_TRANSITION_COMPLETED);
        }

        LED_Animation_SetAnimation(this->LedHandle, this->targetAnimData, this->targetAnimType);
        LED_Animation_Start(this->LedHandle);
        LED_Transition_SetNextState(this, LED_TRANSITION_STATE_IDLE);
        return LED_STATUS_SUCCESS;
    }

    // Execute the transition
    switch (this->transitionType)
    {
    case LED_TRANSITION_IMMINENT:
    {
        LED_TRANSITION_DBG_MSG("Transitioning Immediately\r\n");

        if (this->LedHandle->callback != NULL)
        {
            this->LedHandle->callback(this->LedHandle->animationType, LED_STATUS_ANIMATION_TRANSITION_COMPLETED);
        }

        LED_Animation_SetAnimation(this->LedHandle, this->targetAnimData, this->targetAnimType);
        LED_Animation_Start(this->LedHandle);

        LED_Transition_SetNextState(this, LED_TRANSITION_STATE_IDLE);
    }
    break;

    case LED_TRANSITION_SOFT:
    {
        // quick fade in/out transition
        LED_TRANSITION_DBG_MSG("Transitioning Softly\r\n");
    }
    break;
    case LED_TRANSITION_INTERPOLATE:
    {
        if (elapsed >= TRANSITION_INTERPOLATE_TIME_MS)
        {
            LED_TRANSITION_DBG_MSG("Interpolation Completed\r\n");

            // Set current color to target color to complete the transition
            LED_Animation_GetCurrentColor(this->LedHandle, currentColor, &colorCount);
            LED_TRANSITION_DBG_MSG("Current Color: %d %d %d\r\n", currentColor[0], currentColor[1], currentColor[2]);

            if (this->LedHandle->callback != NULL)
            {
                this->LedHandle->callback(this->LedHandle->animationType, LED_STATUS_ANIMATION_TRANSITION_COMPLETED);
            }

            LED_Animation_SetAnimation(this->LedHandle, this->targetAnimData, this->targetAnimType);
            LED_Animation_Start(this->LedHandle);

            // Transition to idle state
            LED_Transition_SetNextState(this, LED_TRANSITION_STATE_IDLE);
        }
        else
        {
            // Calculate the interpolated color
            uint32_t timeInCycle = elapsed % TRANSITION_INTERPOLATE_TIME_MS;

            // Calculate interpolation factor (t) based on elapsed time and scale it
            uint32_t fraction_scaled = (timeInCycle * 1000) / TRANSITION_INTERPOLATE_TIME_MS;

            // Ensure fraction_scaled is between 0 and 1000
            if (fraction_scaled > 1000)
            {
                fraction_scaled = 1000;
            }

            // Calculate the interpolated color using the scaled fraction
            uint8_t r =
                currentColor[0] + ((int32_t)(fraction_scaled * (int32_t)(targetColor[0] - currentColor[0])) / 1000);
            uint8_t g =
                currentColor[1] + ((int32_t)(fraction_scaled * (int32_t)(targetColor[1] - currentColor[1])) / 1000);
            uint8_t b =
                currentColor[2] + ((int32_t)(fraction_scaled * (int32_t)(targetColor[2] - currentColor[2])) / 1000);
            uint8_t interpolatedColor[3] = {r, g, b};

            // Print the scaled fraction and timeInCycle for debugging
            LED_TRANSITION_DBG_MSG("%d Inter Color: %d %d %d | Fraction: %d | Time in Cycle: %d\r\n",
                                   elapsed,
                                   interpolatedColor[0],
                                   interpolatedColor[1],
                                   interpolatedColor[2],
                                   fraction_scaled,
                                   timeInCycle);

            // Update the LED handle with the interpolated color
            LED_Animation_ExecuteColorSetting(this->LedHandle, interpolatedColor);
        }

        break;
    }

    case LED_TRANSITION_UPON_COMPLETION:
    {
        LED_TRANSITION_DBG_MSG("Transitioning Upon Completion\r\n");

        // safe guard to prevent infinite loop
        if (elapsed > TRANSITION_UPON_COMPLETION_TIMEOUT_MS)
        {
            LED_TRANSITION_DBG_MSG("Upon Completion Error, Timeout\r\n");
            this->transitionType = LED_TRANSITION_IMMINENT;
        }
        else if (this->LedHandle->isRunning == false)
        {
            LED_TRANSITION_DBG_MSG("Transitioning on Completion\r\n");

            if (this->LedHandle->callback != NULL)
            {
                this->LedHandle->callback(this->LedHandle->animationType, LED_STATUS_ANIMATION_TRANSITION_COMPLETED);
            }

            LED_Animation_SetAnimation(this->LedHandle, this->targetAnimData, this->targetAnimType);
            LED_Animation_Start(this->LedHandle);

            LED_Transition_SetNextState(this, LED_TRANSITION_STATE_IDLE);
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
            if (this->LedHandle->callback != NULL)
            {
                this->LedHandle->callback(this->LedHandle->animationType, LED_STATUS_ANIMATION_TRANSITION_COMPLETED);
            }

            LED_Animation_SetAnimation(this->LedHandle, this->targetAnimData, this->targetAnimType);
            LED_Animation_Start(this->LedHandle);

            LED_Transition_SetNextState(this, LED_TRANSITION_STATE_IDLE);
        }

        if (elapsed > TRANSITION_AT_CLEAN_ENTRY_TIMEOUT_MS)
        {
            LED_TRANSITION_DBG_MSG("Clean Entry Error, Timeout\r\n");
            this->transitionType = LED_TRANSITION_IMMINENT;
        }
    }
    break;
    }
    return LED_STATUS_SUCCESS;
}
static uint32_t last_tick = 0;
LED_Status_t    LED_Transition_Update(LED_Transition_Handle_t* this, uint32_t tick)
{

    if (last_tick == tick)
    {
        return LED_STATUS_SUCCESS;
    }

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
        // Code for LED_TRANSITION_COMPLETED state
        break;
    default:
        break;
    }

    last_tick = tick;

    LED_Animation_Update(this->LedHandle, tick);

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
        this->event          = LED_TRANSITION_EVT_START;
        return LED_STATUS_SUCCESS;
    }

    return LED_STATUS_SUCCESS;
}
