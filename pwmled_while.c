#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdint.h>

#define I2C_DEV         "/dev/i2c-1"
#define CLOCK_FREQ      1000.0
#define PCA_ADDR        0x40
#define LED_STEP        50

// Register Addr
#define MODE1           0x00
#define MODE2           0x01
#define LED15_ON_L      0x42
#define LED15_ON_H      0x43
#define LED15_OFF_L     0x44
#define LED15_OFF_H     0x45
#define PRE_SCALE       0xFE

int fd;
unsigned char buffer[3] = {0};

int reg_read8(unsigned char addr)
{
    int length = 1;
    buffer[0] = addr;

    if(write(fd, buffer, length) != length)
    {
        printf("Failed to write frome the i2c bus\n");
    }

    if(read(fd , buffer, length) != length)
    {
        printf("Failed to read from the i2c bus\n");
    }
    
    printf("addr[%d] = %d\n", addr, buffer[0]);

    return 0;

}

int reg_read16(unsigned char addr)
{
    unsigned short temp;

    reg_read8(addr);
    temp = 0xff & buffer[0];         // 상위 8bit 값만 temp에 입력
    
    reg_read8(addr + 1);
    temp |= (buffer[0] << 8);        // 하위 8bit 값만 temp에 입력

    
    printf("addr = 0x%x, data = 0x%x, data = %d\n", addr, temp, temp);

    return 0;
}

int reg_write8(unsigned char addr,unsigned char data)
{
    int length = 2;
    
    buffer[0] = addr;
    buffer[1] = data;

    if(write(fd , buffer, length) != length)
    {
        printf("Failed to write from the i2c bus\n");
        return -1;
    }

    return 0;
}

int reg_write16(unsigned char addr, unsigned short data)
{
    int length = 2;

    reg_write8(addr, (data & 0xff));
    reg_write8(addr+1, ((data>>8) & 0xff));

    return 0;
}

int pca9685_restart(void)
{
    int length = 2; 
    
    if(ioctl(fd, I2C_SLAVE, PCA_ADDR) < 0)
    {
        printf("Failed to acquire bus access and/or talk to slave\n");
        return -1;
    }
    
    reg_write8(MODE1, 0x00);
/*
 * buffer[0] = MODE1;
 * buffer[1] = 0x00;
 * length = 2;

    // start [I2C_ADDR = 0x40] // write '0x00' in MODE1 resister
    // [S][I2C_ADDR][W][ACK][MODE1][ACK][0x00][ACK][P]  // [P] 는 stop을 의미
 * if(write(fd, buffer, length) != length)
 * {
 * printf("Failed to write from the i2c bus\n");
 * return -1;
 * }

 * else
 * {
 * printf("Data Mode1 %x\n", buffer[1]);
 * }
*/

    reg_write8(MODE2, 0x04);
/* 
 * buffer[0] = MODE2;
 * buffer[1] = 0x04;
 * length = 2;

    // [S][I2C_ADDR][W][ACK][MODE2][ACK][0x04][ACK][P]  // [P] 는 stop을 의미
 * if(write(fd, buffer, length) != length)
 * {
 * printf("Failed to write from the i2c bus\n");
 * return -1;
 * }
 
 * else
 * {
 * printf("Data Mode2 %x\n", buffer[1]);
 * }
*/
    return 0;
}

int pca9685_freq()
{
    int length = 2, freq = 10;
    uint8_t prescale_val = (CLOCK_FREQ / 4096 / freq) -1; // preescale value. ref)Data sheet
    printf("prescale_val = %d\n",prescale_val);

    // OP : OSC OFF
    reg_write8(MODE1, 0x10);
/* 
 * buffer[0] = MODE1;
 * buffer[1] = 0x10;
 * if(write(fd, buffer, length) != length)
 * {
 * printf("Failed to write frome the i2c bus\n");
 * return -1;
 * }
*/
    
    // OP : WRITE PRE_SCALE VALUE
    reg_write8(PRE_SCALE,prescale_val);
/* 
 * buffer[0] = PRE_SCALE; 
 * buffer[1] = prescale_val;
 * if(write(fd, buffer, length) != length)
 * {
 * printf("Failed to write frome the i2c bus\n");
 * return -1;
 * }
*/

    // OP : RESTART
    reg_write8(MODE1, 0x80);
/*
 * buffer[0] = MODE1;
 * buffer[1] = 0x80;
 
 * if(write(fd, buffer, length) != length)
 * {
 * printf("Failed to write frome the i2c bus\n");
 * return -1;
 * }
*/

    // OP : TOTEM POLE
    reg_write8(MODE2, 0x04);
/* 
 * buffer[0] = MODE2;
 * buffer[1] = 0x04;

 * if(write(fd, buffer, length) != length)
 * {
 * printf("Failed to write frome the i2c bus\n");
 * return -1;
 * }
*/
    return 0;
}

int led_on(unsigned short value)
{
    char key;
    unsigned short time_val = 4095;

   while(1)
   {
       if(value <= time_val)
       {
           while(value <= time_val)
           {
               value += LED_STEP;               
               printf("O : value = %d\n",value);
               reg_write16(LED15_ON_L, 0);     //16bit이기 때문에 High, Low 둘다 써짐
               reg_read16(LED15_ON_L);
                
               reg_write16(LED15_OFF_L, value);
               reg_read16(LED15_OFF_L);
           }       
        }
        
       else if(value >= time_val)
       {   
           while(value >= LED_STEP)
           {
               value -= LED_STEP;
               printf("U : value = %d\n",value);
               reg_write16(LED15_ON_L, 0);                 // LED ON 지점은 고정
               reg_read16(LED15_ON_L);
               
               reg_write16(LED15_OFF_L, value);            // LED OFF 지점을 옮김
               reg_read16(LED15_OFF_L);


           }
       }    
   }

    return 0;
}

int main(void)
{
    unsigned short value = 2047;

    if((fd = open(I2C_DEV, O_RDWR)) < 0)
    {
        printf("Failed open i2c-1 bus\n");
        return -1;
    }

    if(ioctl(fd,I2C_SLAVE,PCA_ADDR) < 0)
    {
        printf("Failed to acquire bus access and/or talk to slave\n");
        return -1;
    }

    pca9685_restart();
    pca9685_freq();
    led_on(value);

    return 0;
}






