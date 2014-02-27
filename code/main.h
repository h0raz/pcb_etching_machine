#define FALSE 0
#define TRUE 1

#define LM75_ADRESS 0x9E

#define ENC_PORT PORTC
#define BUTTON_PORT PORTC
#define BUTTON_PIN PINC
#define ENC_PIN PINC
#define BUTTON 0x04
#define ENC1 0x01
#define ENC2 0x02

#define SERVO1 D,0
#define SERVO2 D,1
#define HEAT D,4
#define ALARM D,2

#define SERVO_1_STEPS_UP 40
#define SERVO_1_STEPS_DOWN -56
#define SERVO_1_MIDDLE 127
#define SERVO_2_STEPS_UP 54
#define SERVO_2_STEPS_DOWN -39
#define SERVO_2_MIDDLE 105

#define HEAT_DELAY 10

typedef struct {
    uint8_t times;
    uint8_t timem;
    uint8_t speed;
    uint8_t temp_max_diff;
    uint8_t servo_speed_factor;
    uint8_t light_times;
    uint8_t light_timem;
    uint8_t etch_times;
    uint8_t etch_timem;
}set_t;

set_t gset;
set_t *set = &gset;  //  Reading temp variable
uint8_t read_temp;
uint8_t set_temp;  //  pre calcualted values for servo motors
uint8_t ocr1;
uint8_t ocr2;  //  button states
volatile uint8_t dstate;
enum {ACTION_LEFT, ACTION_RIGHT, ACTION_FORWARD, ACTION_BACK};  //  button action values
enum {BUT_NONE, BUT_PRESSED, BUT_RELEASED, BUT_SHORT, BUT_LONG, BUT_HOLD};  //  button states  //  button and rotary pulse encoder variables
uint8_t zust_state;
uint8_t zust_new_state;
uint8_t zust_count;
uint8_t button_state;
uint8_t button_count;  //  servo timer variable
uint8_t t1_servo_state;  //  Heat and pwm light counter
uint8_t t0_count;
uint8_t t2_count;  //  Menu
enum { MENU_LIGHT, MENU_ETCH};  //  main menu
enum { SUB_NONE, SUB_START, SUB_SETUP, SUB_SETUP_MENU,
    SUB_SET_TIME, SUB_SET_SPEED, SUB_SET_TEMP, SUB_SET_SAVE, SUB_SET_SERVO_ZERO, SUB_SET_SERVO_AMP, 
    SUB_SET_SERVO_SPEED_FACTOR, SUB_SET_TEMP_MAX_DIFF,
    SUB_RUN, SUB_WAIT, SUB_ALARM, SUB_ERROR};  //  submenus

uint8_t menu;  //  main menu
uint8_t submenu;  //   sub menu

enum {SETUP_MENU_TIME, SETUP_MENU_TEMP, SETUP_MENU_SPEED, SETUP_MENU_SERVO_SPEED_FACTOR, SETUP_MENU_SAVE};
uint8_t menu_select;  //  menu runtime
uint8_t menu_altstate;
enum{MENU_ALT_OFF, MENU_ALT_TEMP, MENU_ALT_SPEED, MENU_ALT_BEGINN, MENU_ALT_END};  //  run variable and states
enum {RUN_OFF, RUN_START, RUN_STOP, RUN_CHECK, RUN_WAIT, RUN_DELAY, RUN_COUNT, RUN_INDEV};
uint8_t runstate;
uint8_t run_delay_timer;

enum{ALT_OFF, ALT_ON}; 
uint8_t altstate;  //  Servo variables
uint8_t calc_next_value;

uint8_t servo_speed;
uint8_t run_factor;
int8_t servo_1_slope;
int8_t servo_2_slope;
uint8_t servo_low;

//  heat pwm variables
enum {PWM_NONE, PWM_BOT, PWM_NEU, PWM_TOP};
uint8_t pwm_status;  
//  temp values to restore after run is finished
uint8_t temp_times;
uint8_t temp_timem;  
//  Prototypes
void lm75_read(void);
void ee_read(void);
void ee_update(void);
void buttoncheck(void);
void enccheck(void);
void init(void);
void zrpos(void);
void zrneg(void);
void lcd_update(uint8_t new_menu, uint8_t new_submenu);
void lcd_value_change(void);
void lcd_put_time(void);
void lcd_put_temp(uint8_t temp);
void settings_change(uint8_t action);
void menu_change(uint8_t action);
void run(uint8_t new_state);
