#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdint.h>
#include "getch.h"
#include <wiringPi.h>

#define I2C_DEV         "/dev/i2c-1"
#define CLOCK_FREQ      25000000.0
#define PCA_ADDR        0x40
#define LED_STEP        50

#define LEFT            68
#define RIGHT           67
#define UP              65
#define DOWN            66
#define MAX             4050-40
#define MIN             840+40
#define C_MAX           510
#define C_MIN           100

// Register Addr
#define MODE1           0x00
#define MODE2           0x01

#define LED15_ON_L      0x42
#define LED15_ON_H      0x43
#define LED15_OFF_L     0x44
#define LED15_OFF_H     0x45

#define LED14_ON_L      0x3E
#define LED14_ON_H      0x3F
#define LED14_OFF_L     0x40
#define LED14_OFF_H     0x41

#define LED13_ON_L      0x3A
#define LED13_ON_H      0x3B
#define LED13_OFF_L     0x3C
#define LED13_OFF_H     0x3D

#define LED4_ON_L       0x16
#define LED4_ON_H       0x17
#define LED4_OFF_L      0x18
#define LED4_OFF_H      0x19

#define LED5_ON_L       0x1A
#define LED5_ON_H       0x1B
#define LED5_OFF_L      0x1C
#define LED5_OFF_H      0x1D

// Set GPIO
#define GPIO17          0
#define GPIO27          2

#define PRE_SCALE       0xFE



int fd;
unsigned char buffer[3] = {0};
unsigned short front_wheel;
short back_wheel;
unsigned short cam_UD;
unsigned short cam_LR;


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
    
    //printf("addr[%d] = %d\n", addr, buffer[0]);

    return 0;

}

int reg_read16(unsigned char addr)
{
    unsigned short temp;

    reg_read8(addr);
    temp = 0xff & buffer[0];         // 상위 8bit 값만 temp에 입력
    
    reg_read8(addr + 1);
    temp |= (buffer[0] << 8);        // 하위 8bit 값만 temp에 입력

    
    //printf("addr = 0x%x, data = 0x%x, data = %d\n", addr, temp, temp);

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
    int length; 
    
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

    reg_write8(MODE2, 0x04); // 0x10
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
    int freq = 50;
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

/*    //width = 2ms/90도
    if(key == '1')
        freq_step = 410;

    //width = 1.5ms/0도
    else if(key == '0')
        freq_step = 308;

    //width = 1ms/-90도
    else
        freq_step = 205; */

    return 0;
}

void servoOFF(void)
{
    reg_write8(MODE1, 0x10);
}

int Move(short speed)
{
    short max = 4095;
    short min = -4095;
        
    if(speed > 0)
    {
        digitalWrite(GPIO17, LOW);
        digitalWrite(GPIO27, LOW);
    }

    if(speed < 0)
    {
        speed *= -1;
        digitalWrite(GPIO17, HIGH);
        digitalWrite(GPIO27, HIGH);
    }
    
    reg_write16(LED4_ON_L, 0);
    reg_write16(LED5_ON_L, 0);
    reg_write16(LED4_OFF_L, speed);
    reg_write16(LED5_OFF_L, speed);
 //   printf("back_wheel = %d\n", b);
 //   printf("speed = %d\n",speed);
}
   

int blinkLED(void)
{
    int i;
   // char key = 'd';
    int input_h;
    char input_c;
    char input_n;
    int loop = 1;

    //unsigned short value;
    //unsigned short max = 4095;

    front_wheel     = 300;
    back_wheel      = 2000;
    cam_UD          = 300; 
    cam_LR          = 300;


    while(loop)
    {
       input_h = getch();
       //if(input_h == 91)
       //input_h = getch();

       if(input_h == 91)
       {
           input_h = getch();
               
           switch(input_h)
           {
               //REG 15 // front_wheel
               case LEFT:                              
                   if(front_wheel > 195)
                       front_wheel -= 5;
                   break;
          
               case RIGHT:
                   if(front_wheel < 405)
                       front_wheel += 5;   
                   break;
                   
               //back_wheel
               case UP:
                   if(back_wheel <= MAX)                       
                       back_wheel += 40;
             
                   break;
               
               case DOWN:
                   if(back_wheel > -MAX)                   
                       back_wheel -= 40;
                   break;
           }

       }

       else
       {
           switch(input_h)
           {
               //REG 14 // cam_UD
               case 'w':
                   if(cam_UD > C_MIN)
                       cam_UD -= 5;
                   break;
                   
               case 's':
                   if(cam_UD < C_MAX)
                       cam_UD += 5;   
                   break;

               //REG 13 // cam_LR
               case 'd':
                   if(cam_LR > C_MIN)
                       cam_LR -= 5;
                   break;
                   
               case 'a':
                   if(cam_LR < C_MAX)
                       cam_LR += 5;
                   break;

               // MAX motol speed
               case 'f':
                   if(back_wheel > 0)
                       back_wheel = MAX;
                   else
                       back_wheel = -MAX;
                   break;

               // MIN motol speed
               case 'r':
                   if(back_wheel > 0)
                       back_wheel = MIN;
                   else
                       back_wheel = -MIN;
                   break;

               // STEP motol speed
               case 'n':
                   back_wheel = 0;
                   break;
               
               // Change direction
               case 'c':
                   back_wheel *= -1;
                   break;

               // STOP    
               case 'p':  
                   loop = 0;
                   break;       
           }
       }

       Move(back_wheel);
       printf("%d\n",back_wheel);

       reg_write16(LED13_ON_L, 0);
       reg_write16(LED13_OFF_L, cam_LR);

       reg_write16(LED14_ON_L, 0);
       reg_write16(LED14_OFF_L, cam_UD);

       reg_write16(LED15_ON_L, 0);
       reg_write16(LED15_OFF_L, front_wheel);
       usleep(5000);
       //printf("front_wheel : %d\n", front_wheel);
    }
}

/*int led_on(unsigned short value)
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
}*/



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


    wiringPiSetup();
    pinMode(GPIO17, OUTPUT);
    pinMode(GPIO27, OUTPUT);
    pca9685_restart();
    pca9685_freq();
    //led_on(value);
    blinkLED();

    servoOFF();

    return 0;
}






