#ifndef __COMMON_TYPE_H__
#define __COMMON_TYPE_H__

#include <stdint.h>
#include <stdbool.h>

#define UNIT_U16_BITS (16U)
#define UNIT_U16_RESOLUTION ((uint32_t)65536U)
#define UNIT_U16_MAX (0xFFFF)

/* One-sided bits and resolution for values using s16 format. */
#define UNIT_S16_BITS (15U)
#define UNIT_S16_RESOLUTION ((int32_t)32768)
#define UNIT_S16_MAX ((int32_t)32767)

#define INT16_SATURATION(a)        \
  if ((a) > INT16_MAX)             \
  {                                \
    (a) = INT16_MAX;               \
  }                                \
  else if ((a) < (-INT16_MAX - 1)) \
  {                                \
    (a) = (-INT16_MAX - 1);        \
  }                                \
  else                             \
  {                                \
  }

/** @brief  Calculate log2  */
#define LOG2(x) \
  ((x) == 0 ? -1 : \
   (x) & (1UL << 31) ? 31 : \
   (x) & (1UL << 30) ? 30 : \
   (x) & (1UL << 29) ? 29 : \
   (x) & (1UL << 28) ? 28 : \
   (x) & (1UL << 27) ? 27 : \
   (x) & (1UL << 26) ? 26 : \
   (x) & (1UL << 25) ? 25 : \
   (x) & (1UL << 24) ? 24 : \
   (x) & (1UL << 23) ? 23 : \
   (x) & (1UL << 22) ? 22 : \
   (x) & (1UL << 21) ? 21 : \
   (x) & (1UL << 20) ? 20 : \
   (x) & (1UL << 19) ? 19 : \
   (x) & (1UL << 18) ? 18 : \
   (x) & (1UL << 17) ? 17 : \
   (x) & (1UL << 16) ? 16 : \
   (x) & (1UL << 15) ? 15 : \
   (x) & (1UL << 14) ? 14 : \
   (x) & (1UL << 13) ? 13 : \
   (x) & (1UL << 12) ? 12 : \
   (x) & (1UL << 11) ? 11 : \
   (x) & (1UL << 10) ? 10 : \
   (x) & (1UL << 9) ? 9 : \
   (x) & (1UL << 8) ? 8 : \
   (x) & (1UL << 7) ? 7 : \
   (x) & (1UL << 6) ? 6 : \
   (x) & (1UL << 5) ? 5 : \
   (x) & (1UL << 4) ? 4 : \
   (x) & (1UL << 3) ? 3 : \
   (x) & (1UL << 2) ? 2 : \
   (x) & (1UL << 1) ? 1 : \
   0)

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif

#ifndef SQ
#define SQ(a) (a * a)
#endif

#ifndef SIGN
#define SIGN(a) \
  ((a) > 0 ? 1 :  \
  (a) < 0 ? -1 :  \
  0)
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef struct
{
  volatile uint32_t *SetReg;
  volatile uint16_t *ResetReg;
  uint16_t Pin;
} GPIO_Output_Handle_t;

typedef struct
{
  volatile uint32_t *ReadReg;
  uint16_t Pin;
} GPIO_Input_Handle_t;

/* Boolean data types */
typedef unsigned short bool_t;

/* Fractional data types */
typedef signed char             Q7_t;
typedef signed short            Q15_t;
typedef signed long             Q31_t;
typedef signed long long        Q63_t;

#endif
