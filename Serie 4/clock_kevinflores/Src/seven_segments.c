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


#define GPIOA   ((GPIOx_Reg_Def*)(GPIOA_BASE_ADDRESS))
#define GPIOB   ((GPIOx_Reg_Def*)(GPIOB_BASE_ADDRESS))
#define GPIOC   ((GPIOx_Reg_Def*)(GPIOC_BASE_ADDRESS))
#define RCC 	((RCC_Reg_Def*)(RCC_ADDRESS))

#define ALL_DISPLAY_CTRL (D0_CTRL | D1_CTRL | D2_CTRL | D3_CTRL)


// Masks para PORTC
#define D0_CTRL 0b00100000    // POS 5 of PORTC
#define D1_CTRL 0b01000000    // POS 6 of PORTC
#define D2_CTRL 0b100000000   // POS 8 of PORTC
#define D3_CTRL 0b1000000000  // POS 9 of PORTC

#define CC_DP  (1U << 7)  // para common-cathode, dp se enciende con 1

uint8_t my_clock =0x00;


// ---- 7 segmentos (orden [a,b,c,d,e,f,g,dp]) ---- Forzamos la inversion a 8 bits con un macro
#define CA8(x)   ((uint8_t)(x))
#define INV8(x)  ((uint8_t)~(uint8_t)(x))

// Common Anode  (bit = 0 enciende segmento)
#define ca_all_off CA8(0b11111111)
#define ca_0       CA8(0b11000000)
#define ca_1       CA8(0b11111001)
#define ca_2       CA8(0b10100100)
#define ca_3       CA8(0b10110000)
#define ca_4       CA8(0b10011001)
#define ca_5       CA8(0b10010010)
#define ca_6       CA8(0b10000010)
#define ca_7       CA8(0b11111000)
#define ca_8       CA8(0b10000000)
#define ca_9       CA8(0b10010000)

// Common Cathode (bit = 1 enciende segmento)
#define cc_all_off INV8(ca_all_off)   // 0b00000000
#define cc_0       INV8(ca_0)         // 0b00111111
#define cc_1       INV8(ca_1)         // 0b00000110
#define cc_2       INV8(ca_2)         // 0b01011011
#define cc_3       INV8(ca_3)         // 0b01001111
#define cc_4       INV8(ca_4)         // 0b01100110
#define cc_5       INV8(ca_5)         // 0b01101101
#define cc_6       INV8(ca_6)         // 0b01111101
#define cc_7       INV8(ca_7)         // 0b00000111
#define cc_8       INV8(ca_8)         // 0b01111111
#define cc_9       INV8(ca_9)         // 0b01101111

#define ca_cc_bits 0b11111111 //Space used for output GPIOB [7:0]


#define LOOP_DELAY_MS   5U                     // el delay de tu bucle
#define SEC_TICKS       (1000U / LOOP_DELAY_MS) // 1000/5 = 200 ticks ≈ 1s


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



//Tiempo empieza 00:00

typedef struct {
    uint8_t minutes_u;
    uint8_t minutes_d;
    uint8_t hour_u;
    uint8_t hour_d;
} timevariables;


static uint16_t time_keeper = 0;


void delay_ms (uint16_t n);
uint8_t parser (uint8_t decode);



// --- helper: compara reloj vs alarma ---
static inline uint8_t alarm_match(const timevariables *c, const timevariables *a)
{
    return (c->minutes_u == a->minutes_u) &&
           (c->minutes_d == a->minutes_d) &&
           (c->hour_u    == a->hour_u)    &&
           (c->hour_d    == a->hour_d);
}

// Fija una alarma a hh:mm en formato 24 h (convierte a BCD hour_d/hour_u, minutes_d/minutes_u)
static inline void alarm_set(timevariables *a, uint8_t hh24, uint8_t mm)
{
    a->minutes_u = (uint8_t)(mm % 10U);
    a->minutes_d = (uint8_t)(mm / 10U);
    a->hour_u    = (uint8_t)(hh24 % 10U);
    a->hour_d    = (uint8_t)(hh24 / 10U);
}


// ---------- LED AM en LD2 = PA5 ----------
#define AM_LED_PORT   GPIOA
#define AM_LED_PIN    (1U << 5)          // PA5 (LED verde de la Nucleo)

static inline void am_led_on(void)  { AM_LED_PORT->BSRR = AM_LED_PIN; }
static inline void am_led_off(void) { AM_LED_PORT->BSRR = (AM_LED_PIN << 16); }

static inline void am_led_init(void)
{
    GPIOA->MODER  &= ~(3U << (5U * 2U));
    GPIOA->MODER  |=  (1U << (5U * 2U));   // 01 = salida
    GPIOA->OTYPER &= ~(1U << 5);           // push-pull
    GPIOA->PUPDR  &= ~(3U << (5U * 2U));   // sin pull
}

// Enciende si h<12 (AM), apaga si h>=12 (PM)
static inline void am_led_update(uint8_t hour_d, uint8_t hour_u)
{
    uint8_t h24 = (uint8_t)(hour_d * 10U + hour_u); // 00..23
    if (h24 < 12U) am_led_on(); else am_led_off();
}

static volatile uint8_t alarm_ringing = 0;
static uint8_t alarm_seconds = 0;   // cuánto tiempo lleva sonando (en segundos)




// ---------- Botón de FORMATO (B1 azul en PC13) ----------
#define FMT_BTN_PORT   GPIOC
#define FMT_BTN_PIN    (1U << 13)   // PC13

// Flag de formato: 0 = 24h, 1 = 12h
static volatile uint8_t is_12h = 0;

// Antirrebote (tick ~10 ms)
#define DEBOUNCE_TICKS 5            // 5*10ms = 50ms
// En la Nucleo hay pull-down externo en PC13 → reposo=0, presionado=1
static uint8_t  fmt_btn_last  = 0;
static uint8_t  fmt_btn_state = 0;
static uint8_t  fmt_btn_cnt   = 0;

// Debounce del botón de formato (llamar cada ~10 ms)
static inline void FormatButton_Update_10ms(void)
{
    uint8_t sample = (FMT_BTN_PORT->IDR & FMT_BTN_PIN) ? 1U : 0U; // 1=press

    if (sample != fmt_btn_last) { fmt_btn_last = sample; fmt_btn_cnt = 0; }
    else if (fmt_btn_cnt < DEBOUNCE_TICKS) { fmt_btn_cnt++; }

    if (fmt_btn_cnt == DEBOUNCE_TICKS && sample != fmt_btn_state) {
        fmt_btn_state = sample;
        if (fmt_btn_state == 1U) {      // flanco de subida
            is_12h ^= 1U;               // alterna 24h <-> 12h
        }
    }
}


// Convierte horas internas (24h, BCD en hour_d:hour_u) a dígitos de display según is_12h
static inline void GetDisplayHours(uint8_t hour_d, uint8_t hour_u,
                                   uint8_t *disp_d, uint8_t *disp_u)
{
    uint8_t h24 = (uint8_t)(hour_d * 10U + hour_u);

    if (is_12h == 0U) {                 // 24h
        *disp_d = hour_d;
        *disp_u = hour_u;
    } else {                            // 12h (1..12)
        uint8_t h12 = (uint8_t)(h24 % 12U);
        if (h12 == 0U) h12 = 12U;
        *disp_d = (uint8_t)(h12 / 10U); // 0 o 1
        *disp_u = (uint8_t)(h12 % 10U); // 0..9
    }
}


static inline void disp_select(uint32_t mask)
{
    // Apaga todos y enciende solo el indicado
    GPIOC->BSRR = (ALL_DISPLAY_CTRL << 16);
    GPIOC->BSRR = mask;
}


// ---------- LED de ALARMA en PB8 ----------
#define ALARM_LED_PORT   GPIOB
#define ALARM_LED_PIN    (1U << 8)        // PB8

static inline void alarm_led_on(void)  { ALARM_LED_PORT->BSRR = ALARM_LED_PIN; }
static inline void alarm_led_off(void) { ALARM_LED_PORT->BSRR = (ALARM_LED_PIN << 16); }

// Configura PB8 como salida push-pull, sin pull-ups/downs
static inline void alarm_led_init(void)
{
    GPIOB->MODER  &= ~(3U << (8U * 2U));
    GPIOB->MODER  |=  (1U << (8U * 2U));   // 01 = salida
    GPIOB->OTYPER &= ~(1U << 8);           // push-pull
    GPIOB->PUPDR  &= ~(3U << (8U * 2U));   // sin pull
}

static inline void alarm_action(void)
{
    alarm_led_on();      // enciende LED PB8
    alarm_ringing = 1;
    alarm_seconds = 0;
}





int main(void)
{


	// Instancias locales inicializadas en 0
	timevariables clk   = (timevariables){0};
	timevariables alarm1 = (timevariables){0};

	//Alarma
	alarm_set(&alarm1, 3, 5);   // 03:00 AM

	RCC->IOPENR |= (1U << 0) | (1U << 1) | (1U << 2);
	am_led_init();


	// Habilitar clocks A, B, C (ya lo tienes)
	RCC->IOPENR |= (1U << 0) | (1U << 1) | (1U << 2);

	// ...tus otras inits (PA5/PA6, displays, etc.)

	// Inicializa PB8 como salida para LED de alarma
	alarm_led_init();
	// Asegura apagado al arrancar
	alarm_led_off();


	// Habilitar clocks A, B, C
	RCC->IOPENR |= (1U << 0) | (1U << 1) | (1U << 2);

	// ================= GPIOA =================

	// PA6 (D12) y PA7 (D11) como salida push-pull, sin pulls
	GPIOA->MODER  &= ~((3U << (6U * 2U)) | (3U << (7U * 2U)));
	GPIOA->MODER  |=  ((1U << (6U * 2U)) | (1U << (7U * 2U)));
	GPIOA->OTYPER &= ~((1U << 6) | (1U << 7));
	GPIOA->PUPDR  &= ~((3U << (6U * 2U)) | (3U << (7U * 2U)));


	// ===== PA5 (LD2 onboard) como salida push-pull, sin pull =====
	GPIOA->MODER  &= ~(3U << (5U * 2U));   // limpia MODER11:10
	GPIOA->MODER  |=  (1U << (5U * 2U));   // 01 = salida
	GPIOA->OTYPER &= ~(1U << 5);           // push-pull
	GPIOA->PUPDR  &= ~(3U << (5U * 2U));   // sin pull-up/pull-down



	// PA0 (botón de formato) como entrada + pull-up
	GPIOA->MODER &= ~(3U << (0U * 2U));   // input
	GPIOA->PUPDR &= ~(3U << (0U * 2U));
	GPIOA->PUPDR |=  (1U << (0U * 2U));   // 01 = pull-up

	// ================= GPIOB =================
	// PB0..PB3 como salida (añade PB4..PB7 si los usas)
	GPIOB->MODER &= ~(3U << (0U * 2U)); GPIOB->MODER |= (1U << (0U * 2U));
	GPIOB->MODER &= ~(3U << (1U * 2U)); GPIOB->MODER |= (1U << (1U * 2U));
	GPIOB->MODER &= ~(3U << (2U * 2U)); GPIOB->MODER |= (1U << (2U * 2U));
	GPIOB->MODER &= ~(3U << (3U * 2U)); GPIOB->MODER |= (1U << (3U * 2U));
	GPIOB->MODER &= ~(3U << (4U * 2U)); GPIOB->MODER |= (1U << (4U * 2U));
	GPIOB->MODER &= ~(3U << (5U * 2U)); GPIOB->MODER |= (1U << (5U * 2U));
	GPIOB->MODER &= ~(3U << (6U * 2U)); GPIOB->MODER |= (1U << (6U * 2U));
	GPIOB->MODER &= ~(3U << (7U * 2U)); GPIOB->MODER |= (1U << (7U * 2U));

	// ================= GPIOC =================
	// PC5, PC6, PC8, PC9 como salida (control de displays)
	GPIOC->MODER &= ~(3U << (5U * 2U)); GPIOC->MODER |= (1U << (5U * 2U));
	GPIOC->MODER &= ~(3U << (6U * 2U)); GPIOC->MODER |= (1U << (6U * 2U));
	GPIOC->MODER &= ~(3U << (8U * 2U)); GPIOC->MODER |= (1U << (8U * 2U));
	GPIOC->MODER &= ~(3U << (9U * 2U)); GPIOC->MODER |= (1U << (9U * 2U));


	// PC13 (B1) como entrada (pull-down externo en la placa)
	GPIOC->MODER &= ~(3U << (13U * 2U));  // input
	GPIOC->PUPDR &= ~(3U << (13U * 2U));  // sin pull interno

	// Apaga todos los displays al iniciar
	GPIOC->BSRR = (ALL_DISPLAY_CTRL << 16);

	// Habilitar clocks A, B, C (ya lo tienes)
	RCC->IOPENR |= (1U << 0) | (1U << 1) | (1U << 2);

	// ...tus otras inits (PA5, PA6, displays, etc.)

	// PB8 como salida para LED de alarma
	alarm_led_init();

    while (1)
    {

        // 1) Actualiza el botón de formato (cada ~10 ms)
        FormatButton_Update_10ms();

        // 2) Calcula dígitos de hora según el formato actual (24h/12h)
        uint8_t disp_h_d, disp_h_u;
        GetDisplayHours(clk.hour_d, clk.hour_u, &disp_h_d, &disp_h_u);

        // Mantener LED AM/PM acorde a la hora interna (24h)
        am_led_update(clk.hour_d, clk.hour_u);


        switch (my_clock)
        {
            case 0: // minutos unidades
            {
                GPIOC->BSRR = (ALL_DISPLAY_CTRL << 16) | D0_CTRL;

                uint32_t pat = (uint32_t)parser(clk.minutes_u);
                GPIOB->BSRR = ((uint32_t)ca_cc_bits << 16) | pat;
                my_clock++;
                break;
            }
            case 1: // minutos decenas
            {
                GPIOC->BSRR = (ALL_DISPLAY_CTRL << 16) | D1_CTRL;

                uint32_t pat = (uint32_t)parser(clk.minutes_d);
                GPIOB->BSRR = ((uint32_t)ca_cc_bits << 16) | pat;

                my_clock++;
                break;
            }
            case 2: // horas unidades (ya convertidas con GetDisplayHours)
            {
                GPIOC->BSRR = (ALL_DISPLAY_CTRL << 16) | D2_CTRL;

                uint32_t pat = (uint32_t)parser(disp_h_u) | (uint32_t)CC_DP; // dp ON
                GPIOB->BSRR = ((uint32_t)ca_cc_bits << 16) | pat;

                my_clock++;
                break;
            }
            case 3: // horas decenas (blank si 12h y decena=0) + dp ON
            {
                GPIOC->BSRR = (ALL_DISPLAY_CTRL << 16) | D3_CTRL;
                uint32_t pat = (is_12h && (disp_h_d == 0)) ? (uint32_t)cc_all_off
                                                           : (uint32_t)parser(disp_h_d);
                GPIOB->BSRR = ((uint32_t)ca_cc_bits << 16) | pat;
                my_clock = 0x00;
                break;
            }
            default:
            {
                my_clock = 0x00;
                break;
            }
        }




        delay_ms(LOOP_DELAY_MS);
        uint8_t tick_1s = 0;


        if (++time_keeper >= SEC_TICKS) {   // ≈ 1 s
            time_keeper = 0;
            tick_1s = 1;

            // ----- minutos -----
            if (++clk.minutes_u > 9) {
                clk.minutes_u = 0;
                if (++clk.minutes_d > 5) {
                    clk.minutes_d = 0;

                    // ----- horas -----
                    if (clk.hour_d == 2) {              // 20..23
                        if (clk.hour_u >= 3) {          // 23 -> 00
                            clk.hour_u = 0;
                            clk.hour_d = 0;
                        } else {
                            clk.hour_u++;               // 20..22 -> +1
                        }
                    } else {                             // 00..19
                        if (clk.hour_u >= 9) {          // 09 -> 10, 19 -> 20
                            clk.hour_u = 0;
                            clk.hour_d++;
                        } else {
                            clk.hour_u++;               // 00..08, 10..18 -> +1
                        }
                    }
                }
            }
        }

        // ------- COMPARAR RELOJ vs ALARMA -------
        if (!alarm_ringing && alarm_match(&clk, &alarm1)) {
            alarm_action();   // enciende PB8 y arranca temporizador
        }

        // ------- AUTO-OFF a los 3 s -------
        if (alarm_ringing && tick_1s) {
            if (++alarm_seconds >= 30) {
                alarm_led_off();                   // APAGA PB8
                alarm_ringing = 0;
                am_led_update(clk.hour_d, clk.hour_u); // restaura LED AM/PM si lo usas
            }
        }



        }

}


void delay_ms (uint16_t n)
{
	uint16_t i;
	for(;n>0;n--)
		for(i=0; i<140; i++);

}

uint8_t parser (uint8_t decode)
{
    switch (decode)
    {
        case 0: return cc_0;
        case 1: return cc_1;
        case 2: return cc_2;
        case 3: return cc_3;
        case 4: return cc_4;
        case 5: return cc_5;
        case 6: return cc_6;
        case 7: return cc_7;
        case 8: return cc_8;
        case 9: return cc_9;
        default: return cc_all_off;   // cualquier valor fuera de 0–9 apaga segmentos
    }
}


