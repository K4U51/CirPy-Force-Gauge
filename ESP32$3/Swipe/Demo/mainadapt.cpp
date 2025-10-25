/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

#include "Wireless.h"
#include "Gyro_QMI8658.h"
#include "RTC_PCF85063.h"
#include "SD_Card.h"
#include "LVGL_Driver.h"
#include "BAT_Driver.h"

// ------------------ LVGL G-Force Objects ------------------
#include <lvgl.h>

lv_obj_t *gauge_dot;
const int PARTICLES = 20;
lv_obj_t *particle[PARTICLES];

lv_obj_t *label_accel;
lv_obj_t *label_brake;
lv_obj_t *label_left;
lv_obj_t *label_right;

lv_obj_t *bg_img;

// Particle positions
int particle_xpos[PARTICLES];
int particle_ypos[PARTICLES];

// Accelerometer values updated by driver task
float x = 0, y = 0, z = 0;

// ------------------ Driver Loop Task ------------------
void Driver_Loop(void *parameter)
{
    while(1)
    {
        QMI8658_Loop();  // Updates x, y, z internally
        RTC_Loop();
        BAT_Get_Volts();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ------------------ Driver Init ------------------
void Driver_Init()
{
    Flash_test();
    BAT_Init();
    I2C_Init();
    TCA9554PWR_Init(0x00);   
    Set_EXIO(EXIO_PIN8,Low);
    PCF85063_Init();
    QMI8658_Init(); 
    
    xTaskCreatePinnedToCore(
        Driver_Loop,     
        "Other Driver task",   
        4096,                
        NULL,                 
        3,                    
        NULL,                
        0                    
    );
}

// ------------------ LVGL G-Force Initialization ------------------
void Lvgl_GForce_Init()
{
    // Background gauge image
    bg_img = lv_img_create(lv_scr_act());
    // lv_img_set_src(bg_img, &my_gauge_img); // <-- PLACE YOUR 480x480 IMAGE HERE
    lv_obj_center(bg_img);

    // Particle trail
    for(int i=0; i<PARTICLES; i++){
        particle[i] = lv_obj_create(lv_scr_act());
        lv_obj_set_size(particle[i], 6, 6);
        lv_obj_set_style_radius(particle[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(particle[i], lv_color_yellow(), 0);
        lv_obj_set_pos(particle[i], 240, 240);
    }

    // Main G-force dot
    gauge_dot = lv_obj_create(lv_scr_act());
    lv_obj_set_size(gauge_dot, 10, 10);
    lv_obj_set_style_radius(gauge_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(gauge_dot, lv_color_white(), 0);
    lv_obj_center(gauge_dot);

    // Numeric labels
    label_accel = lv_label_create(lv_scr_act());
    lv_label_set_text(label_accel, "Accel: 0");
    lv_obj_align(label_accel, LV_ALIGN_TOP_LEFT, 10, 10);

    label_brake = lv_label_create(lv_scr_act());
    lv_label_set_text(label_brake, "Brake: 0");
    lv_obj_align(label_brake, LV_ALIGN_TOP_LEFT, 10, 30);

    label_left = lv_label_create(lv_scr_act());
    lv_label_set_text(label_left, "Left: 0");
    lv_obj_align(label_left, LV_ALIGN_TOP_LEFT, 10, 50);

    label_right = lv_label_create(lv_scr_act());
    lv_label_set_text(label_right, "Right: 0");
    lv_obj_align(label_right, LV_ALIGN_TOP_LEFT, 10, 70);
}

// ------------------ LVGL G-Force Loop ------------------
void Lvgl_GForce_Loop()
{
    // Map G-force to screen coordinates (center 240,240)
    float xpos = 240 + ((x / 9.81) * 150);
    float ypos = 240 + ((y / 9.81) * 150);

    // Particle trail
    particle_xpos[0] = (int)xpos;
    particle_ypos[0] = (int)ypos;

    for(int i=PARTICLES-1; i>=1; i--){
        particle_xpos[i] = particle_xpos[i-1];
        particle_ypos[i] = particle_ypos[i-1];
    }

    for(int i=0; i<PARTICLES; i++){
        lv_obj_set_pos(particle[i], particle_xpos[i], particle_ypos[i]);
        uint8_t alpha = map(i, 0, PARTICLES-1, 255, 50); // fade effect
        lv_color_t color = lv_color_make(alpha, alpha, 0);
        lv_obj_set_style_bg_color(particle[i], color, 0);
    }

    // Main dot
    lv_obj_set_pos(gauge_dot, (int)xpos, (int)ypos);

    // Numeric labels
    lv_label_set_text_fmt(label_accel, "Accel: %d", (int)max(y/9.81*10,0));
    lv_label_set_text_fmt(label_brake, "Brake: %d", (int)abs(min(y/9.81*10,0)));
    lv_label_set_text_fmt(label_left, "Left: %d", (int)abs(min(x/9.81*10,0)));
    lv_label_set_text_fmt(label_right, "Right: %d", (int)max(x/9.81*10,0));
}

// ------------------ Setup ------------------
void setup()
{
    Wireless_Test2();
    Driver_Init();
    LCD_Init();   // If LCD reinitialized, SD must follow
    SD_Init();    
    Lvgl_Init();

    Lvgl_GForce_Init();  // <-- Our custom LVGL G-Force screen
}

// ------------------ Main Loop ------------------
void loop()
{
    Lvgl_GForce_Loop();  // Update dot, trail, and numeric labels
    Lvgl_Loop();          // LVGL handler
    vTaskDelay(pdMS_TO_TICKS(5));
}
