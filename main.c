#include "stm32f10x.h"                  // Device header
#include "stdlib.h"
#include "Delay.h"
#include "Buzzer.h"
#include "Key.h"
#include "LED.h"
#include "OLED.h"
#include "PWM.h"
#include "Servo.h"
#include "LightSensor.h"
#include "HCSR04.h"

/* 智能垃圾桶控制系统 */
 
// 系统状态定义
typedef enum {
    SYSTEM_OFF = 0,
    SYSTEM_ON = 1
} SystemState;

// 垃圾桶状态定义
typedef enum {
    BIN_CLOSED = 0,
    BIN_OPEN = 1
} BinState;

int main(void)
{
    /* 初始化所有硬件模块 */
    Buzzer_Init();
    LightSensor_Init();
    Key_Init();
    LED_Init();
    OLED_Init();
    PWM_Init();
    Servo_Init();
    HCSR04_Init();
    
    // 系统变量
    SystemState systemState = SYSTEM_OFF;
    BinState binState = BIN_CLOSED;
    uint8_t key;
    uint16_t distance;
    uint8_t autoCloseCounter = 0;
    uint8_t distanceStableCounter = 0;
    uint16_t lastDistance = 0;
    
    // 初始化显示界面
    OLED_ShowString(1, 1, "System:OFF");
    OLED_ShowString(2, 1, "Distance:   cm");
    OLED_ShowString(3, 1, "Bin:Closed");
    OLED_ShowString(4, 1, "Press K1:Start K2:Stop");
    
    // 蜂鸣器提示系统启动
    Buzzer_ON();
    Delay_ms(200);
    Buzzer_OFF();
    
    while (1)
    {
        // 检测按键输入
        key = Key_GetNum();
        
        // 按键1：启动系统
        if(key == 1 && systemState == SYSTEM_OFF) {
            systemState = SYSTEM_ON;
            OLED_ShowString(1, 8, "ON ");
            Buzzer_ON();
            Delay_ms(100);
            Buzzer_OFF();
        }
        
        // 按键2：停止系统
        if(key == 2 && systemState == SYSTEM_ON) {
            systemState = SYSTEM_OFF;
            binState = BIN_CLOSED;
            Servo_SetAngle(0);  // 确保垃圾桶关闭
            OLED_ShowString(1, 8, "OFF");
            OLED_ShowString(3, 5, "Closed");
            LED1_OFF();
            Buzzer_ON();
            Delay_ms(300);
            Buzzer_OFF();
        }
        
        // 系统运行状态处理
        if(systemState == SYSTEM_ON) {
            // 光敏传感器控制LED
            if (LightSensor_Get() == 1) {
                LED1_ON();  // 光线暗时开灯
            } else {
                LED1_OFF(); // 光线亮时关灯
            }
            
            // 获取距离并显示
            distance = HCSR04_GetValue();
            OLED_ShowNum(2, 11, distance, 3);
            
            // 距离检测防抖处理
            if(abs(distance - lastDistance) < 3) {
                distanceStableCounter++;
            } else {
                distanceStableCounter = 0;
            }
            lastDistance = distance;
            
            // 稳定检测后处理垃圾桶开关
            if(distanceStableCounter >= 3) {
                // 检测到物体靠近且垃圾桶关闭
                if(distance < 15 && binState == BIN_CLOSED) {
                    binState = BIN_OPEN;
                    Servo_SetAngle(90);  // 打开垃圾桶
                    OLED_ShowString(3, 5, "Open  ");
                    autoCloseCounter = 0;
                    Buzzer_ON();
                    Delay_ms(50);
                    Buzzer_OFF();
                }
                
                // 垃圾桶打开状态下的自动关闭逻辑
                if(binState == BIN_OPEN) {
                    autoCloseCounter++;
                    // 5秒后自动关闭（50 * 100ms = 5秒）
                    if(autoCloseCounter >= 50 || distance > 30) {
                        binState = BIN_CLOSED;
                        Servo_SetAngle(0);  // 关闭垃圾桶
                        OLED_ShowString(3, 5, "Closed");
                        autoCloseCounter = 0;
                    }
                }
            }
        }
        
        Delay_ms(100);  // 主循环延时
    }
}

