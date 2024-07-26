/**
 * @file led_transition_manager.h
 * @brief Header file for LED transition management library.
 *
 * This library handles transitions between different LED animations.
 *
 * @copyright Copyright (c) 2024
 */

#ifndef INC_LED_TRANSITION_MANAGER_H
#define INC_LED_TRANSITION_MANAGER_H

#include "led_animation.h"

#define CLEAN_ENTRY_TIMEOUT_MS 2000

/**
 * @brief Enum for different types of LED transitions.
 */
typedef enum
{
    LED_TRANSITION_INVALID = 0,     /**< Invalid transition type */
    LED_TRANSITION_IMMINENT,        /**< Immediately switch to the target animation */
    LED_TRANSITION_INTERPOLATE,     /**< Smoothly interpolate between animations */
    LED_TRANSITION_UPON_COMPLETION, /**< Wait for the current animation to complete before transitioning */
    LED_TRANSITION_AT_CLEAN_ENTRY,  /**< Wait for a clean entry point in the current animation before transitioning */
    LED_TRANSITION_LAST             /**< Placeholder for the last transition type */
} LED_Transition_Type_t;

#define IS_VALID_TRANSITION_TYPE(type) ((type) > LED_TRANSITION_INVALID && (type) < LED_TRANSITION_LAST)

/**
 * @brief Structure for configuring an LED transition.
 */
typedef struct
{
    const void*           StartAnim;      /**< Pointer to the starting animation configuration */
    const void*           EndAnim;        /**< Pointer to the target animation configuration */
    LED_Transition_Type_t TransitionType; /**< Type of transition */
} LED_Transition_Config_t;

typedef enum
{
    LED_TRANSITION_STATE_INVALID = 0,
    LED_TRANSITION_STATE_IDLE,
    LED_TRANSITION_STATE_SETUP,
    LED_TRANSITION_STATE_ONGOING,
    LED_TRANSITION_STATE_COMPLETED,
    LED_TRANSITION_STATE_LAST,
} LED_Transition_State_t;

#define IS_VALID_TRANSITION_STATE(state) ((state) > LED_TRANSITION_STATE_INVALID && (state) < LED_TRANSITION_STATE_LAST)

typedef enum
{
    LED_TRANSITION_EVT_INVALID = 0,
    LED_TRANSITION_EVT_START,
    LED_TRANSITION_EVT_STOP,
    LED_TRANSITION_EVT_LAST,
} LED_Transition_Event_t;

#define IS_VALID_TRANSITION_EVENT(event) ((event) > LED_TRANSITION_EVT_INVALID && (event) < LED_TRANSITION_EVT_LAST)

typedef struct
{
    const void*            transitionMap;
    uint32_t               mapSize;
    LED_Handle_t*          LedHandle;
    void*                  targetAnimData;
    LED_Animation_Type_t   targetAnimType;
    LED_Transition_Type_t  transitionType;
    LED_Transition_State_t state;
    LED_Transition_Event_t event;
    uint32_t               lastTick;

} LED_Transition_Handle_t;

LED_Status_t LED_Transition_Init(LED_Transition_Handle_t* this, LED_Handle_t* LedHandle);
LED_Status_t LED_Transition_SetMapping(LED_Transition_Handle_t* this, const void* transitionsMap, uint32_t mapSize);
LED_Status_t LED_Transition_Update(LED_Transition_Handle_t* this, uint32_t tick);
LED_Status_t
LED_Transition_ExecAnimation(LED_Transition_Handle_t* this, const void* animConfig, LED_Animation_Type_t animType);

#endif /* INC_LED_TRANSITION_MANAGER_H */
