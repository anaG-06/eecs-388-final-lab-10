/******************************************************************************
 *   Header Includes
 *******************************************************************************/
#include <Arduino.h>
#include <stdint.h>
#include <stdio.h>

#include "eecs_388_lib.h"

/******************************************************************************
 *   Constant definitions
 *******************************************************************************/

/*ServoMotor constants*/
#define SERVO_PULSE_MAX (2400)    /* 2400 us */
#define SERVO_PULSE_MIN (544)     /* 544 us */
#define SERVO_PERIOD    (20000)   /* 20000 us (20ms) */
#define MIN_ANGLE       (0)       /* degrees */
#define MAX_ANGLE       (180)     /* degrees */

#define RED GPIO_13
#define GREEN GPIO_12
#define BLUE GPIO_11

//variables used in an ISR should be volatile and global
volatile uint8_t SAFE_STATE; //d > 200cm
volatile uint8_t BRAKE_LIGHTLY;// 100cm < d <= 200cm
volatile uint8_t BRAKE_HARD; // 60cm < d <= 100cm
volatile uint8_t UNSAFE_STATE; // d >= 60cm

volatile uint32_t dist;

volatile uint8_t led_state = 0;





/******************************************************************************
 *   Local function prototypes
 *******************************************************************************/
static void auto_brake();
static void steering(int pos);
uint16_t receive_distance();

/******************************************************************************
 *   Function: auto_brake() - Auto Brake
 *      Pre condition: 
 *          None
 *      Post condition: 
 *          Checks the LiDAR distance and configures LEDs
 *******************************************************************************/
ISR(TIMER1_COMPA_vect)
{
    //every 100ms, the ISR will check the distance and change the LED accordingly
    //written by Ana
    led_state = !led_state; //blinks light for red

    gpio_write(RED,OFF);
    gpio_write(GREEN,OFF);
    gpio_write(BLUE,OFF);

    if (dist > 200) //green
    {
      gpio_write(GREEN,ON);
    }
    else if (dist > 100) //yellow
    {
      gpio_write(GREEN,ON);
      gpio_write(RED,ON);
    }
    else if (dist > 60) //red
    {
      gpio_write(RED,ON);
    }
    else //flashing red
    {
      gpio_write(RED, led_state);
    }

}

 static void auto_brake()
{
    // Task-2: 
    // Your code goes here (Use Lab 2 & 4 for reference)
    // Check the project document to understand the task

    //distance will change with delays to test led changes
    //will change to actual values from ardiuno_1 in task 3

    // // loops thru different dists for testing
    // dist = 300;
    // ser_printf("dist: %d", dist);
    // delay_ms(1000);

    // dist = 30;
    // ser_printf("dist: %d", dist);
    // delay_ms(1000);
    
    // dist = 85;
    // ser_printf("dist: %d", dist);
    // delay_ms(1000);
    
    
    // dist = 150;
    // ser_printf("dist: %d", dist);
    // delay_ms(1000);
    //receive distance values from Arduino1
    
    dist = receive_distance();
    ser_printf("distance: %d", dist);


}


/******************************************************************************
 *   Function: steering() - Steering
 *      Pre condition: 
 *          None
 *      Post condition: 
 *          Control the servomotor with GPIO_6
 *******************************************************************************/
static void steering(int pos)
{
  // Task-4: 
    
  /*
  if we treat (0, 544) and (180, 2400) as points on a graph, 
  we can find their slope and find the function for our linear equation:
  pwm_signal = (((servo_mas - servo_min) * angle)/180 ) + 544
  pwm_signal = 10.31(angle) + 544
  */

  uint32_t slope = (SERVO_PULSE_MAX - SERVO_PULSE_MIN)/180; // ~= 10.31
  uint16_t pwm_signal = (slope * pos) + 544; //us

  //instead of delaying; ISR timer?
  /*
  steps
  calc pwm_signal
  turn pwm on
  set ISR cycle
    inside isr: turn pwm off
  repeat
  */

  gpio_write(GPIO_6, ON);
  delay_us(pwm_signal);
  gpio_write(GPIO_6, OFF);
  delay_ms((SERVO_PERIOD/1000) - (pwm_signal/1000)); // OFF for remaining time of period (in ms)

  return;
      
}

/******************************************************************************
 *   Function: receive_distance() - Receive distance from arduino 1 and update dist 
 *   written by Brayden
 *******************************************************************************/
uint16_t receive_distance()
{
  while (1) //ensures dist is transmitted
  {
    uint8_t first = (uint8_t)ser_read();

    if (first == 0xAA) //check for header byte 1
    {
      uint8_t second = (uint8_t)ser_read();

      if (second == 0x55) //check for header byte 2
      {
          //combine high and low from ARD1 into a 16 bit value
          uint8_t dist_l = (uint8_t)ser_read();
          uint8_t dist_h = (uint8_t)ser_read();

          uint16_t dist = ((uint16_t)dist_h << 8) | dist_l;

          return dist;
      }
    }
  }
}

/******************************************************************************
 *   Function: setup() - Initializes the Arduino System
 *      Pre condition: 
 *          Hardware must be properly connected (BMP180 sensors, etc.)
 *      Post condition: 
 *          Runs initialization calls one time on power up
 *          UART is initialized for ser_printf()
 *          I2C is initialized for communication with BMP180
 *          calib_data is filled with calibration data from BMP180
 *******************************************************************************/
void setup() 
{
uart_init();     // Initialize UART for serial output

//Setup Auto-break LEDs for Distance
gpio_mode(GPIO_13, GPIO_OUTPUT); //RED
gpio_mode(GPIO_12, GPIO_OUTPUT); //GREEN
gpio_mode(GPIO_11, GPIO_OUTPUT); //BLUE


//set up for ISR-----------------
//setup timer for 100 ms
enable_timer_interrupt();
set_cycles(6250); //256 //amount of cycles for a 100ms interrupt timer

enable_interrupt();

//Setup GPIO_6 for PWM output
gpio_mode(GPIO_6, GPIO_OUTPUT);

ser_printf("System Initialized");
}

/******************************************************************************
 *   Function: loop() - Main execution loop
 *      Pre condition: 
 *          setup() has been executed and system is initialized
 *      Post condition: 
 *          Performs a single iteration of the system's function
 *          Repeates indefinetely unless the board is reset or powered off
 *******************************************************************************/
void loop() 
{
// Task-4: 
// Setup simulated code for the angles from the lab sheet

//angles
int angle_values[] = {10, 25, 75, 45, 100, 40, 125, 15, 150, 50, 170};
int angle;

for (int i = 0; i < 10; i++)
{
  angle = angle_values[i];
  ser_printf("angle: %d", angle);
  // Task-1&2:
  // Receive data from arudino1 for the distance and setup the LEDs
  auto_brake();

  for( int j = 0; j < 50; j++ )//controls servo for one second
  {
    steering(angle);
  }

}


}