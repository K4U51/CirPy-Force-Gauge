#include <Arduino.h>
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

// ---------- Config ----------
#define UPDATE_RATE_MS 50
#define SMOOTH_FACTOR 0.2f
#define G_MAX 2.5f
#define DIAL_CENTER_X 240
#define DIAL_CENTER_Y 240
#define DIAL_SCALE 90.0f

// ---------- Globals ----------
static float smoothed_ax = 0, smoothed_ay = 0, smoothed_az = 0;
static float peak_accel = 0, peak_brake = 0, peak_lat = 0;
static File logFile;
static lv_obj_t *currentScreen = NULL;

// ---------- Math ----------
static inline float lerp_val(float a, float b, float t) { return a + (b - a) * t; }
static void smoothAccel(float ax_in, float ay_in, float az_in) {
    smoothed_ax = smoothed_ax * (1.0f - SMOOTH_FACTOR) + ax_in * SMOOTH_FACTOR;
    smoothed_ay = smoothed_ay * (1.0f - SMOOTH_FACTOR) + ay_in * SMOOTH_FACTOR;
    smoothed_az = smoothed_az * (1.0f - SMOOTH_FACTOR) + az_in * SMOOTH_FACTOR;
}

// ---------- Peak Logic ----------
static void updatePeaks() {
    if (smoothed_ax > peak_accel) peak_accel = smoothed_ax;
    if (smoothed_ax < -peak_brake) peak_brake = -smoothed_ax;
    float lateral = fabs(smoothed_ay);
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

    last_x = (int16_t)lerp_val(last_x, target_x, SMOOTH_FACTOR);
    last_y = (int16_t)lerp_val(last_y, target_y, SMOOTH_FACTOR);

    if (ui_Dot) lv_obj_set_pos(ui_Dot, last_x, last_y);
}

// ---------- Label Updates ----------
static void updateLabels() {
    if (ui_Fwd) {
        lv_label_set_text_fmt(ui_Fwd, "%.2f", smoothed_ax > 0 ? smoothed_ax : 0.0f);
        lv_obj_set_style_text_font(ui_Fwd, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_Brake) {
        lv_label_set_text_fmt(ui_Brake, "%.2f", smoothed_ax < 0 ? -smoothed_ax : 0.0f);
        lv_obj_set_style_text_font(ui_Brake, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_Left) {
        lv_label_set_text_fmt(ui_Left, "%.2f", smoothed_ay < 0 ? -smoothed_ay : 0.0f);
        lv_obj_set_style_text_font(ui_Left, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_Right) {
        lv_label_set_text_fmt(ui_Right, "%.2f", smoothed_ay > 0 ? smoothed_ay : 0.0f);
        lv_obj_set_style_text_font(ui_Right, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (ui_PeakAccel) {
        lv_label_set_text_fmt(ui_PeakAccel, "Fwd: %.2f g", peak_accel);
        lv_obj_set_style_text_font(ui_PeakAccel, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_PeakBrake) {
        lv_label_set_text_fmt(ui_PeakBrake, "Brake: %.2f g", peak_brake);
        lv_obj_set_style_text_font(ui_PeakBrake, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_PeakLat) {
        lv_label_set_text_fmt(ui_PeakLat, "Lat: %.2f g", peak_lat);
        lv_obj_set_style_text_font(ui_PeakLat, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
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

// ---------- Reset Peaks ----------
static void resetPeaksEventHandler(lv_event_t * e) {
    peak_accel = peak_brake = peak_lat = 0;
    updateLabels();
}

// ---------- Setup ----------
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
    lv_scr_load(ui_Gforce);  // Start directly on G-Force screen
    currentScreen = ui_Gforce;

    lv_obj_add_event_cb(ui_Gforce, swipeEventHandler, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(ui_Peaks, swipeEventHandler, LV_EVENT_GESTURE, NULL);

    if (ui_Dot) lv_obj_set_pos(ui_Dot, DIAL_CENTER_X, DIAL_CENTER_Y);
    if (ui_ResetPeaks)
        lv_obj_add_event_cb(ui_ResetPeaks, resetPeaksEventHandler, LV_EVENT_CLICKED, NULL);

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

// ---------- Loop ----------
void loop() {
    float ax, ay, az;
    QMI8658_Read_XYZ(&ax, &ay, &az);
    smoothAccel(ax, ay, az);
    updatePeaks();
    updateDotImage();
    updateLabels();
    logData();

    lv_timer_handler();
    delay(UPDATE_RATE_MS);
}
