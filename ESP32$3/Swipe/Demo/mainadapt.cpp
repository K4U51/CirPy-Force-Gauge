#include <Arduino.h>
#include "I2C_Driver.h"
#include "TCA9554PWR.h"
#include "Gyro_QMI8658.h"
#include "RTC_PCF85063.h"
#include "SD_Card.h"
#include "LVGL_Driver.h"
#include "BAT_Driver.h"
#include "Display_ST7701.h"
#include "Touch_CST820.h"
#include "ui.h"  // SquareLine generated UI

// ------------------ Global Variables ------------------
float x = 0, y = 0, z = 0;  // Gyro/accelerometer data

// ------------------ Driver Task ------------------
void Driver_Loop(void *parameter)
{
    while (1)
    {
        QMI8658_Loop();   // Updates x, y, z internally
        RTC_Loop();
        BAT_Get_Volts();

        // Store latest gyro values globally
        x = QMI8658_Get_Gx();
        y = QMI8658_Get_Gy();
        z = QMI8658_Get_Gz();

        vTaskDelay(pdMS_TO_TICKS(50)); // Update every 50ms
    }
}

// ------------------ Driver Init ------------------
void Driver_Init()
{
    Serial.println("Initializing Drivers...");

    I2C_Init();
    TCA9554PWR_Init(0x00);

    // Enable LCD Power and Backlight
    Set_EXIO(EXIO_PIN8, Low);
    delay(50);
    LCD_SetBackLight(100); // Full brightness

    PCF85063_Init();
    QMI8658_Init();
    BAT_Init();
    Flash_test();

    // Start driver task on Core 0
    xTaskCreatePinnedToCore(
        Driver_Loop,
        "Driver Loop",
        4096,
        NULL,
        3,
        NULL,
        0
    );
}

// ------------------ LVGL G-Force Update ------------------
void Lvgl_GForce_Loop()
{
    // Map X/Y acceleration to display coordinates (480x480)
    float xpos = 240 + ((x / 9.81f) * 150);
    float ypos = 240 + ((y / 9.81f) * 150);

    // Clamp to screen bounds
    xpos = constrain(xpos, 0, 479);
    ypos = constrain(ypos, 0, 479);

    // Move dot on screen based on gyro
    if(ui_dot != NULL) {
        lv_obj_set_pos(ui_dot, (int)xpos, (int)ypos);
    }

    // Update numeric labels - using your SquareLine object names
    if(ui_Accel != NULL) {
        lv_label_set_text_fmt(ui_Accel, "Accel: %.2f", max(y / 9.81f, 0.0f));
    }
    if(ui_Brake != NULL) {
        lv_label_set_text_fmt(ui_Brake, "Brake: %.2f", abs(min(y / 9.81f, 0.0f)));
    }
    if(ui_Left != NULL) {
        lv_label_set_text_fmt(ui_Left, "Left: %.2f", abs(min(x / 9.81f, 0.0f)));
    }
    if(ui_Right != NULL) {
        lv_label_set_text_fmt(ui_Right, "Right: %.2f", max(x / 9.81f, 0.0f));
    }
}

// ------------------ Setup ------------------
void setup()
{
    Serial.begin(115200);
    Serial.println("System Booting...");

    // Initialize drivers and sensors
    Driver_Init();

    // Waveshare display init
    LCD_Init();
    Touch_Init();     // Touch panel init
    Lvgl_Init();      // LVGL driver init

    // IMPORTANT: Remove the demo label that Lvgl_Init() creates
    lv_obj_clean(lv_scr_act());

    // Load SquareLine UI assets
    ui_init();               // Create screens and objects
    
    // Load main screen - adjust to your actual screen name from SquareLine
    lv_scr_load(ui_Screen1); // Change if your screen has a different name

    SD_Init();               // Initialize SD last
    Serial.println("Setup Complete.");
}

// ------------------ Main Loop ------------------
void loop()
{
    Lvgl_GForce_Loop();  // Update dot and numeric labels
    Lvgl_Loop();         // LVGL internal handler (calls lv_timer_handler)
    delay(5);            // Small delay for scheduler
}
