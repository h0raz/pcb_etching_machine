void i2c_init(void);
void i2c_send_stop(void);
int i2c_read_ack(void);
unsigned char i2c_read_nack(void);
int i2c_send_start(uint8_t adress);