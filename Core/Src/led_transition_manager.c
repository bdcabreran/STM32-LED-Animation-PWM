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
        return LED_ERROR_NULL_POINTER;
    }

    this->transitionMap  = NULL;
    this->mapSize        = 0;
    this->LedHandle      = LedHandle;
    this->state          = LED_TRANSITION_STATE_IDLE;
    this->targetAnimData = NULL;
    this->targetAnimType = LED_ANIMATION_INVALID;

    LED_TRANSITION_DBG_MSG("LED Transition Manager Initialized\r\n");
    return LED_SUCCESS;
}

LED_Status_t LED_Transition_SetMapping(LED_Transition_Handle_t* this, const void* transitionsMap, uint32_t mapSize)
{
    if (this == NULL || transitionsMap == NULL || mapSize == 0)
    {
        return LED_ERROR_NULL_POINTER;
    }

    this->transitionMap = transitionsMap;
    this->mapSize       = mapSize;

    LED_TRANSITION_DBG_MSG("LED Transition Mapping Set\r\n");
    return LED_SUCCESS;
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
    return LED_SUCCESS;
}

static LED_Status_t LED_Transition_StateSetup(LED_Transition_Handle_t* this, uint32_t tick)
{
    // Default transition type
    bool                  transitionFound = false;
    LED_Transition_Type_t transitionType  = LED_TRANSITION_IMMINENT;

    // Search current and target animation in the transition map
    for (uint32_t i = 0; i < this->mapSize; i++)
    {
        LED_Transition_Config_t* transition = (LED_Transition_Config_t*)this->transitionMap + i;
        if (transition->StartAnim == this->LedHandle->animationData && transition->EndAnim == this->targetAnimData)
        {
            LED_TRANSITION_DBG_MSG("Transition found in the map, %d\r\n", transition->TransitionType);
            this->transitionType = transition->TransitionType;
            transitionFound      = true;
        }
    }

    if (!transitionFound)
    {
        LED_TRANSITION_DBG_MSG("Transition not found in the map, using default transition type\r\n");
        this->transitionType = transitionType;
    }

    this->lastTick = tick;
    LED_Transition_SetNextState(this, LED_TRANSITION_STATE_ONGOING);

    return LED_SUCCESS;
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
        LED_Animation_SetAnimation(this->LedHandle, this->targetAnimData, this->targetAnimType);
        LED_Animation_Start(this->LedHandle);
        LED_Transition_SetNextState(this, LED_TRANSITION_STATE_IDLE);
        this->lastTick = tick; // Update lastTick after transition
    }
    break;
    case LED_TRANSITION_INTERPOLATE:
    {
        LED_TRANSITION_DBG_MSG("Transitioning with Interpolation\r\n");
        // Code for LED_TRANSITION_INTERPOLATE transition
        this->lastTick = tick; // Update lastTick after transition
    }
    break;
    case LED_TRANSITION_UPON_COMPLETION:
    {
        LED_TRANSITION_DBG_MSG("Transitioning Upon Completion\r\n");
        this->lastTick = tick; // Update lastTick after transition
    }
    break;
    case LED_TRANSITION_AT_CLEAN_ENTRY:
    {
        if (this->LedHandle->isRunning == false)
        {
            LED_Animation_SetAnimation(this->LedHandle, this->targetAnimData, this->targetAnimType);
            LED_Animation_Start(this->LedHandle);
            LED_Transition_SetNextState(this, LED_TRANSITION_STATE_IDLE);
            this->lastTick = tick; // Update lastTick after transition
        }
        else
        {
            uint8_t colorCount = CalculateColorCount(this->LedHandle->animationType);
            uint8_t color[colorCount];
            LED_Animation_GetColor(this->LedHandle, color, &colorCount);

            bool isOff = true;
            for (uint8_t i = 0; i < colorCount; i++)
            {
                if (color[i] != 0)
                {
                    isOff = false;
                    LED_TRANSITION_DBG_MSG("not off\r\n");
                    break;
                }
            }

            if (isOff)
            {
                LED_TRANSITION_DBG_MSG("Transitioning on Off\r\n");
                LED_Animation_SetAnimation(this->LedHandle, this->targetAnimData, this->targetAnimType);
                LED_Animation_Start(this->LedHandle);
                LED_Transition_SetNextState(this, LED_TRANSITION_STATE_IDLE);
                this->lastTick = tick; // Update lastTick after transition
            }
        }

        if (elapsed > 1000)
        {
            LED_TRANSITION_DBG_MSG("Clean Entry Error, Timeout\r\n");
            this->transitionType = LED_TRANSITION_IMMINENT;
            this->lastTick       = tick; // Update lastTick after handling timeout
        }
    }
    break;
    }
    return LED_SUCCESS;
}
LED_Status_t LED_Transition_Update(LED_Transition_Handle_t* this, uint32_t tick)
{

    if (this == NULL)
    {
        return LED_ERROR_NULL_POINTER;
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

    LED_Animation_Update(this->LedHandle, tick);

    return LED_SUCCESS;
}

LED_Status_t
LED_Transition_ExecAnimation(LED_Transition_Handle_t* this, const void* animData, LED_Animation_Type_t animType)
{
    if (this == NULL || animData == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    if (this->state == LED_TRANSITION_STATE_IDLE)
    {
        this->targetAnimData = animData;
        this->targetAnimType = animType;
        this->event          = LED_TRANSITION_EVT_START;
        return LED_SUCCESS;
    }

    return LED_SUCCESS;
}