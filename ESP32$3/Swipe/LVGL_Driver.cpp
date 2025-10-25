/*****************************************************************************
  | File        :   LVGL_Driver.c
  | Help        :   LVGL integration driver for Waveshare ESP32-S3 Round LCD
******************************************************************************/
#include "LVGL_Driver.h"

lv_disp_drv_t disp_drv;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;

/* Debug print helper */
void Lvgl_print(const char * buf)
{
    // Uncomment for LVGL debug messages
    // Serial.printf("%s", buf);
}

/*  Display flushing (called by LVGL to draw a region)  */
void Lvgl_Display_LCD(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    if (panel_handle == NULL) {
        Serial.println("ERROR: panel_handle is NULL in Lvgl_Display_LCD");
        lv_disp_flush_ready(disp_drv);
        return;
    }

    esp_lcd_panel_draw_bitmap(panel_handle,
                              area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1,
                              (uint8_t *)color_p);

    lv_disp_flush_ready(disp_drv);
}

/* Read CST820 touch data for LVGL input */
void Lvgl_Touchpad_Read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    Touch_Read_Data();

    if (touch_data.points != 0x00) {
        data->point.x = touch_data.x;
        data->point.y = touch_data.y;
        data->state = LV_INDEV_STATE_PR;
        // printf("Touch: X=%u Y=%u Points=%d\n", touch_data.x, touch_data.y, touch_data.points);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }

    touch_data.x = 0;
    touch_data.y = 0;
    touch_data.points = 0;
    touch_data.gesture = NONE;
}

void example_increase_lvgl_tick(void *arg)
{
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

void Lvgl_Init(void)
{
    lv_init();

    if (panel_handle == NULL) {
        Serial.println("WARNING: panel_handle is NULL — did ST7701_Init() run?");
    }

    esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, (void**)&buf1, (void**)&buf2);

    if (buf1 == NULL) {
        Serial.println("ERROR: framebuffer buf1 is NULL");
    }

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LVGL_BUF_LEN);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.flush_cb = Lvgl_Display_LCD;
    disp_drv.full_refresh = 1;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = Lvgl_Touchpad_Read;
    lv_indev_drv_register(&indev_drv);

    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello Arduino + LVGL!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);
}

void Lvgl_Loop(void)
{
    lv_timer_handler();
}
