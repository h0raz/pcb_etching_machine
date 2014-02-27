//i2c Befehle von 
//http://www.rclineforum.de/forum/board49-zubeh-r-elektronik-usw/board72-elektronik-spezial-eigene-scha/board92-atmel-programmierung-f-r-einst/fragen-zur-win-avr-programmier/112742-probleme-mit-i-c-lm75/Seite_3
#include <avr/io.h>
#include "i2c.h"

void i2c_init(void) {
    TWBR = (1 << 5) | (1 << 1) | (1 << 3); //TWBR = 11
}

void i2c_send_stop(void){
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
    while(!(TWCR & (1 << TWSTO)));
}

int i2c_read_ack(void){
    TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
    while(!(TWCR & (1<<TWINT)));
        if(!(TWSR & 0x50))
            return -1;
    return TWDR;
}

unsigned char i2c_read_nack(void){
    TWCR = (1<<TWINT) | (1<<TWEN);
    while(!(TWCR & (1<<TWINT)));
    if(!(TWSR & 0x58))
        return -1;
    return TWDR;
}

int i2c_send_start(uint8_t adress){    
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); //Sende start Condition
    while(!(TWCR & (1 << TWINT))); //Warte bis ende
    if(!(TWSR & 0x08))
        return -1;
    TWDR = adress | 1;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while(!(TWCR & (1 << TWINT)));
    if(!(TWSR & 0x40))
        return(-2);
    return 0;
}
