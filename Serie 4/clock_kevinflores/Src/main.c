#include <stdint.h>

// PERIPHERAL & BUS BASE ADDRESS
#define PERIPHERAL_BASE_ADDRESS        0x40000000U
#define AHB_BASE_ADDRESS               (PERIPHERAL_BASE_ADDRESS + 0x00020000U)

// RCC BASE ADDRESS
#define RCC_ADDRESS                    (AHB_BASE_ADDRESS + 0x00001000U)
#define RCC_IOPENR_ADDRESS             (RCC_ADDRESS + 0x0000002CU)

// IOPORT BASE ADDRESS
#define IOPORT_ADDRESS                 (PERIPHERAL_BASE_ADDRESS + 0x10000000U)

// GPIO BASE ADDRESSES
#define GPIOA_BASE_ADDRESS             (IOPORT_ADDRESS + 0x00000000U)
#define GPIOB_BASE_ADDRESS             (IOPORT_ADDRESS + 0x00000400U)
#define GPIOC_BASE_ADDRESS             (IOPORT_ADDRESS + 0x00000800U)


typedef struct
{
    uint32_t MODER;
    uint32_t OTYPER;
    uint32_t OSPEEDR;
    uint32_t PUPDR;
    uint32_t IDR;
    uint32_t ODR;
    uint32_t BSRR;
    uint32_t LCKR;
    uint32_t AFR[2];
    uint32_t BRR;
} GPIOx_Reg_Def;


// ---- RCC register map (STM32L053) ----
typedef struct
{
    uint32_t CR;        // 0x00
    uint32_t ICSCR;     // 0x04
    uint32_t CRRCR;     // 0x08
    uint32_t CFGR;      // 0x0C
    uint32_t CIER;      // 0x10
    uint32_t CIFR;      // 0x14
    uint32_t CICR;      // 0x18
    uint32_t IOPRSTR;   // 0x1C  I/O port reset
    uint32_t AHBRSTR;   // 0x20  AHB reset
    uint32_t APB2RSTR;  // 0x24  APB2 reset
    uint32_t APB1RSTR;  // 0x28  APB1 reset
    uint32_t IOPENR;    // 0x2C  I/O port clock enable
} RCC_Reg_Def;


#define GPIOA   ((GPIOx_Reg_Def*)(GPIOA_BASE_ADDRESS))
#define GPIOB   ((GPIOx_Reg_Def*)(GPIOB_BASE_ADDRESS))
#define GPIOC   ((GPIOx_Reg_Def*)(GPIOC_BASE_ADDRESS))
#define RCC 	((RCC_Reg_Def*)(RCC_ADDRESS))



int main(void)
{

    // Enable clocks: GPIOA y GPIOC
    RCC->IOPENR |= (1 << 0) | (1U << 2);   // IOPAEN | IOPCEN

    // GPIOA: PA5 como salida (MODER11:10 = 01)
    GPIOA->MODER &= ~(1 << 11);
    GPIOC->MODER &= ~(1 << 26);
    GPIOC->MODER &= ~(1 << 27);

    // Loop: LED ON cuando se presiona el botón (PC13 es activo-bajo)
    while(1)
    {
    	if ((GPIOC->IDR & 0X2000))
    	{
    		GPIOA->ODR &= +~(1 << 5);
    	}
    	else
    	{
    		GPIOA->ODR |= (1<<5);
    	}
    }
}
