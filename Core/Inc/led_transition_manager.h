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
#include <stddef.h> // For NULL

// Default transition times
#define DEFAULT_TRANSITION_CLEAN_ENTRY_TIMEOUT_MS 2000
#define DEFAULT_TRANSITION_UPON_COMPLETION_TIMEOUT_MS 5000
#define DEFAULT_TRANSITION_INTERPOLATE_TIME_MS 200

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
 * @note Duration of the transition, meaning varies with TransitionType
 * - For LED_TRANSITION_IMMINENT: This field is ignored
 * - For LED_TRANSITION_INTERPOLATE: Time in milliseconds to interpolate between animations
 * - For LED_TRANSITION_UPON_COMPLETION: Maximum wait time in milliseconds before force switching
 * - For LED_TRANSITION_AT_CLEAN_ENTRY: Maximum wait time in milliseconds before force switching
 */
typedef struct
{
    const void*           StartAnim;      /**< Pointer to the starting animation configuration */
    const void*           TargetAnim;     /**< Pointer to the target animation configuration */
    LED_Transition_Type_t TransitionType; /**< Type of transition */
    uint16_t              Duration;       /**< Duration of the transition, meaning varies with TransitionType */
} LED_Transition_Config_t;

typedef enum
{
    LED_TRANSITION_STATE_INVALID = 0, /**< State indicating an invalid or uninitialized transition. */
    LED_TRANSITION_STATE_IDLE,        /**< State indicating the transition is idle and not active. */
    LED_TRANSITION_STATE_SETUP,       /**< State indicating the transition is being set up. */
    LED_TRANSITION_STATE_ONGOING,     /**< State indicating the transition is currently in progress. */
    LED_TRANSITION_STATE_COMPLETED,   /**< State indicating the transition has been completed. */
    LED_TRANSITION_STATE_LAST,        /**< Marker for the last valid state; used for validation purposes. */
} LED_Transition_State_t;

#define IS_VALID_TRANSITION_STATE(state) ((state) > LED_TRANSITION_STATE_INVALID && (state) < LED_TRANSITION_STATE_LAST)

/**
 * @brief Structure for managing LED transitions.
 */
typedef struct
{
    const void*            transitionMap;                    /**< Pointer to the transition map. */
    LED_Handle_t*          ledHandle;                        /**< Pointer to the LED handle. */
    const void*            targetAnimData;                   /**< Pointer to the target animation data. */
    uint32_t               lastTick;                         /**< Last tick value. */
    uint16_t               Duration;                         /**< Duration of the transition. */
    uint8_t                mapSize;                          /**< Size of the transition map. */
    LED_Animation_Type_t   targetAnimType;                   /**< Type of the target animation. */
    LED_Transition_Type_t  transitionType;                   /**< Type of the transition. */
    LED_Transition_State_t state;                            /**< State of the transition. */
    uint8_t                currentColor[MAX_COLOR_CHANNELS]; /**< Current color of the LED. */
    uint8_t                targetColor[MAX_COLOR_CHANNELS];  /**< Target color of the LED. */
} LED_Transition_Handle_t;

/**
 * @brief Initializes the LED transition handle.
 * @param this Pointer to the LED transition handle.
 * @param ledHandle Pointer to the LED handle.
 * @return LED status.
 */
LED_Status_t LED_Transition_init(LED_Transition_Handle_t* this, LED_Handle_t* ledHandle);

/**
 * @brief Executes an animation transition using a transition map.
 * @param this Pointer to the LED transition handle.
 * @param animData Pointer to the animation data.
 * @param animType Type of the animation.
 * @return LED status.
 */
LED_Status_t LED_Transition_executeWithMap(LED_Transition_Handle_t* this, const void* animData,
                                           LED_Animation_Type_t animType);

/**
 * @brief Executes a specified animation transition.
 * @param this Pointer to the LED transition handle.
 * @param animData Pointer to the animation data.
 * @param animType Type of the animation.
 * @param transitionType Type of the transition.
 * @param duration Duration of the transition.
 * @return LED status.
 */
LED_Status_t LED_Transition_execute(LED_Transition_Handle_t* this, const void* animData, LED_Animation_Type_t animType,
                                    LED_Transition_Type_t transitionType, uint16_t duration);

/**
 * @brief Sets the transition mapping for the LED transition handle.
 * @param this Pointer to the LED transition handle.
 * @param transitionsMap Pointer to the transition map.
 * @param mapSize Size of the transition map.
 * @return LED status.
 */
LED_Status_t LED_Transition_setMapping(LED_Transition_Handle_t* this, const void* transitionsMap, uint32_t mapSize);

/**
 * @brief Updates the LED transition state based on the current tick.
 * @param this Pointer to the LED transition handle.
 * @param tick Current tick value.
 * @return LED status.
 */
LED_Status_t LED_Transition_update(LED_Transition_Handle_t* this, uint32_t tick);

/**
 * @brief Executes a transition to turn off the LED.
 * @param this Pointer to the LED transition handle.
 * @param transitionType Type of the transition.
 * @param duration Duration of the transition.
 * @return LED status.
 */
LED_Status_t LED_Transition_toOff(LED_Transition_Handle_t* this, LED_Transition_Type_t transitionType,
                                  uint16_t duration);

/**
 * @brief Executes a transition to a blinking animation.
 * @param this Pointer to the LED transition handle.
 * @param animData Pointer to the animation data.
 * @param transitionType Type of the transition.
 * @param duration Duration of the transition.
 * @return LED status.
 */
LED_Status_t LED_Transition_toBlink(LED_Transition_Handle_t* this, const void* animData,
                                    LED_Transition_Type_t transitionType, uint16_t duration);

/**
 * @brief Executes a transition to a breathing animation.
 * @param this Pointer to the LED transition handle.
 * @param animData Pointer to the animation data.
 * @param transitionType Type of the transition.
 * @param duration Duration of the transition.
 * @return LED status.
 */
LED_Status_t LED_Transition_toBreath(LED_Transition_Handle_t* this, const void* animData,
                                     LED_Transition_Type_t transitionType, uint16_t duration);

/**
 * @brief Executes a transition to a solid animation.
 * @param this Pointer to the LED transition handle.
 * @param animData Pointer to the animation data.
 * @param transitionType Type of the transition.
 * @param duration Duration of the transition.
 * @return LED status.
 */
LED_Status_t LED_Transition_toSolid(LED_Transition_Handle_t* this, const void* animData,
                                    LED_Transition_Type_t transitionType, uint16_t duration);

/**
 * @brief Executes a transition to a pulse animation.
 * @param this Pointer to the LED transition handle.
 * @param animData Pointer to the animation data.
 * @param transitionType Type of the transition.
 * @param duration Duration of the transition.
 * @return LED status.
 */
LED_Status_t LED_Transition_toPulse(LED_Transition_Handle_t* this, const void* animData,
                                    LED_Transition_Type_t transitionType, uint16_t duration);

/**
 * @brief Executes a transition to a fade-in animation.
 * @param this Pointer to the LED transition handle.
 * @param animData Pointer to the animation data.
 * @param transitionType Type of the transition.
 * @param duration Duration of the transition.
 * @return LED status.
 */
LED_Status_t LED_Transition_toFadeIn(LED_Transition_Handle_t* this, const void* animData,
                                     LED_Transition_Type_t transitionType, uint16_t duration);

/**
 * @brief Executes a transition to a fade-out animation.
 * @param this Pointer to the LED transition handle.
 * @param animData Pointer to the animation data.
 * @param transitionType Type of the transition.
 * @param duration Duration of the transition.
 * @return LED status.
 */
LED_Status_t LED_Transition_toFadeOut(LED_Transition_Handle_t* this, const void* animData,
                                      LED_Transition_Type_t transitionType, uint16_t duration);

/**
 * @brief Executes a transition to a flash animation.
 * @param this Pointer to the LED transition handle.
 * @param animData Pointer to the animation data.
 * @param transitionType Type of the transition.
 * @param duration Duration of the transition.
 * @return LED status.
 */
LED_Status_t LED_Transition_toFlash(LED_Transition_Handle_t* this, const void* animData,
                                    LED_Transition_Type_t transitionType, uint16_t duration);

/**
 * @brief Executes a transition to alternating colors animation.
 * @param this Pointer to the LED transition handle.
 * @param animData Pointer to the animation data.
 * @param transitionType Type of the transition.
 * @param duration Duration of the transition.
 * @return LED status.
 */
LED_Status_t LED_Transition_toAlternatingColors(LED_Transition_Handle_t* this, const void* animData,
                                                LED_Transition_Type_t transitionType, uint16_t duration);

/**
 * @brief Executes a transition to a color cycle animation.
 * @param this Pointer to the LED transition handle.
 * @param animData Pointer to the animation data.
 * @param transitionType Type of the transition.
 * @param duration Duration of the transition.
 * @return LED status.
 */
LED_Status_t LED_Transition_toColorCycle(LED_Transition_Handle_t* this, const void* animData,
                                         LED_Transition_Type_t transitionType, uint16_t duration);

/**
 * @brief Checks if the LED transition is busy.
 * @param this Pointer to the LED transition handle.
 * @return True if the LED transition is busy, false otherwise.
 */
bool LED_Transition_isBusy(LED_Transition_Handle_t* this);

/**
 * @brief Checks if the LED is off.
 * @param this Pointer to the LED transition handle.
 * @return True if the LED is off, false otherwise.
 */
bool LED_Transition_IsLEDOff(LED_Transition_Handle_t* this);

/**
 * @brief Stops the LED transition.
 * @param this Pointer to the LED transition handle.
 * @return True if the LED transition is stopped, false otherwise.
 */
bool LED_Transition_stop(LED_Transition_Handle_t* this);

#endif /* INC_LED_TRANSITION_MANAGER_H */
