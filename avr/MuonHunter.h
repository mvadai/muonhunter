/* 
 *
 * MuonHunter.h
 * Created: 18/08/2015
 * Last modified: 17/07/2016
 * 
 * Author: Mihaly Vadai
 * Website:	http://muonhunter.com
 * 
 * For credits see README.md
 *
 * License: GPL v.3
 * Version: 0.3-eero
 * 
 */

volatile uint8_t SERIAL = 17;
// test mode w/o crystal 
// #define TIMER_OVF_M 122
#define TIMER_OVF_M 125
// test mode w/o crystal 
// #define TIMER_OVF_S 3906
#define TIMER_OVF_S 4000
#define OSS 0

#define MEMORY_LIMIT 60
#define DEFAULT_STATE 0x1B
#define DEFAULT_MODE 0x0
#define BMP180_ADDRESS 0x77
#define TWI_WRITE 0x0
#define TWI_READ 0x1

// input
#define AB_CD_PIN	PC0 // > MUON_PIN - OK
#define AC_BD_PIN	PC1 // > GM1_PIN - OK
#define AD_PIN		PC2 // > GM2_PIN - OK
#define MUTE_BUTTON	PD6
#define BC_PIN		PD5 // > BACKLIGHT
#define A_B_C_D_PIN	PC3 // > RST_BUTTON - OK
#define MODE_BUTTON PD7

// output
#define MUON_LED	PD0
#define GM1_LED		PD2
#define GM2_LED		PD1
#define MUON_BUZZER PD3
#define LIGHTSWITCH PD4

volatile uint8_t state = DEFAULT_STATE;
volatile uint8_t modes = DEFAULT_MODE;
volatile char str[7];
volatile uint8_t buffer_address;
volatile uint8_t txbuffer[0x1F];
volatile uint8_t rxbuffer[0xA];

// core timers and sums
volatile uint16_t timer_time = 0;
volatile uint8_t timer_sec = 0;
volatile uint8_t timer_min = 0;
volatile uint8_t timer_hour = 0;
volatile uint8_t timer_day = 0;

//timer compensation variables
volatile uint8_t gm_LED_comp = 0;
volatile uint8_t muon_LED_comp = 0;
volatile uint8_t gm_buzz_comp = 0;
volatile uint8_t muon_buzz_comp = 0;
volatile uint8_t uart_comp = 0;

// core counting variables
volatile uint16_t AC_BD_hits = 0;
volatile uint16_t AD_hits = 0;
volatile uint16_t BC_hits = 0;
volatile uint16_t AB_CD_hits = 0;
volatile uint16_t A_B_C_D_hits = 0;
char buffer[3];

void Initialize(void);
void update_state(void);
void update_counter(void);
void gm_buzz(void);
void change_clock(uint8_t mode);
void timer_update(void);
void flash_gm1(void);
void flash_gm2(void);
void flash_muon(void);
void timer_compensate(void);
void display_time(void);
void plateau_counter_update(void);
void timer_update(void);
