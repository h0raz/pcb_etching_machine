#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <string.h>
#include <stdlib.h>
#include "./main.h"
#include "./lcd.h"
#include "./i2c.h"
#include "./linsin.h"
#include "./utils.h"

// EEPROM variables for time, speed and temperature
uint8_t EEETCHTIMES EEMEM = 0;
uint8_t EEETCHTIMEM EEMEM = 5;
uint8_t EESPEED EEMEM = 70;
uint8_t EETEMP EEMEM = 108;  // 54 * 2 for easier calculation
uint8_t EESERVOSPEEDFACTOR EEMEM = 6;
uint8_t EELIGHTTIMES EEMEM = 0;
uint8_t EELIGHTTIMEM EEMEM = 5;

uint32_t servo_angle = 0;

uint8_t temp_pwm = 254;
uint8_t temp_pwm_control = 30;


void lm75_read(void) {  // function to get lm75 value
    uint8_t temp_high;
    i2c_send_start(LM75_ADRESS);
    read_temp = i2c_read_ack();
    temp_high = i2c_read_nack();
    i2c_send_stop();
    read_temp = read_temp *2;
    if (bit_is_set(temp_high, 7)) {
        read_temp++;
    }
}


void ee_read(void) {  // load Data from EEPROM, if no value available a stdandard value is used
    uint8_t myetch_times;  // local variables
    uint8_t myetch_timem;
    uint8_t mytemp;
    uint8_t myspeed;
    uint8_t myservospeedfactor;
    uint8_t mylight_times;
    uint8_t mylight_timem;

    myetch_times = eeprom_read_byte(&EEETCHTIMES);  // load data from EEPROM
    myetch_timem = eeprom_read_byte(&EEETCHTIMEM);
    mytemp =  eeprom_read_byte(&EETEMP);
    myspeed = eeprom_read_byte(&EESPEED);
    myservospeedfactor = eeprom_read_byte(&EESERVOSPEEDFACTOR);
    mylight_times = eeprom_read_byte(&EELIGHTTIMES);  // load data from EEPROM
    mylight_timem = eeprom_read_byte(&EELIGHTTIMEM);
    if (myetch_times != 0) {  // Check if data valid (EEPROM standard is 0xff)
        set->etch_times = myetch_times;
    }
    if (myetch_timem != 0) {
        set->etch_timem = myetch_timem;
    }
    if (mytemp != 0) {
        set_temp = mytemp;
    }
    if (myspeed != 0) {
        set->speed = myspeed;
    }
    if (myservospeedfactor != 0) {
        set->servo_speed_factor = myservospeedfactor;
    }
    if (mylight_times != 0) {
        set->light_times = mylight_times;
    }
    if (mylight_timem != 0) {
        set->light_timem = mylight_timem;
    }
}


void ee_update(void) {  // function to update EEPROM (EEPROM writes only if value is changed )
    if (menu == MENU_LIGHT) {
        set->light_times = set->times;
        set->light_timem = set->timem;
    } else {
        set->etch_times = set->times;
        set->etch_timem = set->timem;
    }
    eeprom_update_byte(&EEETCHTIMES, set->etch_times);
    eeprom_update_byte(&EEETCHTIMEM, set->etch_timem);
    eeprom_update_byte(&EESPEED, set->speed);
    eeprom_update_byte(&EETEMP, set_temp);
    eeprom_update_byte(&EESERVOSPEEDFACTOR, set->servo_speed_factor);
    eeprom_update_byte(&EELIGHTTIMES, set->light_times);
    eeprom_update_byte(&EELIGHTTIMEM, set->light_timem);
}


void buttoncheck(void) {  // function to check button
    uint8_t button = BUTTON_PIN & BUTTON;  // get all Pin States
    if (button_state != BUT_LONG && button_state != BUT_SHORT) {  // button check
        if ((button & BUTTON) != BUTTON) {
            if (button_state != BUT_HOLD) {
                button_state = BUT_PRESSED;
                button_count++;
                if (button_count >= 24) {  // button pressed long  // 1000
                    button_state = BUT_LONG;
                }
            }
        } else {
            if (button_state == BUT_PRESSED && button_count >= 4) {  // button pressed short  // 200
                button_state = BUT_SHORT;
            } else {
                button_state = BUT_NONE;
            }
        }
    }
}


void enccheck(void) {  // function to check button
    uint8_t loc_dstate = dstate;
    uint8_t encode = ENC_PIN & (ENC1 | ENC2);  // get encoder pin states
    if (encode != zust_state) {
        if (encode == zust_new_state) {
            zust_count++;
        } else {
            zust_count = 0;
            zust_new_state = encode;
        }
        if (zust_count == 5) {
            zust_state = zust_new_state;  // encoder state for several times

            if (zust_state == ENC2 && loc_dstate != 22) {  // encoder states
                loc_dstate = 11;
            } else if (zust_state == ENC1 && loc_dstate != 12) {
                loc_dstate = 21;
            } else if (loc_dstate == 12 && zust_state == ENC1) {
                loc_dstate = 13;
            } else if (loc_dstate == 21 && zust_state == 0) {
                loc_dstate = 22;
            } else if (loc_dstate == 23 && zust_state == (ENC1 | ENC2)) {
                loc_dstate = 24;
            } else if (loc_dstate == 11 && zust_state == 0) {
                loc_dstate = 12;
            } else if (loc_dstate == 13 && zust_state == (ENC1 | ENC2)) {
                loc_dstate = 14;
            } else if (loc_dstate == 22 && zust_state == ENC2) {
                loc_dstate = 23;
            } else {
                loc_dstate = 0;
            }
        }
    }
    dstate = loc_dstate;
}


inline void calc(uint32_t *angle) {
    uint32_t lins_conv_32;
    uint32_t lins;
    uint8_t servo_1_new;
    uint8_t servo_2_new;
    if (*angle >= 46080) {  // 180°*256 = 1/2 Sinus
        *angle = 0;
        if (runstate == RUN_STOP && run_factor >= 1) {
            run_factor--;
            if (run_factor == 0) {
                runstate = RUN_OFF;
            }
        } else if (run_factor < set->servo_speed_factor) {
            run_factor++;
        }
        if (servo_speed != set->speed) {
            servo_speed = set->speed;
        }
        if (servo_1_slope == SERVO_1_STEPS_DOWN) {
            servo_1_slope = SERVO_1_STEPS_UP;
            servo_2_slope = SERVO_2_STEPS_DOWN;
        } else {
            servo_1_slope = SERVO_1_STEPS_DOWN;
            servo_2_slope = SERVO_2_STEPS_UP;
        }
    }
    lins = linsin(*angle);
    lins_conv_32 = (((lins * servo_1_slope) / 32768) + SERVO_1_MIDDLE);
    servo_1_new = (uint8_t)lins_conv_32;
    lins_conv_32 = (((lins * servo_2_slope) / 32768) + SERVO_2_MIDDLE);
    servo_2_new = (uint8_t)lins_conv_32;
    if (servo_1_new < servo_2_new) {
        ocr1 = servo_1_new;
        ocr2 = (servo_2_new - servo_1_new);
        servo_low = 1;
    } else {
        ocr1 = servo_2_new;
        ocr2 = (servo_1_new - servo_2_new);
        servo_low = 2;
    }
}


void init(void) {  // initialise Ports, Timers etc.
    BUTTON_PORT |= BUTTON;
    SET_OUTPUT(SERVO1);
    SET_OUTPUT(SERVO2);
    ENC_PORT |= ENC1 | ENC2;
    SET_OUTPUT(HEAT);
    SET_OUTPUT(ALARM);
    // Timersettings
    // Timer0
    TCCR0 = (1 << CS01);
    TIMSK |= (1 << TOIE0);
    // Timer1
    TCCR1B = (1 << WGM12) | (1 << CS10);
    TIMSK |= (1 << OCIE1A);
    OCR1A = 100;
    // Timer 2
    TCCR2 |= (1 << CS22) |(1 << CS21)|(1 << WGM21);
    TIMSK |= (1 << OCIE2);
    OCR2 = 250;
    ee_read();  // LM75 Init
    i2c_init();  // Lcd init
    lcd_init(LCD_DISP_ON);
    lcd_update(MENU_ETCH, SUB_NONE);
    runstate = RUN_OFF;
    calc(0);
}


void zrpos(void) {  // function to represent a clock with 60 seconds
    if (set->times == 60) {
        set->times = 0;
        set->timem++;
    }
}


void zrneg(void) {  // function to represent a clock with 60 seconds
    if (set->times == 255) {
        set->times = 59;
        set->timem--;
    }
}


void lcd_update(uint8_t new_menu, uint8_t new_submenu) {  // function to change lcd screen if menu is changed
    lcd_clrscr();
    if (new_submenu == SUB_SETUP_MENU) {  // Setup Menu
        switch (menu_select) {
            case SETUP_MENU_TIME:
            case SETUP_MENU_TEMP:
            case SETUP_MENU_SPEED: lcd_puts("Setup:  Time\n  Temp  Speed"); break;
            case SETUP_MENU_SERVO_SPEED_FACTOR:
            case SETUP_MENU_SAVE: lcd_puts("Setup:  S-Factor\n  Save"); break;
        }
        if (menu_select % 3 == 0) {lcd_gotoxy(7, 0);}  // show which setup point is selected
        else if (menu_select % 3 == 1) { lcd_gotoxy(1, 1);}
        else if (menu_select % 3 == 2) { lcd_gotoxy(7, 1);}
        lcd_puts(">");
    } else if (new_submenu == SUB_ERROR) {
        lcd_puts(" ERROR  RESET\n  Watchdog");
    } else {
        if (new_submenu == SUB_NONE) {  // Main menu
            lcd_puts("Mainmenu Timer\n");
            if (new_menu == MENU_LIGHT) {
                lcd_puts("  Etch  <Expose>");
            } else {
                lcd_puts(" <Etch>  Expose ");
            }
        } else {
            if (new_menu == MENU_LIGHT) {  // show Main menu in submenu
                lcd_puts("Exposing");
            } else {
                lcd_puts("Etching");
            }
            lcd_gotoxy(0, 1);
            switch (new_submenu) {  // submenus
                case SUB_START: lcd_puts("<Start>   Setup "); break;
                case SUB_SETUP: lcd_puts(" Start   <Setup>"); break;
                case SUB_SET_TIME: lcd_puts("Time:"); break;
                case SUB_SET_SPEED: lcd_puts("Speed:"); break;
                case SUB_SET_TEMP: lcd_puts("Temperature:"); break;
                case SUB_SET_SERVO_SPEED_FACTOR: lcd_puts("Speed Factor:"); break;
                case SUB_WAIT: lcd_puts("Heating up"); break;
                case SUB_SET_SAVE:
                    if (altstate == ALT_ON) {
                        lcd_puts("Save: <YES>  NO");
                    } else {
                        lcd_puts("Save:  YES  <NO>");
                    }
                break;
                case SUB_ALARM:
                    if (altstate == ALT_ON) {
                        lcd_puts("!! TIME'S  UP !!");  // alarm text blinking
                    }
                break;
            }
        }
    }
    menu = new_menu;  // take take new submenu
    submenu = new_submenu;
    lcd_value_change();
}


void lcd_value_change(void) {  // function to change values on lcd
    char Buffer[3];
    if (submenu == SUB_START || submenu == SUB_SETUP || submenu == SUB_RUN) {  // show time
        lcd_gotoxy(11, 0);
        lcd_put_time();
    }
    switch (submenu) {  // select menu
        case SUB_SET_TIME: lcd_gotoxy(11, 1); lcd_put_time(); break;
        case SUB_SET_SPEED:
            itoa(set->speed, Buffer, 10);
            lcd_gotoxy(13 - strlen(Buffer), 1);
            lcd_puts(Buffer);
        break;
        case SUB_SET_TEMP:
            lcd_gotoxy(12, 1);
            lcd_put_temp(set_temp);
        break;
        case SUB_RUN:
            if (menu == MENU_ETCH) {
                lcd_gotoxy(0, 1);
                if (menu_altstate == MENU_ALT_TEMP) {
                    lcd_puts(">");
                } else {
                    lcd_puts(" ");
                }
                lcd_put_temp(read_temp);
                lcd_puts("->");
                lcd_put_temp(set_temp);
                lcd_puts(" ");
                if (menu_altstate == MENU_ALT_SPEED) {
                    lcd_puts(">");
                } else {
                    lcd_puts(" ");
                }
                itoa(set->speed, Buffer, 10);
                lcd_puts(Buffer);
            }
        break;
        case SUB_WAIT:
            lcd_gotoxy(9, 0);
            lcd_put_temp(read_temp);
            lcd_puts("->");
            lcd_gotoxy(11, 1);
            lcd_put_temp(set_temp);
        break;
        case SUB_SET_SERVO_SPEED_FACTOR:
            lcd_gotoxy(14, 1);
            itoa(set->servo_speed_factor, Buffer, 10);
            lcd_puts(Buffer);
        break;
    }
}


void lcd_put_time(void) {  // function to put time on display
    char Buffer[3];
    if (set->times == 0 && set->timem == 0) {
        lcd_puts("off");
    } else {
        itoa(set->timem, Buffer, 10);
        if (strlen(Buffer) == 1) {
            lcd_puts("0");
        }
        lcd_puts(Buffer);
        lcd_puts(":");
        itoa(set->times, Buffer, 10);
        if (strlen(Buffer) == 1) {
            lcd_puts("0");
        }
        lcd_puts(Buffer);
    }
}


void lcd_put_temp(uint8_t temp) {  // function to put temperature on display
    char Buffer[3];
    itoa((temp / 2), Buffer, 10);
    lcd_puts(Buffer);
    if (bit_is_set(temp, 0)) {
        lcd_puts(".5");
    } else {
        lcd_puts(".0");
    }
}


void settings_change(uint8_t direction) {  // function to change value in settings
    uint8_t change_status = 30;
    uint8_t *setting_p;
    uint8_t top = 255;
    uint8_t bottom = 0;
    uint8_t sub = submenu;
    if (submenu == SUB_RUN && menu_altstate == MENU_ALT_SPEED) {sub = SUB_SET_SPEED;} else if (submenu == SUB_RUN && menu_altstate == MENU_ALT_TEMP) {sub = SUB_SET_TEMP;}
    switch (sub) {
        case SUB_SET_TEMP: bottom = 20 * 2; top = 70 * 2; setting_p = &set_temp; break;
        case SUB_SET_SPEED: bottom = 1; top = 255; setting_p = &set->speed; break;
        case SUB_SET_SERVO_SPEED_FACTOR: bottom = 1; top = 10; setting_p = &set->servo_speed_factor; break;
        default:
            change_status = 31;
    }
    if (direction) {
        if (*setting_p < top) {
            *setting_p += 1;
        }
    } else {
        if (*setting_p > bottom) {
            *setting_p -= 1;
        }
    }
    if (change_status == 31) {
        switch (sub) {
            case SUB_SET_TIME:
                if (direction) {
                    if (!(set->times == 59 && set->timem == 99)) {
                        set->times += 1;
                        zrpos();
                    }
                } else {
                    if (set->times > 0 || set->timem > 0) {
                        set->times -= 1;;
                        zrneg();
                    }
                }
            break;
        }
    }
}


void menu_change(uint8_t action) {  // function to convert encoder action to menu reaction
    uint8_t new_menu = menu;
    uint8_t new_submenu = submenu;
    switch (action) {
        case ACTION_LEFT:  // encoder turn left
            if (submenu == SUB_NONE && menu == MENU_LIGHT) {
                new_menu = MENU_ETCH;
            } else {
                switch (submenu) {
                    case SUB_SET_TIME:
                    case SUB_SET_SPEED:
                    case SUB_SET_TEMP:
                    case SUB_SET_SERVO_SPEED_FACTOR:settings_change(action); break;
                    case SUB_SETUP: new_submenu = SUB_START; break;
                    case SUB_RUN:
                        if (menu_altstate == MENU_ALT_TEMP || menu_altstate == MENU_ALT_SPEED) {
                            settings_change(action);
                        }
                    break;
                    case SUB_SETUP_MENU:
                        if (menu_select > 0) {
                            menu_select--;
                        }
                    break;
                    case SUB_SET_SAVE:
                        if (altstate == ALT_OFF) {  // save no
                            altstate = ALT_ON;
                        }
                    break;
                }
                lcd_value_change();
            }
        break;
        case ACTION_RIGHT:  // encoder turn right
            if (submenu == SUB_NONE) {
                switch (menu) {
                case MENU_ETCH: new_menu = MENU_LIGHT; break;
                }
            } else {
                switch (submenu) {
                    case SUB_SET_TIME:
                    case SUB_SET_SPEED:
                    case SUB_SET_TEMP:
                    case SUB_SET_SERVO_SPEED_FACTOR: settings_change(action); break;
                    case SUB_START: new_submenu = SUB_SETUP; break;
                    case SUB_RUN:
                        if (menu_altstate == MENU_ALT_TEMP || menu_altstate == MENU_ALT_SPEED) {
                            settings_change(action);
                        }
                    break;
                    case SUB_SETUP_MENU: if (menu_select < (SETUP_MENU_SAVE )) {menu_select++; }
                    case SUB_SET_SAVE:
                        if (altstate == ALT_ON) {  // save no
                            altstate = ALT_OFF;
                        }
                    break;
                }
                lcd_value_change();
            }
        break;
        case ACTION_FORWARD:  // encoder short button
            switch (submenu) {
                case SUB_ERROR: new_submenu = SUB_NONE; break;
                case SUB_NONE:
                    if (new_menu ==MENU_ETCH) {
                        set->times = set->etch_times;
                        set->timem = set->etch_timem;
                    } else {
                        set->times = set->light_times;
                        set->timem = set->light_timem;
                    }
                    new_submenu =  SUB_START;
                break;
                case SUB_START:
                    if (menu == MENU_ETCH) {
                        runstate = RUN_WAIT;
                        new_submenu = SUB_RUN;
                        pwm_status = PWM_NONE;
                        menu_altstate = MENU_ALT_BEGINN;
                    } else {
                        run(RUN_START);
                        new_submenu = SUB_RUN;
                    }
                break;
                case SUB_SET_SAVE:
                    if (altstate == ALT_ON) {
                        ee_update();
                    }
                    new_submenu = SUB_SETUP_MENU;
                break;
                case SUB_SETUP: if (menu == MENU_LIGHT) {new_submenu = SUB_SET_TIME; break;}
                case SUB_SET_TIME: if (menu == MENU_LIGHT) {new_submenu = SUB_SETUP; break;}
                case SUB_SET_SPEED:
                case SUB_SET_SERVO_SPEED_FACTOR:
                case SUB_SET_TEMP: new_submenu =  SUB_SETUP_MENU; break;
                case SUB_ALARM: new_submenu = SUB_START; break;
                case SUB_WAIT: new_submenu = SUB_START; break;
                case SUB_SETUP_MENU:
                    switch (menu_select) {
                        case SETUP_MENU_TIME: new_submenu = SUB_SET_TIME; break;
                        case SETUP_MENU_TEMP: new_submenu = SUB_SET_TEMP; break;
                        case SETUP_MENU_SPEED: new_submenu = SUB_SET_SPEED; break;
                        case SETUP_MENU_SERVO_SPEED_FACTOR: new_submenu = SUB_SET_SERVO_SPEED_FACTOR; break;
                        case SETUP_MENU_SAVE: new_submenu = SUB_SET_SAVE; altstate = ALT_OFF; break;
                    }
                break;
                case SUB_RUN:
                    if (menu == MENU_ETCH) {
                        switch (menu_altstate) {
                            case MENU_ALT_BEGINN: menu_altstate = MENU_ALT_TEMP; break;
                            case MENU_ALT_TEMP: menu_altstate = MENU_ALT_SPEED; break;
                            case MENU_ALT_SPEED: menu_altstate = MENU_ALT_BEGINN; break;
                        }
                    } else {
                        new_submenu = SUB_START;
                    }
                break;
            }
        break;
        case ACTION_BACK:  // encoder pressed long
            switch (submenu) {
                case SUB_ERROR: new_submenu = SUB_NONE; break;
                case SUB_START:
                case SUB_SETUP:
                    if (new_menu == MENU_ETCH) {
                        set->etch_times = set->times;
                        set->etch_timem = set->timem;
                    } else {
                        set->light_times = set->times;
                        set->light_timem = set->timem;
                    }
                    run(RUN_STOP);
                    new_submenu = SUB_NONE;
                break;
                case SUB_WAIT: new_submenu = SUB_START; break;
                case SUB_SET_TIME: if (menu == MENU_LIGHT) {new_submenu = SUB_SETUP; break;}
                case SUB_SET_SPEED:
                case SUB_SET_SAVE:
                case SUB_SET_SERVO_SPEED_FACTOR:
                case SUB_SET_TEMP: new_submenu =  SUB_SETUP_MENU; break;
                case SUB_SETUP_MENU: new_submenu = SUB_SETUP; break;
                case SUB_ALARM: new_submenu = SUB_START; break;
            }
            if (submenu == SUB_RUN) {
                if (menu == MENU_ETCH) {
                    switch (menu_altstate) {
                        case MENU_ALT_BEGINN: run(RUN_STOP); new_submenu =  SUB_START; menu_altstate = MENU_ALT_OFF; break;
                        case MENU_ALT_TEMP: menu_altstate = MENU_ALT_BEGINN; break;
                        case MENU_ALT_SPEED: menu_altstate = MENU_ALT_TEMP; break;
                        case MENU_ALT_END: menu_altstate = MENU_ALT_SPEED; break;
                    }
                } else {
                    new_submenu = SUB_START;
                    run(RUN_STOP);
                }
            }
        break;
    }
    lcd_update(new_menu, new_submenu);
}


void run(uint8_t new_state) {  // function to control run state
    if (new_state == RUN_START) {
        run_factor = 1;
        if (set->times > 0 || set->timem > 0) {
            runstate = RUN_COUNT;
            temp_times = set->times;
            temp_timem = set->timem;
        } else {
            runstate = RUN_INDEV;
        }
    } else if (new_state == RUN_STOP) {
        RESET(HEAT);
        if (runstate == RUN_INDEV) {
            set->times = 0;
            set->timem = 0;
        } else {
            set->times = temp_times;
            set->timem = temp_timem;
        }
        if (menu == MENU_ETCH) {
            runstate = RUN_STOP;
        } else {
            runstate = RUN_OFF;
        }
    }
}


int main() {
    uint8_t mcucsr_mirror = MCUCSR;
    MCUCSR = 0;
    wdt_disable();
    OSCCAL = 0xA3;
    init();
    if (mcucsr_mirror&(_BV(WDRF))) {
        lcd_update(menu, SUB_ERROR);
    }
    set_sleep_mode(SLEEP_MODE_IDLE);
    wdt_enable(WDTO_500MS);
    sei();
    while (1) {
        if (dstate == 24) {
            dstate = 0;
            menu_change(ACTION_RIGHT);
        } else if (dstate == 14) {
            dstate = 0;
            menu_change(ACTION_LEFT);
        }
        if (button_state == BUT_LONG) {
            button_state = BUT_HOLD;
            button_count = 0;
            menu_change(ACTION_BACK);
        } else if (button_state == BUT_SHORT) {
            button_state = BUT_NONE;
            button_count = 0;
            menu_change(ACTION_FORWARD);
        }
        if (runstate == RUN_WAIT && pwm_status == PWM_NEU) {
            runstate = RUN_DELAY;
            run_delay_timer = HEAT_DELAY;
        } else if (calc_next_value == TRUE) {  // calculate next value for servo motor
            calc_next_value = FALSE;
            servo_angle += 64 + servo_speed * run_factor;  // base value plus speed dependency
            calc(&servo_angle);
        }
        if (runstate >= RUN_WAIT && menu == MENU_ETCH) {
            if (read_temp < set_temp - 2 && pwm_status != PWM_BOT) {
                temp_pwm = 254;
                temp_pwm_control += 10;
                pwm_status = PWM_BOT;
            } else if (read_temp >= set_temp - 2 && read_temp  <= set_temp + 2) {
                temp_pwm = temp_pwm_control;
                pwm_status = PWM_NEU;
            } else if (read_temp - 2 > set_temp && pwm_status != PWM_TOP) {
                temp_pwm_control -=10;
                temp_pwm = 0;
                pwm_status = PWM_TOP;
            }
        }
       sleep_mode();
    }
    return 0;
}


ISR(TIMER0_OVF_vect) {  // timer for button polling and temperature pwm
    if (submenu == SUB_ALARM || submenu == SUB_ERROR) {
        TOGGLE(ALARM);
    }
    wdt_reset();
    enccheck();
    if (runstate >= RUN_WAIT && menu == MENU_ETCH) {
        t0_count++;
        if (t0_count == 0) {
            SET(HEAT);
        }
        if (t0_count == temp_pwm) {
            RESET(HEAT);
        }
    }
}


ISR(TIMER1_COMPA_vect) {  // timer for servo controlling
    switch (t1_servo_state) {
        case 0:
            SET(SERVO1);
            SET(SERVO2);
            OCR1A = ocr1 * 100;
            if (ocr2 != 0) {
                t1_servo_state = 1;
            } else {
                t1_servo_state  = 2;
            }
        break;
        case 1:
            OCR1A = ocr2 * 100;
            if (servo_low == 1) {
                RESET(SERVO1);
            } else {
                RESET(SERVO2);
            }
            t1_servo_state = 2;
        break;
        case 2:
            RESET(SERVO1);
            RESET(SERVO2);
            t1_servo_state = 0;
            OCR1A = 65000;
            if (servo_angle != 0 || runstate == RUN_STOP || ((runstate >= RUN_COUNT)&& menu == MENU_ETCH)) {
                calc_next_value = TRUE;
            }
        break;
    }
}


ISR(TIMER2_COMP_vect) {
    buttoncheck();
    if (runstate == RUN_WAIT || runstate == RUN_DELAY|| submenu == SUB_ALARM) {
        if (++t2_count == 63) {
            t2_count = 0;
            if (altstate == ALT_ON) {
                altstate = ALT_OFF;
            } else {
                altstate = ALT_ON;
                lm75_read();
                if (runstate == RUN_DELAY) {
                    if (run_delay_timer > 0) {
                        run_delay_timer--;
                    } else if (run_delay_timer == 0) {
                        run(RUN_START);
                    }
                }
            }
            lcd_update(menu, submenu);
        }
    } else if (runstate >= RUN_COUNT) {
        if (++t2_count == 127) {
            t2_count = 0;
            lm75_read();
            if (runstate == RUN_INDEV) {
                set->times++;
                zrpos();
            } else {
                set->times--;
                if (set->times == 00 && set->timem == 00) {
                    run(RUN_STOP);
                    lcd_update(menu, SUB_ALARM);
                } else {
                    zrneg();
                }
            }
            lcd_value_change();
        }
    } else {
        t2_count = 0;
    }
}
