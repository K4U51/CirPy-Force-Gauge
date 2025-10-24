#include <Arduino.h>
#include <TCA9554PWR.h>
#include <PCF85063.h>
#include <QMI8658.h>
#include <ST7701S.h>
#include <CST820.h>
#include <lvgl.h>
#include <Wireless.h>
#include <RTC_PCF85063.h>
#include <SD_Card.h>
#include <LVGL_Driver.h>
#include <BAT_Driver.h>
#include "ui.h"  // SquareLine Studio UI

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

static File logFile;
static lv_obj_t *currentScreen = NULL;

static float ax, ay, az;

// ---------- Linear interpolation ----------
static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

// ---------- Accelerometer ----------
void getAccelerometerData() {
    ax = QMI8658.readAccelX();
    ay = QMI8658.readAccelY();
    az = QMI8658.readAccelZ();
}

// ... keep smoothAccel(), updatePeaks(), updateDotImage(), updateLabels(), logData(), generateLogFilename(), resetPeaksEventHandler(), swipeEventHandler() unchanged ...

void setup() {
    Serial.begin(115200);
    Serial.println("🚀 Starting Minimal G-Force UI");

    // Hardware init
    Wireless_Init();
    I2C_Init();
    PCF85063_Init();
    QMI8658.begin();   // Waveshare init function
    LCD_Init();
    Touch_Init();
    SD_Init();
    LVGL_Init();

    // UI init
    ui_init();

    lv_scr_load(ui_scrSplash);
    currentScreen = ui_scrSplash;

    lv_obj_add_event_cb(ui_scrSplash, swipeEventHandler, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(ui_scrGForce, swipeEventHandler, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(ui_scrPeaks, swipeEventHandler, LV_EVENT_GESTURE, NULL);

    delay(2000);
    lv_scr_load(ui_scrGForce);
    currentScreen = ui_scrGForce;

    if(ui_imgDot) lv_obj_set_pos(ui_imgDot, DIAL_CENTER_X, DIAL_CENTER_Y);
    if(ui_btnResetPeaks)
        lv_obj_add_event_cb(ui_btnResetPeaks, resetPeaksEventHandler, LV_EVENT_CLICKED, NULL);

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

void loop() {
    delay(UPDATE_RATE_MS);
    lv_timer_handler();

    getAccelerometerData();
    smoothAccel(ax, ay, az);
    updatePeaks();
    updateDotImage();
    updateLabels();
    logData();
}
