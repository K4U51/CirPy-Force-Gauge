#pragma once
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_rgb.h"

#include "TCA9554PWR.h"
#include "LVGL_Driver.h"
#include "Touch_CST820.h"

// ---------------- LCD PIN CONFIG ----------------
#define LCD_CLK_PIN        2
#define LCD_MOSI_PIN       1
#define LCD_Backlight_PIN  6

// ---------------- PWM / Backlight ----------------
#define PWM_Channel   1
#define Frequency     20000
#define Resolution    10
#define Backlight_MAX 100

// ---------------- LCD PANEL CONFIG ----------------
#define ESP_PANEL_LCD_WIDTH                480
#define ESP_PANEL_LCD_HEIGHT               480
#define ESP_PANEL_LCD_COLOR_BITS           16
#define ESP_PANEL_LCD_RGB_PIXEL_BITS       16
#define ESP_PANEL_LCD_RGB_DATA_WIDTH       16
#define ESP_PANEL_LCD_RGB_TIMING_FREQ_HZ   (16 * 1000 * 1000)
#define ESP_PANEL_LCD_RGB_TIMING_HPW       8
#define ESP_PANEL_LCD_RGB_TIMING_HBP       10
#define ESP_PANEL_LCD_RGB_TIMING_HFP       50
#define ESP_PANEL_LCD_RGB_TIMING_VPW       3
#define ESP_PANEL_LCD_RGB_TIMING_VBP       8
#define ESP_PANEL_LCD_RGB_TIMING_VFP       8
#define ESP_PANEL_LCD_RGB_FRAME_BUF_NUM    2
#define ESP_PANEL_LCD_RGB_BOUNCE_BUF_SIZE (ESP_PANEL_LCD_WIDTH * 10)

// ---------------- RGB GPIO ----------------
#define ESP_PANEL_LCD_PIN_NUM_RGB_HSYNC  38
#define ESP_PANEL_LCD_PIN_NUM_RGB_VSYNC  39
#define ESP_PANEL_LCD_PIN_NUM_RGB_DE     40
#define ESP_PANEL_LCD_PIN_NUM_RGB_PCLK   41
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA0  5
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA1  45
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA2  48
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA3  47
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA4  21
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA5  14
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA6  13
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA7  12
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA8  11
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA9  10
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA10 9
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA11 46
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA12 3
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA13 8
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA14 18
#define ESP_PANEL_LCD_PIN_NUM_RGB_DATA15 17
#define ESP_PANEL_LCD_PIN_NUM_RGB_DISP   (-1)

// ---------------- BACKLIGHT LEVEL ----------------
#define ESP_PANEL_LCD_BK_LIGHT_ON_LEVEL  1
#define ESP_PANEL_LCD_BK_LIGHT_OFF_LEVEL !ESP_PANEL_LCD_BK_LIGHT_ON_LEVEL

// ---------------- DEBUG ----------------
#define EXAMPLE_ENABLE_PRINT_LCD_FPS 0

// ---------------- GLOBALS ----------------
extern uint8_t LCD_Backlight;
extern esp_lcd_panel_handle_t panel_handle;

// ---------------- FUNCTIONS ----------------
bool example_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data);

void ST7701_Init();
void LCD_Init();
void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, lv_color_t* color);

void Backlight_Init();
void Set_Backlight(uint8_t Light);
