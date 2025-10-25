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
#include "ui.h"  // <-- SquareLine generated UI

// ------------------ Global Variables ------------------
float x = 0, y = 0, z = 0;  // Accelerometer data

// ------------------ Driver Task ------------------
void Driver_Loop(void *parameter)
{
    while (1)
    {
        QMI8658_Loop();   // Updates x, y, z internally
        RTC_Loop();
        BAT_Get_Volts();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ------------------ Driver Init ------------------
void Driver_Init()
{
    Serial.println("Initializing Drivers...");

    I2C_Init();
    TCA9554PWR_Init(0x00);

    // Enable LCD Power and Backlight
    Set_EXIO(EXIO_PIN8, Low);   // Power Enable pin
    delay(50);
    LCD_SetBackLight(100);      // Full brightness

    PCF85063_Init();
    QMI8658_Init();
    BAT_Init();
    Flash_test();

    // Run sensor tasks on Core 0
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

// ------------------ LVGL G-Force UI Update ------------------
void Lvgl_GForce_Loop()
{
    // Map G-force to screen coordinates
    float xpos = 240 + ((x / 9.81) * 150);
    float ypos = 240 + ((y / 9.81) * 150);

    // Main G-force indicator
    lv_obj_set_pos(dot, (int)xpos, (int)ypos);

    // Numeric labels
    lv_label_set_text_fmt(ui_labelAccel, "Accel: %d", (int)max(y / 9.81 * 10, 0));
    lv_label_set_text_fmt(ui_labelBrake, "Brake: %d", (int)abs(min(y / 9.81 * 10, 0)));
    lv_label_set_text_fmt(ui_labelLeft,  "Left: %d", (int)abs(min(x / 9.81 * 10, 0)));
    lv_label_set_text_fmt(ui_labelRight, "Right: %d", (int)max(x / 9.81 * 10, 0));
}

// ------------------ Setup ------------------
void setup()
{
    Serial.begin(115200);
    Serial.println("System Booting...");

    I2C_Init();
    Driver_Init();

    // LCD and LVGL Setup
    LCD_Init();      // Initialize ST7701 panel
    Touch_Init();    // Initialize CST820 touch
    Lvgl_Init();     // Initialize LVGL driver

    ui_init();       // Load SquareLine project
    LCD_SetBackLight(100);

    SD_Init();       // Initialize SD last
    Serial.println("Setup Complete.");
}

// ------------------ Main Loop ------------------
void loop()
{
    Lvgl_GForce_Loop();  // Update G-force UI
    Lvgl_Loop();         // LVGL internal handler
    delay(5);
}
