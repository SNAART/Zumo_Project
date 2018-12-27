/**
* @mainpage ZumoBot Project
* @brief    You can make your own ZumoBot with various sensors.
* @details  <br><br>
    <p>
    <B>General</B><br>
    You will use Pololu Zumo Shields for your robot project with CY8CKIT-059(PSoC 5LP) from Cypress semiconductor.This 
    library has basic methods of various sensors and communications so that you can make what you want with them. <br> 
    <br><br>
    </p>
    
    <p>
    <B>Sensors</B><br>
    &nbsp;Included: <br>
        &nbsp;&nbsp;&nbsp;&nbsp;LSM303D: Accelerometer & Magnetometer<br>
        &nbsp;&nbsp;&nbsp;&nbsp;L3GD20H: Gyroscope<br>
        &nbsp;&nbsp;&nbsp;&nbsp;Reflectance sensor<br>
        &nbsp;&nbsp;&nbsp;&nbsp;Motors
    &nbsp;Wii nunchuck<br>
    &nbsp;TSOP-2236: IR Receiver<br>
    &nbsp;HC-SR04: Ultrasonic sensor<br>
    &nbsp;APDS-9301: Ambient light sensor<br>
    &nbsp;IR LED <br><br><br>
    </p>
    
    <p>
    <B>Communication</B><br>
    I2C, UART, Serial<br>
    </p>
*/


#include <project.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "Motor.h"
#include "Ultra.h"
#include "Nunchuk.h"
#include "Reflectance.h"
#include "Gyro.h"
#include "Accel_magnet.h"
#include "LSM303D.h"
#include "IR.h"
#include "Beep.h"
#include "mqtt_sender.h"
#include <time.h>
#include <sys/time.h>
#include "serial1.h"
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#define PI 3.141592654
/**
 * @file    main.c
 * @brief   
 * @details  ** Enable global interrupt since Zumo library uses interrupts. **<br>&nbsp;&nbsp;&nbsp;CyGlobalIntEnable;<br>
*/

//PRỌECT - FOLLOW THE LINE
#if 0
void zmain(void)
{
    struct sensors_ ref;
    struct sensors_ dig;
    uint8_t count = 0;
    bool onWhite = false;
    bool button=1;
    bool missed = false;
    int first_timestamp = 0;
    int last_timestamp = 0;
    int lap;
    reflectance_start();
    reflectance_set_threshold(9000, 9000, 11000, 11000, 9000, 9000);
    IR_Start();
    motor_start();    
    for(;;)
    {    
        reflectance_read(&ref);
        reflectance_digital(&dig);
        //Check for user button clicked
        if (SW1_Read()== 0)
        { 
            button = !button;
            vTaskDelay(2000);
        
        }
        if (button == 0)
        { 
            motor_forward(255,0);
            //Check for the horizontal line
            if ( onWhite == true && ( dig.l1 == 1 && dig.l2 == 1 && dig.l3 == 1 && dig.r1 == 1 && dig.r2 == 1 && dig.r3 == 1))
            {
                onWhite = false;
                count ++;    
            }
            if ( dig.l3 == 0 && dig.r3 == 0)
            {
                onWhite = true;              
            }
            if (count == 0)
            {
                motor_forward(50,0);
            }
            //Stop on the first line and wait for IR Signal
            
            //Count ++ to get out of the repeat of IR_wait()
            if ( count == 1)
            {            
                motor_forward(0,0);
                send_mqtt("Zumo013/ready", "line");
                IR_wait();
                first_timestamp = xTaskGetTickCount();
                print_mqtt("Zumo013/start", "%d", first_timestamp);
                count ++;
            }
            
            //Stop at the 2nd finishing line
            if ( count == 4)
            {
                last_timestamp=xTaskGetTickCount();
                print_mqtt("Zumo013/stop", "%d", last_timestamp);
                lap = last_timestamp - first_timestamp;
                print_mqtt("Zumo013/time", "%d", lap);
                motor_stop();
                break;
            }
            
            //Check anh print when the robot is on or off the line
            if ( missed == true)
            {
                last_timestamp=xTaskGetTickCount();
                print_mqtt("Zumo013/miss", "%d", last_timestamp);
                for (;;)
                { 
                    reflectance_digital(&dig);
                    if ( dig.l1 == 1 && dig.r1 == 1)
                    {
                        last_timestamp=xTaskGetTickCount();
                        print_mqtt("Zumo013/line", "%d", last_timestamp);
                        missed = false;
                        break;
                    }
                }             
            }
            
            //Centering and following the line
            while ( ref.l1 < 15000 )
            {
                reflectance_read(&ref);
                reflectance_digital(&dig);
                motor_turn(255,20,0); 
                if ( dig.l1 == 0 && dig.r1 == 0)
                {   
                    missed = true;
                }
            }           
            while ( ref.r1 < 15000 )
            {
                reflectance_read(&ref);
                reflectance_digital(&dig);
                motor_turn(20,255,0);
                if ( dig.l1 == 0 && dig.r1 == 0)
                {   
                    missed = true;
                }
            }
        }    
    }
}
#endif

//PROJECT - SUMO
#if 0
void zmain(void)
{
    struct sensors_ ref;
    struct sensors_ dig;
    struct accData_ data;
    uint8_t count = 0;
    uint8_t b_count = 0;
    bool onWhite = false;
    bool button = 1;
    int first_timestamp = 0;
    int last_timestamp = 0;
    int lap;
    int r;
    
    reflectance_start();
    reflectance_set_threshold(9000, 9000, 11000, 11000, 9000, 9000);
    IR_Start(); 
    LSM303D_Start();
    motor_start();
    for(;;)
    {
        reflectance_read(&ref);
        reflectance_digital(&dig);
        
        //Check for the user button clicked
        if (SW1_Read()== 0)
        { 
           
            button = !button;
            vTaskDelay(1500);
            b_count ++;
            
            //Stop the robot and print when the user button pressed for the 2nd time.
            if ( b_count == 2 )
            {
                last_timestamp=xTaskGetTickCount();
                print_mqtt("Zumo013/stop", "%d", last_timestamp);
                lap = last_timestamp - first_timestamp;
                print_mqtt("Zumo013/time", "%d", lap);
                motor_stop();
                break;
                
            }
        }
        if (button == 0)
        {                   
            int first_dataX;
            int second_dataX;
            int first_dataY;
            int second_dataY;
            int diff_dataX;
            int diff_dataY;
            double angle;
            LSM303D_Read_Acc(&data);
            first_dataX=data.accX;
            LSM303D_Read_Acc(&data);
            second_dataX=data.accX;
            LSM303D_Read_Acc(&data);
            first_dataY=data.accY;
            LSM303D_Read_Acc(&data);
            second_dataY=data.accY;
            diff_dataX = first_dataX-second_dataX;
            diff_dataY = first_dataY-second_dataY;
            if (count == 0)
            {                             
                motor_forward(50,0);
            }
            
            //Stop at first line and wait for the IR Signal
            //Count ++ to get out of the repeat of IR_wait()
            if ( count == 1)
            {            
                motor_forward(0,0);                    
                send_mqtt("Zumo013/ready", "zumo");
                IR_wait();
                first_timestamp = xTaskGetTickCount();
                print_mqtt("Zumo013/start", "%d", first_timestamp);
                motor_forward(255,0); 
                count ++;            
            }
            
            //The difference of accelerations can show when the robot is hitted or not. Print when the robot is hitted.            
            //Calculate the angle hit with Accelerations by X-axis and Y-axis            
            if ( diff_dataX > 10000 || diff_dataY > 10000)
            {
                if ( dig.l1 == 0 && dig.l1 == 0 && dig.r2 == 0 && dig.l2 == 0 )
                {
                    last_timestamp = xTaskGetTickCount();
                    angle = (atan2((double) data.accX, data.accY)*180)/PI;
                    if ( angle < 0 )
                    {
                        angle = angle + 360;
                    }
                    print_mqtt("Zumo013/hit", "%d  %.2f", last_timestamp, angle);
                }
            }
            if ( onWhite == true && dig.l2 == 1 && dig.r2 ==1 && dig.l3 == 1 && dig.r3 ==1)
            {
                onWhite = false;
                count ++; 
                
            }
            
            //Check when the robot meets the border of the circle and make the robot going backward and turn randomly
            if ( (ref.l3 > 14000 && ref.l2 > 14000) || (ref.r1 > 15000 || ref.l1 > 15000) || (ref.r3 > 14000 && ref.r2 > 14000))
            {   
                if ( count > 2)
                {             
                    r = rand();
                    motor_backward(180,300);
                    if (r%2==0) 
                    {                      
                       motor_tank_left(255,200);                
                    } 
                    else 
                    {
                       motor_tank_right(255,200);
                    }
                    motor_forward(255,0);
                } 
            }
            if ( dig.l3 == 0 && dig.r3 == 0)
            {
                onWhite = true;              
            }          
        }    
    }
}
#endif

//PROJECT - MAZE
#if 1
void zmain(void)
{
    struct sensors_ ref;
    struct sensors_ dig;
    int count = 0;
    int x_coordinate = 0;
    int y_coordinate = 0;
    int first_timestamp = 0;
    int last_timestamp = 0;
    int lap;
    int d;
    bool obs = false;  
    bool onWhite = false;
    bool onWhite_2 = false;
    bool button = 1;
    bool onBlack = true;
    bool finished = false;
    bool side = true;
    Ultra_Start();
    reflectance_start();
    reflectance_set_threshold(9000, 9000, 11000, 11000, 9000, 9000);
    IR_Start();
    motor_start();    
    for(;;)
    {    
        reflectance_read(&ref);
        reflectance_digital(&dig);
        
        //Check for the user button clicked
        if (SW1_Read()== 0)
        { 
            button = !button;
            vTaskDelay(2000);
        
        }
        if (button == 0)
        { 
            motor_forward(180,0);
            d = Ultra_GetDistance();
            
            //Robot turns when meets an obstacle
            if ( d < 10 )
            {         
                //When the robot meet an obstacle at the edges, it will turn in the opposite direction
                if ( x_coordinate == 3 || x_coordinate == -3)
                {
                    side = !side;
                }
                
                //Turn right when meets obstacle
                if ( side == true )
                {
                    while ( onBlack == true )
                    {        
                        reflectance_digital(&dig);
                        motor_tank_right(180,0);
                        if ( dig.l1 == 0 && dig.r1 == 0)
                        {
                            onBlack = false;
                        }
                    }
                    while ( onBlack == false)
                    {
                        reflectance_read(&ref);
                        motor_tank_right(180,0);
                        if ( ref.l1 > 22000 && ref.r1 > 22000 )
                        {
                            obs = true;
                            onBlack = true;
                        }                   
                    }
                    continue;
                }
                
                //Turn left when meets obstacle
                if ( side == false )
                {
                    while ( onBlack == true )
                    {        
                        reflectance_digital(&dig);
                        motor_tank_left(180,0);
                        if ( dig.l1 == 0 && dig.r1 == 0)
                        {
                            onBlack = false;
                        }
                    }
                    while ( onBlack == false)
                    {
                        reflectance_read(&ref);
                        motor_tank_left(180,0);
                        if ( ref.l1 > 22000 && ref.r1 > 22000 )
                        {
                            obs = true;
                            onBlack = true;
                        }                 
                    }
                }
            }
            if ( obs == true )
            {
                
                //Run forward for 1 unit and turn the robot to the vertical direction.
                for (;;)
                {
                    reflectance_read(&ref);
                    motor_forward(180,0);
                    while ( ref.l1 < 18000)
                    {
                        reflectance_read(&ref);
                        motor_turn(255,0,0);     
                    }           
                    while ( ref.r1 < 18000 )
                    {
                        reflectance_read(&ref);
                        motor_turn(0,255,0);
                    }
                    if ( ref.l1 > 22000 && ref.r1 > 22000 && ref.l2 > 18000 && ref.r2 > 18000 && ref.l3 > 18000 && ref.r3 > 18000 )
                    {                       
                        if ( side == true )
                        {
                            x_coordinate ++;
                        }
                        if ( side == false )
                        {
                            x_coordinate --;
                        }
                        motor_forward(180,80);
                        onBlack = true;
                        print_mqtt("Zumo013/position", "%d %d", x_coordinate, y_coordinate);
                        break;
                    } 
                }               
                while ( onBlack == true )
                {
                    reflectance_digital(&dig);
                    if ( side == true )
                    {
                        motor_tank_left(180,0);
                    }
                    if ( side == false )
                    {
                        motor_tank_right(180,0);
                    }
                    if ( dig.l1 == 0 && dig.r1 == 0 )
                    {
                        onBlack = false;
                    }
                }
                while ( onBlack == false)
                {
                    reflectance_read(&ref);
                    if ( side == true )
                    {
                        motor_tank_left(180,0);
                    }
                    if ( side == false )
                    {
                        motor_tank_right(180,0);
                    }
                    if ( ref.l1 > 22000 && ref.r1 > 22000 )
                    {
                        onBlack = true;
                        obs = false;
                    }   
                }
            }
            
            //When the y-coordinate is 11, make the robot to run to the middle of the maze (x = 0) and turn the robot the the vertical direction
            if ( y_coordinate == 11 && x_coordinate !=0)
            {
                motor_forward(180,30);
                while ( onBlack == true )
                {
                    reflectance_digital(&dig);
                    if ( x_coordinate > 0 )
                    {
                    motor_tank_left(180,0);
                    }
                    if ( x_coordinate < 0 )
                    {
                    motor_tank_right(180,0);
                    }
                    if ( dig.l1 == 0 && dig.r1 == 0 )
                    {
                        onBlack = false;
                    }
                }
                while ( onBlack == false)
                {
                    reflectance_read(&ref);
                    if ( x_coordinate > 0 )
                    {
                        motor_tank_left(180,0);
                    }
                    if ( x_coordinate < 0 )
                    {
                        motor_tank_right(180,0);
                    }
                    if ( ref.l1 > 22000 && ref.r1 > 22000 )
                    {
                        obs = false;
                        onBlack = true;
                    }
                }
                while ( x_coordinate!=0)
                {
                    reflectance_digital(&dig);
                    reflectance_read(&ref);
                    motor_forward(180,0);
                    while ( ref.l1 < 18000)
                    {
                        reflectance_read(&ref);
                        motor_turn(255,0,0);     
                    }           
                    while ( ref.r1 < 18000 )
                    {
                        reflectance_read(&ref);
                        motor_turn(0,255,0);
                    }
                    if ( dig.l3 == 0 && dig.r3 == 0)
                    {
                        onWhite_2 = true;
                    }
                    
                    if ( onWhite_2 == true && (( dig.l1 == 1 && dig.l2 == 1 && dig.l3 == 1 ) ||( dig.r1 == 1 && dig.r2 == 1 && dig.r3 == 1 )))
                    {
                        if ( x_coordinate > 0 )
                        {
                            x_coordinate --;
                            if ( x_coordinate == 0 )
                            {                          
                                motor_forward(180,50);
                                while ( onBlack == true )
                                {
                                    reflectance_digital(&dig);
                                    motor_tank_right(180,0);
                                    if ( dig.l1 == 0 && dig.r1 == 0 )
                                    {
                                        onBlack = false;
                                    }
                                }
                                while ( onBlack == false)
                                {
                                    reflectance_read(&ref);
                                    motor_tank_right(180,0);
                                    if ( ref.l1 > 22000 && ref.r1 > 22000 )
                                    {
                                        onBlack = true;
                                    }
                                }
                            }
                        }
                        if ( x_coordinate < 0)
                        {
                            x_coordinate ++;
                            if ( x_coordinate == 0 )
                            {                          
                                motor_forward(180,50);
                                while ( onBlack == true )
                                {
                                    reflectance_digital(&dig);
                                    motor_tank_left(180,0);
                                    if ( dig.l1 == 0 && dig.r1 == 0 )
                                    {
                                        onBlack = false;
                                    }
                                }
                                while ( onBlack == false)
                                {
                                    reflectance_read(&ref);
                                    motor_tank_left(180,0);
                                    if ( ref.l1 > 22000 && ref.r1 > 22000 )
                                    {
                                        onBlack = true;
                                    }
                                }
                            }
                        }
                        
                        print_mqtt("Zumo013/position", "%d %d", x_coordinate, y_coordinate);
                        
                        onWhite_2 = false;
                    }
                    
                    
                }
                    
            }
            
            //Stop at the first line and wait for IR-Signal
            //Count ++ to get out of the repeat of IR_wait()
            if ( count == 1)
            {
                motor_forward(0,0);
                send_mqtt("Zumo013/ready", "maze");
                IR_wait();
                first_timestamp = xTaskGetTickCount();
                print_mqtt("Zumo013/start", "%d", first_timestamp);
                count ++;
            }
            
            //Reset the x-coordinate, y-coordinate when robot entering the maze
            if ( count == 3 )
            {
                x_coordinate = 0;
                y_coordinate = 0;
                print_mqtt("Zumo013/position", "%d %d", x_coordinate, y_coordinate);
                count++;
            }
            
            //Finishing and print MQTT
            if ( finished == true )
            {
                motor_stop();
                last_timestamp=xTaskGetTickCount();
                print_mqtt("Zumo013/stop", "%d", last_timestamp);
                lap = last_timestamp - first_timestamp;
                print_mqtt("Zumo013/time", "%d", lap);           
                break;
            }
            
            if ( dig.l3 == 0 && dig.r3 == 0)
            {
                onWhite = true;
            }
            
            if ( onWhite == true && (( dig.l1 == 1 && dig.l2 == 1 && dig.l3 == 1 ) ||( dig.r1 == 1 && dig.r2 == 1 && dig.r3 == 1 )))
            {
                count ++;
                y_coordinate ++;
                if ( count > 3 )
                {
                    print_mqtt("Zumo013/position", "%d %d", x_coordinate, y_coordinate);
                }
                onWhite = false;
            }
            
            //Centering the robot in the line
            while ( ref.l1 < 18000)
            {
                reflectance_read(&ref);
                motor_turn(255,0,0);
                
                //Turn to stopping process when y = 13 and 2 middle sensors are on white
                if (ref.l1 < 6000 && ref.r1 < 6000 && x_coordinate == 0 && y_coordinate == 13) 
                {
                    finished = true;
                    break;
                }
            }           
            while ( ref.r1 < 18000 )
            {
                reflectance_read(&ref);
                motor_turn(0,255,0);
                
                 //Turn to stopping process when y = 13 and 2 middle sensors are on white
                if (ref.l1 < 6000 && ref.r1 < 6000 && x_coordinate == 0 && y_coordinate == 13) 
                {
                    finished = true;
                    break;
                }
            }
        }    
    }
}
#endif
/* [] END OF FILE */
