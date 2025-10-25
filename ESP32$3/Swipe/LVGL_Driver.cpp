/*****************************************************************************
  | File        :   LVGL_Driver.cpp
  | Help        :   Arduino-compatible LVGL display and touch driver
******************************************************************************/
#include <Arduino.h>
#include "LVGL_Driver.h"
#include "Display_ST7701.h"
#include "Touch_CST820.h"
#include "lvgl.h"

// --- LVGL draw buffer ---
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;
lv_disp_drv_t disp_drv;

#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

// --- LVGL flush callback (draws to screen) ---
void Lvgl_Display_LCD(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    LCD_addWindow(area->x1, area->y1, area->x2, area->y2, (uint8_t *)&color_p->full);
    lv_disp_flush_ready(disp_drv);
}

// --- Touch read callback ---
void Lvgl_Touchpad_Read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    Touch_Read_Data();
    if (touch_data.points != 0x00)
    {
        data->point.x = touch_data.x;
        data->point.y = touch_data.y;
        data->state = LV_INDEV_STATE_PR;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
    touch_data.x = 0;
    touch_data.y = 0;
    touch_data.points = 0;
    touch_data.gesture = NONE;
}

// --- LVGL tick handler ---
void example_increase_lvgl_tick(void *arg)
{
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

// --- Initialize LVGL + display + touch ---
void Lvgl_Init(void)
{
    lv_init();

    // Allocate draw buffers
    buf1 = (lv_color_t *)heap_caps_malloc(LCD_WIDTH * 40 * sizeof(lv_color_t), MALLOC_CAP_INTERNAL);
    buf2 = (lv_color_t *)heap_caps_malloc(LCD_WIDTH * 40 * sizeof(lv_color_t), MALLOC_CAP_INTERNAL);
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LCD_WIDTH * 40);

    // Initialize display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.flush_cb = Lvgl_Display_LCD;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Initialize touch driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = Lvgl_Touchpad_Read;
    lv_indev_drv_register(&indev_drv);

    // LVGL tick update
    static unsigned long lastTick = 0;

    // Simple test label
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello Arduino + LVGL!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

// --- LVGL loop ---
void Lvgl_Loop(void)
{
    lv_timer_handler();
    delay(5);
}
