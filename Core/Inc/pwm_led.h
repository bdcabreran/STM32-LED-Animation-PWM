#ifndef __PWM_LED_H__
#define __PWM_LED_H__

#include "p_type.h"

#define PWMLED_NO_CMD 0x00
#define PWMLED_OFF_CMD 0x01
#define PWMLED_SOLID_CMD 0x02
#define PWMLED_FLASH_CMD 0x04
#define PWMLED_BREATH_CMD 0x08

#define PWMLED_COLOR_RESOLUTION (256U)
#define PWMLED_COLOR_RESOLUTION_BITS (8U)

#define PWMLED_BREATH_RESOLUTION ((uint32_t)65536U)
#define PWMLED_BREATH_RESOLUTION_BITS (16U)
#define PWMLED_BREATH_TICKS_MAX_BITS (8U)

typedef enum
{
    LED_Active_LOW = 0,
    LED_Active_HIGH,
} LED_Active_t;

typedef enum
{
    PWMLED_OFF = 0, /*!< Persistent state. LED is off and all timers are paused. */

    PWMLED_SOLID, /*!< Persistent state. Solid brightness. */

    PWMLED_FLASH, /*!< Timed state. LED flashes a configurable number of times
                    then moves to next command. Default is OFF if none given. */

    PWMLED_BREATH, /*!< Persistent state. LED is breathing with configurable period
                      and brightnesses. */

} PWMLEDStates_t;

/** @brief Dummy handle used as parent type for all command data handles. */
typedef struct
{
    uint8_t ID; /* Dummy variable for arm compiler v5 support. */
} PWMLED_Cmd_Handle_t;

typedef enum
{
    PWMLEDFlashType_ON = 0, /*!< LED flashes on then off. Finishing in off state. */
    PWMLEDFlashType_OFF,    /*!< LED flashes off then on. Finishing in on state. */
} PWMLEDFlashType_t;

typedef enum
{
    PWMLEDBreathState_IN = 0,  /*!< LED breathing in. */
    PWMLEDBreathState_HOLDIN,  /*!< LED holding breath after inhale. */
    PWMLEDBreathState_OUT,     /*!< LED breathing out. */
    PWMLEDBreathState_HOLDOUT, /*!< LED holding breath after exhale. */
    PWMLEDBreathComplete,      /*!< LED breathing cycle complete. */
} PWMLEDBreathState_t;

typedef struct
{
    PWMLED_Cmd_Handle_t _Super;
    uint8_t             Duty;
} PWMLED_Solid_Handle_t;

typedef struct
{
    PWMLED_Cmd_Handle_t _Super;
    uint8_t             Duty;
    PWMLEDFlashType_t   Type;
    int8_t              Times;
    uint16_t            OnTicks;
    uint16_t            OffTicks;
} PWMLED_Flash_Handle_t;

typedef struct
{
    PWMLED_Cmd_Handle_t _Super;
    uint8_t             Duty;
    uint16_t            PeriodTicks;
    uint16_t            HoldInTicks;
    uint16_t            HoldOutTicks;
} PWMLED_Breath_Handle_t;

typedef struct
{
    PWMLEDStates_t Now;
    PWMLEDStates_t Prev;
    bool           Configured;
    uint32_t       Counter;
} PWMLED_State_Handle_t;

typedef struct
{
    PWMLEDBreathState_t    State;
    uint32_t               Value;
    uint16_t               ValueInc;
    uint16_t               InhaleExhaleTicks;
    uint16_t               HoldInTicks;
    uint16_t               HoldOutTicks;
    uint8_t                Counter;
    uint8_t                InhaleExhaleIncrements;
    PWMLED_Breath_Handle_t LocalConfig;
    uint32_t               tick;
    uint32_t               start_cycle;
    uint32_t               end_cycle;
} PWMLED_Breath_InternalHandle_t;

typedef struct
{
    bool                  LEDisOn;
    uint8_t               FlashCounter;
    PWMLED_Flash_Handle_t LocalConfig;
} RGBLED_Flash_InternalHandle_t;

typedef struct
{
    /******************* Configuration Parameters *******************/
    uint32_t MaxPulse; /*!< Max pulse for PWM period. */

    LED_Active_t ActiveConfig;
    /***************************************************************/

    PWMLED_State_Handle_t State;

    uint8_t CommandNow;
    uint8_t CommandNext;

    PWMLED_Cmd_Handle_t* CommandNow_Handle;
    PWMLED_Cmd_Handle_t* CommandNext_Handle;

    uint16_t PWMDuty;

    RGBLED_Flash_InternalHandle_t  Flash;
    PWMLED_Breath_InternalHandle_t Breath;

} PWMLED_Handle_t;

void PWMLED_Init(PWMLED_Handle_t* this);
void PWMLED_Reset(PWMLED_Handle_t* this);

uint16_t PWMLED_Update(PWMLED_Handle_t* this);

/* LED Command Interface. */
void PWMLED_Command_Off(PWMLED_Handle_t* this, bool ForceNow);
void PWMLED_Command_Solid(PWMLED_Handle_t* this, PWMLED_Solid_Handle_t* CmdHandle, bool ForceNow);
void PWMLED_Command_Flash(PWMLED_Handle_t* this, PWMLED_Flash_Handle_t* CmdHandle, bool ForceNow);
void PWMLED_Command_Breath(PWMLED_Handle_t* this, PWMLED_Breath_Handle_t* CmdHandle, bool ForceNow);

#endif
