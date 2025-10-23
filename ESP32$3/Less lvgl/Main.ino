#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "Wireless.h"
#include "RTC_PCF85063.h"
#include "SD_Card.h"
#include "LVGL_Driver.h"
#include "ui.h"  // Only SquareLine objects

extern RTC_DateTypeDef datetime;

#define UPDATE_RATE_MS 50
#define SMOOTH_FACTOR 0.2f
#define G_MAX 2.5f
#define DIAL_CENTER_X 240
#define DIAL_CENTER_Y 240
#define DIAL_SCALE 90.0f

static float smoothed_ax = 0;
static float smoothed_ay = 0;
static float smoothed_az = 0;
static float peak_accel = 0;
static float peak_brake = 0;
static float peak_left  = 0;
static float peak_right = 0;

static FILE *logFile = NULL;

static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

static void smoothAccel(float ax, float ay, float az) {
    smoothed_ax = smoothed_ax * (1.0f - SMOOTH_FACTOR) + ax * SMOOTH_FACTOR;
    smoothed_ay = smoothed_ay * (1.0f - SMOOTH_FACTOR) + ay * SMOOTH_FACTOR;
    smoothed_az = smoothed_az * (1.0f - SMOOTH_FACTOR) + az * SMOOTH_FACTOR;
}

static void updatePeaks() {
    if (smoothed_ax > peak_accel) peak_accel = smoothed_ax;
    if (smoothed_ax < -peak_brake) peak_brake = -smoothed_ax;
    if (smoothed_ay < -peak_left)  peak_left  = -smoothed_ay;
    if (smoothed_ay > peak_right)  peak_right = smoothed_ay;
}

static void updateDotImage() {
    static int16_t last_x = DIAL_CENTER_X;
    static int16_t last_y = DIAL_CENTER_Y;

    float ax = smoothed_ax;
    float ay = smoothed_ay;
    float mag = sqrtf(ax*ax + ay*ay);

    if (mag > G_MAX) {
        ax = (ax / mag) * G_MAX;
        ay = (ay / mag) * G_MAX;
    }

    int16_t target_x = DIAL_CENTER_X + (int16_t)((ax / G_MAX) * DIAL_SCALE);
    int16_t target_y = DIAL_CENTER_Y - (int16_t)((ay / G_MAX) * DIAL_SCALE);

    last_x = (int16_t)lerp(last_x, target_x, SMOOTH_FACTOR);
    last_y = (int16_t)lerp(last_y, target_y, SMOOTH_FACTOR);

    if (ui_img_dot) lv_obj_set_pos(ui_img_dot, last_x, last_y);
}

static void logData() {
    if (!logFile) return;
    uint64_t micros = esp_timer_get_time() % 1000000;
    fprintf(logFile, "%04d-%02d-%02d %02d:%02d:%02d.%06llu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
            datetime.year, datetime.month, datetime.day,
            datetime.hour, datetime.minute, datetime.second,
            micros,
            smoothed_ax, smoothed_ay, smoothed_az,
            peak_accel, peak_brake, peak_left, peak_right);
    fflush(logFile);
}

static void generateLogFilename(char *filename, int max_len) {
    PCF85063_Read_Time(&datetime);
    int attempt = 0;
    do {
        uint64_t micros = esp_timer_get_time() % 1000000;
        snprintf(filename, max_len,
                 "/sdcard/gforce_%04d%02d%02d_%02d%02d%02d_%06llu_%d.csv",
                 datetime.year, datetime.month, datetime.day,
                 datetime.hour, datetime.minute, datetime.second,
                 micros, attempt);
        FILE *f = fopen(filename, "r");
        if (f) { fclose(f); attempt++; if (attempt > 999) break; }
        else break;
    } while(1);
}

static void resetPeaksEventHandler(lv_event_t * e) {
    peak_accel = peak_brake = peak_left = peak_right = 0;
}

void app_main(void) {
    printf("🚀 Starting Ultra-Minimal G-Force UI\n");

    Wireless_Init();
    I2C_Init();
    PCF85063_Init();
    QMI8658_Init();
    LCD_Init();
    Touch_Init();
    SD_Init();
    LVGL_Init();

    ui_init();

    if (ui_img_dot) lv_obj_set_pos(ui_img_dot, DIAL_CENTER_X, DIAL_CENTER_Y);

    if (ui_btn_reset_peaks)
        lv_obj_add_event_cb(ui_btn_reset_peaks, resetPeaksEventHandler, LV_EVENT_CLICKED, NULL);

    char filename[128];
    generateLogFilename(filename, sizeof(filename));
    logFile = fopen(filename, "w");
    if (logFile) fprintf(logFile, "Timestamp,Ax,Ay,Az,PeakAccel,PeakBrake,PeakLeft,PeakRight\n");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(UPDATE_RATE_MS));
        lv_timer_handler();

        getAccelerometerData();
        smoothAccel(ax, ay, az);
        updatePeaks();
        updateDotImage();
        logData();
    }

    if (logFile) fclose(logFile);
}
