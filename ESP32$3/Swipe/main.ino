#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "Wireless.h"
#include "RTC_PCF85063.h"
#include "SD_Card.h"
#include "LVGL_Driver.h"
#include "ui.h"  // SquareLine UI objects

extern RTC_DateTypeDef datetime;

// ---------- Config ----------
#define UPDATE_RATE_MS 50
#define SMOOTH_FACTOR 0.2f
#define G_MAX 2.5f
#define DIAL_CENTER_X 240
#define DIAL_CENTER_Y 240
#define DIAL_SCALE 90.0f

// ---------- Globals ----------
static float smoothed_ax = 0;
static float smoothed_ay = 0;
static float smoothed_az = 0;
static float peak_accel = 0;
static float peak_brake = 0;
static float peak_left  = 0;
static float peak_right = 0;

static File logFile = File();
static lv_obj_t *currentScreen = NULL;

// For accelerometer read
static float ax, ay, az;

// ---------- Linear interpolation ----------
static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

// ---------- Accelerometer smoothing ----------
static void smoothAccel(float ax_in, float ay_in, float az_in) {
    smoothed_ax = smoothed_ax * (1.0f - SMOOTH_FACTOR) + ax_in * SMOOTH_FACTOR;
    smoothed_ay = smoothed_ay * (1.0f - SMOOTH_FACTOR) + ay_in * SMOOTH_FACTOR;
    smoothed_az = smoothed_az * (1.0f - SMOOTH_FACTOR) + az_in * SMOOTH_FACTOR;
}

// ---------- Peak calculation ----------
static void updatePeaks() {
    if (smoothed_ax > peak_accel) peak_accel = smoothed_ax;
    if (smoothed_ax < -peak_brake) peak_brake = -smoothed_ax;
    if (smoothed_ay < -peak_left)  peak_left  = -smoothed_ay;
    if (smoothed_ay > peak_right)  peak_right = smoothed_ay;
}

// ---------- Dot movement ----------
static void updateDotImage() {
    static int16_t last_x = DIAL_CENTER_X;
    static int16_t last_y = DIAL_CENTER_Y;

    float ax_clip = smoothed_ax;
    float ay_clip = smoothed_ay;
    float mag = sqrtf(ax_clip*ax_clip + ay_clip*ay_clip);
    if(mag > G_MAX) {
        ax_clip = (ax_clip / mag) * G_MAX;
        ay_clip = (ay_clip / mag) * G_MAX;
    }

    int16_t target_x = DIAL_CENTER_X + (int16_t)((ax_clip / G_MAX) * DIAL_SCALE);
    int16_t target_y = DIAL_CENTER_Y - (int16_t)((ay_clip / G_MAX) * DIAL_SCALE);

    last_x = (int16_t)lerp(last_x, target_x, SMOOTH_FACTOR);
    last_y = (int16_t)lerp(last_y, target_y, SMOOTH_FACTOR);

    if(ui_imgDot) lv_obj_set_pos(ui_imgDot, last_x, last_y);
}

// ---------- Label updates ----------
static void updateLabels() {
    if(ui_labelGx) lv_label_set_text_fmt(ui_labelGx, "X: %.2f g", smoothed_ax);
    if(ui_labelGy) lv_label_set_text_fmt(ui_labelGy, "Y: %.2f g", smoothed_ay);
    if(ui_labelGz) lv_label_set_text_fmt(ui_labelGz, "Z: %.2f g", smoothed_az);

    if(ui_labelFwd)   lv_label_set_text_fmt(ui_labelFwd, "%.2f", smoothed_ax > 0 ? smoothed_ax : 0.0f);
    if(ui_labelBrake) lv_label_set_text_fmt(ui_labelBrake, "%.2f", smoothed_ax < 0 ? -smoothed_ax : 0.0f);
    if(ui_labelLeft)  lv_label_set_text_fmt(ui_labelLeft, "%.2f", smoothed_ay < 0 ? -smoothed_ay : 0.0f);
    if(ui_labelRight) lv_label_set_text_fmt(ui_labelRight, "%.2f", smoothed_ay > 0 ? smoothed_ay : 0.0f);

    if(ui_labelPeakAccel) lv_label_set_text_fmt(ui_labelPeakAccel, "Fwd: %.2f g", peak_accel);
    if(ui_labelPeakBrake) lv_label_set_text_fmt(ui_labelPeakBrake, "Brake: %.2f g", peak_brake);
    if(ui_labelPeakLeft)  lv_label_set_text_fmt(ui_labelPeakLeft, "Left: %.2f g", peak_left);
    if(ui_labelPeakRight) lv_label_set_text_fmt(ui_labelPeakRight, "Right: %.2f g", peak_right);
}

// ---------- Logging ----------
static void logData() {
    if(!logFile) return;
    unsigned long micros_val = micros() % 1000000;
    logFile.printf("%04d-%02d-%02d %02d:%02d:%02d.%06lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
            datetime.year, datetime.month, datetime.day,
            datetime.hour, datetime.minute, datetime.second,
            micros_val,
            smoothed_ax, smoothed_ay, smoothed_az,
            peak_accel, peak_brake, peak_left, peak_right);
    logFile.flush();
}

// ---------- Generate unique log filename ----------
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
        if(logFile) { logFile.close(); attempt++; if(attempt>999) break; }
        else break;
    } while(1);
}

// ---------- Reset Peaks ----------
static void resetPeaksEventHandler(lv_event_t * e) {
    peak_accel = peak_brake = peak_left = peak_right = 0;
    updateLabels();
}

// ---------- Swipe Gesture ----------
static void swipeEventHandler(lv_event_t * e) {
    lv_gesture_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if(dir == LV_GESTURE_DIR_LEFT) {
        if(currentScreen == ui_scrSplash) { lv_scr_load(ui_scrGForce); currentScreen = ui_scrGForce; }
        else if(currentScreen == ui_scrGForce) { lv_scr_load(ui_scrPeaks); currentScreen = ui_scrPeaks; }
    } else if(dir == LV_GESTURE_DIR_RIGHT) {
        if(currentScreen == ui_scrPeaks) { lv_scr_load(ui_scrGForce); currentScreen = ui_scrGForce; }
        else if(currentScreen == ui_scrGForce) { lv_scr_load(ui_scrSplash); currentScreen = ui_scrSplash; }
    }
}

// ---------- Arduino setup() ----------
void setup() {
    Serial.begin(115200);
    Serial.println("🚀 Starting Minimal G-Force UI");

    // Hardware init
    Wireless_Init();
    I2C_Init();
    PCF85063_Init();
    QMI8658_Init();
    LCD_Init();
    Touch_Init();
    SD_Init();
    LVGL_Init();

    // UI init
    ui_init();

    // Set initial screen
    lv_scr_load(ui_scrSplash);
    currentScreen = ui_scrSplash;

    // Attach swipe gestures
    lv_obj_add_event_cb(ui_scrSplash, swipeEventHandler, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(ui_scrGForce, swipeEventHandler, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(ui_scrPeaks, swipeEventHandler, LV_EVENT_GESTURE, NULL);

    // Optional: auto-transition splash -> GForce
    delay(2000);
    lv_scr_load(ui_scrGForce);
    currentScreen = ui_scrGForce;

    // Initialize moving dot
    if(ui_imgDot) lv_obj_set_pos(ui_imgDot, DIAL_CENTER_X, DIAL_CENTER_Y);

    // Attach reset peaks button
    if(ui_btnResetPeaks)
        lv_obj_add_event_cb(ui_btnResetPeaks, resetPeaksEventHandler, LV_EVENT_CLICKED, NULL);

    // Create log file
    char filename[128];
    generateLogFilename(filename, sizeof(filename));
    logFile = SD.open(filename, FILE_WRITE);
    if(logFile) {
        logFile.printf("Timestamp,Ax,Ay,Az,PeakAccel,PeakBrake,PeakLeft,PeakRight\n");
        logFile.flush();
        Serial.printf("✅ Logging to %s\n", filename);
    } else {
        Serial.printf("⚠️ Could not open SD log file: %s\n", filename);
    }
}

// ---------- Arduino loop() ----------
void loop() {
    delay(UPDATE_RATE_MS);
    lv_timer_handler();

    getAccelerometerData();  // fills ax, ay, az
    smoothAccel(ax, ay, az);
    updatePeaks();
    updateDotImage();
    updateLabels();
    logData();
}
