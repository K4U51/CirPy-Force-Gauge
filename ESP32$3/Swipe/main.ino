#include <stdio.h>
#include <math.h>
#include "TCA9554PWR.h"
#include "Gyro_QMI8658.h"
#include "Display_ST7701.h"
#include "Touch_CST820.h"
#include "I2C_Driver.h"
#include "lvgl.h"
#include "Wireless.h"
#include "RTC_PCF85063.h"
#include "SD_Card.h"
#include "LVGL_Driver.h"
#include "BAT_Driver.h"
#include "ui.h"  // SquareLine UI

extern RTC_DateTypeDef datetime;

// ---------- Config ----------
#define UPDATE_RATE_MS 50
#define SMOOTH_FACTOR 0.2f
#define G_MAX 2.5f
#define DIAL_CENTER_X 240
#define DIAL_CENTER_Y 240
#define DIAL_SCALE 90.0f

// ---------- Globals ----------
static float smoothed_ax = 0, smoothed_ay = 0, smoothed_az = 0;
static float peak_accel = 0, peak_brake = 0, peak_lat = 0;  // single lateral peak
static float ax, ay, az;  // raw sensor values
static File logFile = File();
static lv_obj_t *currentScreen = NULL;

// ---------- Math ----------
static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
static void smoothAccel(float ax_in, float ay_in, float az_in) {
    smoothed_ax = smoothed_ax * (1.0f - SMOOTH_FACTOR) + ax_in * SMOOTH_FACTOR;
    smoothed_ay = smoothed_ay * (1.0f - SMOOTH_FACTOR) + ay_in * SMOOTH_FACTOR;
    smoothed_az = smoothed_az * (1.0f - SMOOTH_FACTOR) + az_in * SMOOTH_FACTOR;
}

// ---------- Peak Logic ----------
static void updatePeaks() {
    if (smoothed_ax > peak_accel) peak_accel = smoothed_ax;
    if (smoothed_ax < -peak_brake) peak_brake = -smoothed_ax;

    // lateral peak (largest of left/right)
    float lateral = fabsf(smoothed_ay);
    if (lateral > peak_lat) peak_lat = lateral;
}

// ---------- Dot Movement ----------
static void updateDotImage() {
    static int16_t last_x = DIAL_CENTER_X;
    static int16_t last_y = DIAL_CENTER_Y;

    float ax_clip = smoothed_ax;
    float ay_clip = smoothed_ay;
    float mag = sqrtf(ax_clip * ax_clip + ay_clip * ay_clip);
    if (mag > G_MAX) {
        ax_clip = (ax_clip / mag) * G_MAX;
        ay_clip = (ay_clip / mag) * G_MAX;
    }

    int16_t target_x = DIAL_CENTER_X + (int16_t)((ax_clip / G_MAX) * DIAL_SCALE);
    int16_t target_y = DIAL_CENTER_Y - (int16_t)((ay_clip / G_MAX) * DIAL_SCALE);

    last_x = (int16_t)lerp(last_x, target_x, SMOOTH_FACTOR);
    last_y = (int16_t)lerp(last_y, target_y, SMOOTH_FACTOR);

    if (ui_imgDot) lv_obj_set_pos(ui_imgDot, last_x, last_y);
}

// ---------- Label Updates ----------
static void updateLabels() {
    // Main directional labels
    if (ui_labelFwd)   lv_label_set_text_fmt(ui_labelFwd,   "%.2f", smoothed_ax > 0 ? smoothed_ax : 0.0f);
    if (ui_labelBrake) lv_label_set_text_fmt(ui_labelBrake, "%.2f", smoothed_ax < 0 ? -smoothed_ax : 0.0f);
    if (ui_labelLeft)  lv_label_set_text_fmt(ui_labelLeft,  "%.2f", smoothed_ay < 0 ? -smoothed_ay : 0.0f);
    if (ui_labelRight) lv_label_set_text_fmt(ui_labelRight, "%.2f", smoothed_ay > 0 ? smoothed_ay : 0.0f);

    // Peak labels
    if (ui_labelPeakAccel) lv_label_set_text_fmt(ui_labelPeakAccel, "Fwd: %.2f g", peak_accel);
    if (ui_labelPeakBrake) lv_label_set_text_fmt(ui_labelPeakBrake, "Brake: %.2f g", peak_brake);
    if (ui_labelPeakLat)   lv_label_set_text_fmt(ui_labelPeakLat,   "Lat: %.2f g", peak_lat);
}

// ---------- Logging ----------
static void logData() {
    if (!logFile) return;
    unsigned long micros_val = micros() % 1000000;
    logFile.printf("%04d-%02d-%02d %02d:%02d:%02d.%06lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
                   datetime.year, datetime.month, datetime.day,
                   datetime.hour, datetime.minute, datetime.second,
                   micros_val,
                   smoothed_ax, smoothed_ay, smoothed_az,
                   peak_accel, peak_brake, peak_lat);
    logFile.flush();
}

// ---------- Helpers ----------
static void generateLogFilename(char *filename, int max_len) {
    PCF85063_Read_Time(&datetime);
    int attempt = 0;
    do {
        unsigned long micros_val = micros() % 1000000;
        snprintf(filename, max_len,
                 "/sdcard/gforce_%04d%02d%02d_%02d%02d%02d_%06lu_%d.csv",
                 datetime.year, datetime.month, datetime.day,
                 datetime.hour, datetime.minute, datetime.second,
                 micros_val, attempt);
        logFile = SD.open(filename, FILE_READ);
        if (logFile) { logFile.close(); attempt++; if (attempt > 999) break; }
        else break;
    } while (1);
}

static void resetPeaksEventHandler(lv_event_t * e) {
    peak_accel = peak_brake = peak_lat = 0;
    updateLabels();
}

// ---------- Swipe ----------
static void swipeEventHandler(lv_event_t * e) {
    lv_gesture_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_GESTURE_DIR_LEFT) {
        if (currentScreen == ui_scrSplash) { lv_scr_load(ui_scrGForce); currentScreen = ui_scrGForce; }
        else if (currentScreen == ui_scrGForce) { lv_scr_load(ui_scrPeaks); currentScreen = ui_scrPeaks; }
    } else if (dir == LV_GESTURE_DIR_RIGHT) {
        if (currentScreen == ui_scrPeaks) { lv_scr_load(ui_scrGForce); currentScreen = ui_scrGForce; }
        else if (currentScreen == ui_scrGForce) { lv_scr_load(ui_scrSplash); currentScreen = ui_scrSplash; }
    }
}

// ---------- setup ----------
void setup() {
    Serial.begin(115200);
    Serial.println("🚀 Starting Minimal G-Force UI");

    Wireless_Init();
    I2C_Init();
    PCF85063_Init();
    QMI8658_Init();
    LCD_Init();
    Touch_Init();
    SD_Init();
    LVGL_Init();

    ui_init();
    lv_scr_load(ui_scrSplash);
    currentScreen = ui_scrSplash;

    lv_obj_add_event_cb(ui_scrSplash, swipeEventHandler, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(ui_scrGForce, swipeEventHandler, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(ui_scrPeaks, swipeEventHandler, LV_EVENT_GESTURE, NULL);

    delay(2000);
    lv_scr_load(ui_scrGForce);
    currentScreen = ui_scrGForce;

    if (ui_imgDot) lv_obj_set_pos(ui_imgDot, DIAL_CENTER_X, DIAL_CENTER_Y);
    if (ui_btnResetPeaks) lv_obj_add_event_cb(ui_btnResetPeaks, resetPeaksEventHandler, LV_EVENT_CLICKED, NULL);

    char filename[128];
    generateLogFilename(filename, sizeof(filename));
    logFile = SD.open(filename, FILE_WRITE);
    if (logFile) {
        logFile.printf("Timestamp,Ax,Ay,Az,PeakAccel,PeakBrake,PeakLat\n");
        logFile.flush();
        Serial.printf("✅ Logging to %s\n", filename);
    } else {
        Serial.printf("⚠️ Could not open log file\n");
    }
}

// ---------- loop ----------
void loop() {
    QMI8658_Read_XYZ(&ax, &ay, &az);
    smoothAccel(ax, ay, az);
    updatePeaks();
    updateDotImage();
    updateLabels();
    logData();

    lv_timer_handler();
    delay(UPDATE_RATE_MS);
}
