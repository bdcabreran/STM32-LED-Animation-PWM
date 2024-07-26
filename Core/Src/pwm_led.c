#include "pwm_led.h"

#define DEBUG_BUFFER_SIZE 100
#define LED_PWM_DBG 1

#if LED_PWM_DBG
#include "stm32l4xx_hal.h"
#include <stdio.h>
extern UART_HandleTypeDef huart2;
#define LED_PWM_DBG_MSG(fmt, ...)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        static char dbgBuff[DEBUG_BUFFER_SIZE];                                                                        \
        snprintf(dbgBuff, DEBUG_BUFFER_SIZE, (fmt), ##__VA_ARGS__);                                                    \
        HAL_UART_Transmit(&huart2, (uint8_t*)dbgBuff, strlen(dbgBuff), 1000);                                          \
    } while (0)
#else
#define LED_PWM_DBG_MSG(fmt, ...)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#endif

static void PWMLED_BreathValueUpdate(PWMLED_Handle_t* this, uint16_t Duty);
static void PWMLED_SetPWMDutyBreath(PWMLED_Handle_t* this, uint8_t Duty);
static void PWMLED_SetPWMDuty(PWMLED_Handle_t* this, uint8_t Duty);

static void
PWMLED_Command_Generic(PWMLED_Handle_t* this, uint8_t Command, PWMLED_Cmd_Handle_t* CommandHandle, bool ForceNow);
static void PWMLED_CheckBufferedCommand(PWMLED_Handle_t* this);

static PWMLED_Cmd_Handle_t NullCmdHandle;

__WEAK void PWMLED_Init(PWMLED_Handle_t* this)
{
    this->CommandNow         = PWMLED_NO_CMD;
    this->CommandNow_Handle  = &NullCmdHandle;
    this->CommandNext_Handle = &NullCmdHandle;
    this->State              = (PWMLED_State_Handle_t){PWMLED_OFF, PWMLED_OFF, 0};
}

__WEAK uint16_t PWMLED_Update(PWMLED_Handle_t* this)
{
    if (this->State.Counter > 0)
    {
        this->State.Counter--;
    }

    switch (this->State.Now)
    {

    case PWMLED_OFF:
    {
        /* Can move to any state from here. */

        if (this->State.Configured == false)
        {
            this->PWMDuty = 0;

            this->CommandNow &= ~PWMLED_OFF_CMD;

            this->State.Configured = true;
        }

        switch (this->CommandNow)
        {
        case PWMLED_NO_CMD:
        {
            /* Nothing to do. */
            break;
        }

        case PWMLED_OFF_CMD:
        {
            /* Nothing to do. */
            break;
        }

        case PWMLED_SOLID_CMD:
        {
            this->CommandNow &= ~PWMLED_SOLID_CMD;
            this->State = (PWMLED_State_Handle_t){PWMLED_SOLID, this->State.Now, false, 0};
            break;
        }

        case PWMLED_FLASH_CMD:
        {
            this->CommandNow &= ~PWMLED_FLASH_CMD;
            this->State = (PWMLED_State_Handle_t){PWMLED_FLASH, this->State.Now, false, 0};
            break;
        }

        case PWMLED_BREATH_CMD:
        {
            this->CommandNow &= ~PWMLED_BREATH_CMD;
            this->State = (PWMLED_State_Handle_t){PWMLED_BREATH, this->State.Now, false, 0};
            break;
        }

        default:
        {
            break;
        }
        }

        break;
    }

    case PWMLED_SOLID:
    {
        /* Can move to PWMLED_OFF state from here. */
        static PWMLED_Solid_Handle_t* Config;

        if (this->CommandNow != PWMLED_NO_CMD)
        {
            /* New command received, move back to OFF state so it can be executed. */
            this->State = (PWMLED_State_Handle_t){PWMLED_OFF, this->State.Now, false, 0};
            break; /* Break immediately. */
        }

        if (this->State.Configured != true)
        {
            Config = (PWMLED_Solid_Handle_t*)this->CommandNow_Handle;

            PWMLED_SetPWMDuty(this, Config->Duty);

            this->State.Configured = true;
        }

        PWMLED_CheckBufferedCommand(this);

        break;
    }

    case PWMLED_FLASH:
    {
        if (this->CommandNow != PWMLED_NO_CMD)
        {
            /* Change of state requested immediately. Go back to OFF so command can be executed. */
            this->State = (PWMLED_State_Handle_t){PWMLED_OFF, this->State.Now, false, 0};
            break; /* Break immediately. */
        }

        /* Do initial state configuration. */
        if (this->State.Configured != true)
        {
            this->Flash.LocalConfig = *((PWMLED_Flash_Handle_t*)this->CommandNow_Handle);

            /* Set local flash counter to track how many times LED has flashed. */
            this->Flash.FlashCounter = this->Flash.LocalConfig.Times;

            /* Set initial LED state based on configured flash type. */
            if (this->Flash.LocalConfig.Type == PWMLEDFlashType_ON)
            {
                /* Set counter for how long LED should be on. */
                this->State.Counter = this->Flash.LocalConfig.OnTicks;

                /* Set initial LED color. */
                PWMLED_SetPWMDuty(this, this->Flash.LocalConfig.Duty);

                this->Flash.LEDisOn = true;
            }
            else if (this->Flash.LocalConfig.Type == PWMLEDFlashType_OFF)
            {
                /* Set counter for how long LED should be off. */
                this->State.Counter = this->Flash.LocalConfig.OffTicks;

                PWMLED_SetPWMDuty(this, 0);

                this->Flash.LEDisOn = false;
            }

            this->State.Configured = true;
        }

        /* If negative number was passed, assume this means to flash permanently. */
        if (this->Flash.LocalConfig.Times < 0)
        {
            this->Flash.FlashCounter = UINT8_MAX;
        }

        if (this->State.Counter == 0)
        {
            /* Check if flash sequence has finished. */
            if (this->Flash.FlashCounter == 0)
            {
                /* Flash sequence finished. Look if there is another command buffered. */
                PWMLED_CheckBufferedCommand(this);

                /* Move to PWMLED_OFF state. */
                this->State = (PWMLED_State_Handle_t){PWMLED_OFF, this->State.Now, false, 0};
                break; /* Break immediately. */
            }

            /* Sequence still in progress, check which state to move to. */
            if (this->Flash.LEDisOn == true)
            {
                /* Turn LED off. */
                PWMLED_SetPWMDuty(this, 0);

                /* Capture local LED state. */
                this->Flash.LEDisOn = false;

                /* Set counter to count how long we should remain with LED off. */
                this->State.Counter = this->Flash.LocalConfig.OffTicks;

                if (this->Flash.LocalConfig.Type == PWMLEDFlashType_ON)
                {
                    /* Decrement flash counter as LED has completed one flash cycle. */
                    this->Flash.FlashCounter--;
                }
            }
            else
            {
                /* Turn LED on. */
                PWMLED_SetPWMDuty(this, this->Flash.LocalConfig.Duty);

                /* Capture local LED state. */
                this->Flash.LEDisOn = true;

                /* Set counter to count how long we should remain with LED on. */
                this->State.Counter = this->Flash.LocalConfig.OnTicks;

                if (this->Flash.LocalConfig.Type == PWMLEDFlashType_OFF)
                {
                    /* Decrement flash counter as LED has completed one flash cycle. */
                    this->Flash.FlashCounter--;
                }
            }
        }

        break;
    }

    case PWMLED_BREATH:
    {

        if (this->CommandNow != PWMLED_NO_CMD)
        {
            /* Change of state requested immediately. Go back to OFF so command can be executed. */
            this->State = (PWMLED_State_Handle_t){PWMLED_OFF, this->State.Now, false, 0};
            break; /* Break immediately. */
        }

        if (this->State.Configured != true)
        {
            this->Breath.start_cycle = DWT->CYCCNT;

            int16_t  Error;
            uint16_t TmpInhaleExhaleTicks;

            // PWMLED_Breath_Handle_t *LocalConfig = (PWMLED_Breath_Handle_t *)this->CommandNow_Handle;
            // PWMLED_Breath_Handle_t Config = *LocalConfig;

            this->Breath.LocalConfig = *((PWMLED_Breath_Handle_t*)this->CommandNow_Handle);

            /* Calculate temporary ticks based on exact timings requested by config. */
            TmpInhaleExhaleTicks = (this->Breath.LocalConfig.PeriodTicks - this->Breath.LocalConfig.HoldInTicks -
                                    this->Breath.LocalConfig.HoldOutTicks) /
                                   2U;

            /* Calculate time between each breath value update. With fair rounding applied. */
            this->Breath.InhaleExhaleIncrements =
                (TmpInhaleExhaleTicks + (1 << (PWMLED_BREATH_TICKS_MAX_BITS - 1))) >> PWMLED_BREATH_TICKS_MAX_BITS;

            /* Make inhale exhale ticks an exact multiple of increments. */
            this->Breath.InhaleExhaleTicks = this->Breath.InhaleExhaleIncrements * 256U;

            /* Calculate error between exact ticks and ticks after truncation. */
            Error = TmpInhaleExhaleTicks - this->Breath.InhaleExhaleTicks;

            /* Adjust on and off ticks so total configured period is still correct. */
            this->Breath.HoldInTicks  = this->Breath.LocalConfig.HoldInTicks + Error;
            this->Breath.HoldOutTicks = this->Breath.LocalConfig.HoldOutTicks + Error;

            /* Reset breath variables. */
            this->Breath.Counter  = 0;
            this->Breath.ValueInc = 1;
            this->Breath.Value    = 0;

            /* Set state. */
            this->State.Counter    = this->Breath.InhaleExhaleTicks;
            this->Breath.State     = PWMLEDBreathState_IN;
            this->State.Configured = true;

            this->Breath.tick = HAL_GetTick();

            this->Breath.end_cycle = DWT->CYCCNT;
            LED_PWM_DBG_MSG("Breath Configured: %lu\r\n", this->Breath.end_cycle - this->Breath.start_cycle);

            /* Break so first and subsequent breath sequences are identical. */
            break;
        }

        switch (this->Breath.State)
        {
        case PWMLEDBreathState_IN:
        {
            this->Breath.start_cycle = DWT->CYCCNT;

            if (this->State.Counter == 0)
            {
                /* Set LED to max value. */
                PWMLED_SetPWMDuty(this, this->Breath.LocalConfig.Duty);

                /* Set number of ticks LED should remain on for. */
                this->State.Counter = this->Breath.HoldInTicks;

                /* Reset Breath counter. */
                this->Breath.Counter = 0;

                /* Reduce increment value by 2 since 2 was added and not used.
                   This ensures exhale is symmetric with inhale. */
                this->Breath.ValueInc -= 2;

                /* Change state. */
                this->Breath.State = PWMLEDBreathState_HOLDIN;

                this->Breath.end_cycle = DWT->CYCCNT;
                // LED_PWM_DBG_MSG("Breath in 1: %lu\r\n", this->Breath.end_cycle - this->Breath.start_cycle);

                break; /* Break immediately. */
            }

            PWMLED_BreathValueUpdate(this, this->Breath.LocalConfig.Duty);
            this->Breath.end_cycle = DWT->CYCCNT;
            // LED_PWM_DBG_MSG("Breath in 2: %lu\r\n", this->Breath.end_cycle - this->Breath.start_cycle);
            break;
        }

        case PWMLEDBreathState_HOLDIN:
        {
            if (this->State.Counter == 0)
            {
                /* Set number of ticks LED should exhale for. */
                this->State.Counter = this->Breath.InhaleExhaleTicks;

                /* Change state. */
                this->Breath.State = PWMLEDBreathState_OUT;
            }
            // LED_PWM_DBG_MSG("%lu; %d\r\n", (HAL_GetTick() - this->Breath.tick), this->PWMDuty);

            break;
        }

        case PWMLEDBreathState_OUT:
        {
            this->Breath.start_cycle = DWT->CYCCNT;

            if (this->State.Counter == 0)
            {
                /* Set LED to off. */
                PWMLED_SetPWMDuty(this, 0);

                /* Set number of ticks LED should be off for. */
                this->State.Counter = this->Breath.HoldOutTicks;

                /* Reset Breath State. */
                this->Breath.Counter  = 0;
                this->Breath.ValueInc = 1;
                this->Breath.Value    = 0;

                /* Change state. */
                this->Breath.State     = PWMLEDBreathState_HOLDOUT;
                this->Breath.end_cycle = DWT->CYCCNT;
                // LED_PWM_DBG_MSG("Breath out 1: %lu\r\n", this->Breath.end_cycle - this->Breath.start_cycle);

                break; /* Break immediately. */
            }

            PWMLED_BreathValueUpdate(this, this->Breath.LocalConfig.Duty);
            this->Breath.end_cycle = DWT->CYCCNT;
            // LED_PWM_DBG_MSG("Breath out 2: %lu\r\n", this->Breath.end_cycle - this->Breath.start_cycle);

            break;
        }

        case PWMLEDBreathState_HOLDOUT:
        {
            if (this->State.Counter == 0)
            {
                /* Set number of ticks LED should inhale for. */
                this->State.Counter = this->Breath.InhaleExhaleTicks;

                /* Change state. */
                this->Breath.State = PWMLEDBreathComplete;

                /* One breath cycle finished so check if there is a buffered command. */
                PWMLED_CheckBufferedCommand(this);
            }
            // LED_PWM_DBG_MSG("%lu; %d\r\n", (HAL_GetTick() - this->Breath.tick), this->PWMDuty);

            break;
        }

        case PWMLEDBreathComplete:
        {
            LED_PWM_DBG_MSG("Breath complete.\n");
            this->State.Now = PWMLED_OFF;
            break;
        }
        }
    }

    default:
    {
        break;
    }
    }

    return this->PWMDuty;
}

static void PWMLED_BreathValueUpdate(PWMLED_Handle_t* this, uint16_t Duty)
{
    if (++this->Breath.Counter >= this->Breath.InhaleExhaleIncrements)
    {
        this->Breath.start_cycle = DWT->CYCCNT;

        // LED_PWM_DBG_MSG("State: %d, Brightness: %d\r\n", this->Breath.State, this->Breath.Value);

        switch (this->Breath.State)
        {
        case PWMLEDBreathState_IN:
        {
            this->Breath.Value += this->Breath.ValueInc;

            if (this->Breath.Value > PWMLED_BREATH_RESOLUTION)
            {
                this->Breath.Value = PWMLED_BREATH_RESOLUTION;
            }

            this->Breath.ValueInc += 2U;

            break;
        }

        case PWMLEDBreathState_OUT:
        {
            if (this->Breath.Value >= this->Breath.ValueInc)
            {
                this->Breath.Value -= this->Breath.ValueInc;

                /* Prevent underflow. */
                if (this->Breath.ValueInc > 1U)
                {
                    this->Breath.ValueInc -= 2U;
                }
            }

            break;
        }

        default:
        {
            break;
        }
        }

        PWMLED_SetPWMDutyBreath(this, Duty);

        this->Breath.end_cycle = DWT->CYCCNT;
        // LED_PWM_DBG_MSG("%lu; %d\r\n", (HAL_GetTick() - this->Breath.tick), this->PWMDuty);
        uint32_t total = this->Breath.end_cycle - this->Breath.start_cycle;

        this->Breath.Counter = 0;
        LED_PWM_DBG_MSG("%lu C : %lu\r\n", (HAL_GetTick() - this->Breath.tick), total);
    }
}

static void PWMLED_SetPWMDutyBreath(PWMLED_Handle_t* this, uint8_t Duty)
{
    this->PWMDuty =
        ((((uint32_t)this->MaxPulse * this->Breath.Value) >> PWMLED_BREATH_RESOLUTION_BITS) * (uint32_t)Duty) >>
        PWMLED_COLOR_RESOLUTION_BITS;
    // LED_PWM_DBG_MSG("Updated PWM Duty: %u\r\n", this->PWMDuty);
}

static void PWMLED_CheckBufferedCommand(PWMLED_Handle_t* this)
{
    if (this->CommandNext != PWMLED_NO_CMD)
    {
        /* Set next to command to current command. */
        this->CommandNow        = this->CommandNext;
        this->CommandNow_Handle = this->CommandNext_Handle;

        /* Remove pending next command. */
        this->CommandNext        = PWMLED_NO_CMD;
        this->CommandNext_Handle = &NullCmdHandle;
    }
}

static void PWMLED_SetPWMDuty(PWMLED_Handle_t* this, uint8_t Duty)
{
    this->PWMDuty = ((uint32_t)this->MaxPulse * Duty) >> PWMLED_COLOR_RESOLUTION_BITS;
}

__WEAK void PWMLED_Command_Off(PWMLED_Handle_t* this, bool ForceNow)
{
    PWMLED_Command_Generic(this, PWMLED_OFF_CMD, &NullCmdHandle, ForceNow);
}

__WEAK void PWMLED_Command_Solid(PWMLED_Handle_t* this, PWMLED_Solid_Handle_t* CmdHandle, bool ForceNow)
{
    PWMLED_Command_Generic(this, PWMLED_SOLID_CMD, &CmdHandle->_Super, ForceNow);
}

__WEAK void PWMLED_Command_Flash(PWMLED_Handle_t* this, PWMLED_Flash_Handle_t* CmdHandle, bool ForceNow)
{
    PWMLED_Command_Generic(this, PWMLED_FLASH_CMD, &CmdHandle->_Super, ForceNow);
}

__WEAK void PWMLED_Command_Breath(PWMLED_Handle_t* this, PWMLED_Breath_Handle_t* CmdHandle, bool ForceNow)
{
    PWMLED_Command_Generic(this, PWMLED_BREATH_CMD, &CmdHandle->_Super, ForceNow);
}

static void
PWMLED_Command_Generic(PWMLED_Handle_t* this, uint8_t Command, PWMLED_Cmd_Handle_t* CommandHandle, bool ForceNow)
{
    if (ForceNow == true)
    {
        this->CommandNow        = Command;
        this->CommandNow_Handle = CommandHandle;
    }
    else
    {
        this->CommandNext        = Command;
        this->CommandNext_Handle = CommandHandle;
    }
}
