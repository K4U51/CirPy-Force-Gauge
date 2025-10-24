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
#define UPDATE_RATE_MS 5      // fast updates for smooth UI
#define SMOOTH_SAMPLES 5      // moving average window
#define G_MAX 2.5f
#define DIAL_CENTER_X 240
#define DIAL_CENTER_Y 240
#define DIAL_SCALE 90.0f
#define LOG_BUFFER_SIZE 512   // bytes per SD buffer

// ---------- Globals ----------
static float ax_buf[SMOOTH_SAMPLES] = {0}, ay_buf[SMOOTH_SAMPLES] = {0}, az_buf[SMOOTH_SAMPLES] = {0};
static uint8_t buf_index = 0;
static float smoothed_ax = 0, smoothed_ay = 0, smoothed_az = 0;
static float peak_accel = 0, peak_brake = 0, peak_lat = 0;
static File logFile;
static lv_obj_t *currentScreen = NULL;

// SD logging buffer
static char logBuffer[LOG_BUFFER_SIZE];
static size_t logBufferIndex = 0;

// ---------- Math ----------
static inline float lerp_val(float a, float b, float t) { return a + (b - a) * t; }

// ---------- Optimized Smoothing + Peaks ----------
static void smoothAccel(float ax, float ay, float az) {
    ax_buf[buf_index] = ax;
    ay_buf[buf_index] = ay;
    az_buf[buf_index] = az;
    buf_index = (buf_index + 1) % SMOOTH_SAMPLES;

    float sum_ax = 0, sum_ay = 0, sum_az = 0;
    for (uint8_t i = 0; i < SMOOTH_SAMPLES; i++) {
        sum_ax += ax_buf[i];
        sum_ay += ay_buf[i];
        sum_az += az_buf[i];
    }
    smoothed_ax = sum_ax / SMOOTH_SAMPLES;
    smoothed_ay = sum_ay / SMOOTH_SAMPLES;
    smoothed_az = sum_az / SMOOTH_SAMPLES;

    // Update peaks
    if (smoothed_ax > peak_accel) peak_accel = smoothed_ax;
    if (smoothed_ax < -peak_brake) peak_brake = -smoothed_ax;
    float lateral = fabs(smoothed_ay);
    if (lateral > peak_lat) peak_lat = lateral;
}

// ---------- Dot Movement ----------
static void updateDotImage() {
    if (!ui_Dot || currentScreen != ui_Gforce) return;

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

    last_x = (int16_t)lerp_val(last_x, target_x, 0.2f);
    last_y = (int16_t)lerp_val(last_y, target_y, 0.2f);

    lv_obj_set_pos(ui_Dot, last_x, last_y);
}

// ---------- Label Updates ----------
static void updateLabels() {
    if(currentScreen != ui_Gforce) return;

    if (ui_Fwd) lv_label_set_text_fmt(ui_Fwd, "%.2f", smoothed_ax > 0 ? smoothed_ax : 0.0f);
    if (ui_Brake) lv_label_set_text_fmt(ui_Brake, "%.2f", smoothed_ax < 0 ? -smoothed_ax : 0.0f);
    if (ui_Left) lv_label_set_text_fmt(ui_Left, "%.2f", smoothed_ay < 0 ? -smoothed_ay : 0.0f);
    if (ui_Right) lv_label_set_text_fmt(ui_Right, "%.2f", smoothed_ay > 0 ? smoothed_ay : 0.0f);

    if (ui_PeakAccel) lv_label_set_text_fmt(ui_PeakAccel, "Fwd: %.2f g", peak_accel);
    if (ui_PeakBrake) lv_label_set_text_fmt(ui_PeakBrake, "Brake: %.2f g", peak_brake);
    if (ui_PeakLat) lv_label_set_text_fmt(ui_PeakLat, "Lat: %.2f g", peak_lat);
}

// ---------- Non-blocking Logging ----------
static void logDataBuffered() {
    if (!logFile) return;

    char line[128];
    unsigned long micros_val = micros() % 1000000;
    int len = snprintf(line, sizeof(line),
        "%04d-%02d-%02d %02d:%02d:%02d.%06lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
        datetime.year, datetime.month, datetime.day,
        datetime.hour, datetime.minute, datetime.second,
        micros_val,
        smoothed_ax, smoothed_ay, smoothed_az,
        peak_accel, peak_brake, peak_lat);

    if (logBufferIndex + len < LOG_BUFFER_SIZE) {
        memcpy(logBuffer + logBufferIndex, line, len);
        logBufferIndex += len;
    } else {
        logFile.write((const uint8_t*)logBuffer, logBufferIndex);
        logFile.flush();
        logBufferIndex = 0;

        if(len < LOG_BUFFER_SIZE) {
            memcpy(logBuffer, line, len);
            logBufferIndex = len;
        }
    }
}

static void flushLogBuffer() {
    if (!logFile || logBufferIndex == 0) return;
    logFile.write((const uint8_t*)logBuffer, logBufferIndex);
    logFile.flush();
    logBufferIndex = 0;
}

// ---------- Reset Peaks ----------
static void resetPeaksEventHandler(lv_event_t * e) {
    peak_accel = peak_brake = peak_lat = 0;
    updateLabels();
}

// ---------- Swipe Handler ----------
static void swipeEventHandler(lv_event_t * e) {
    lv_indev_t *indev = lv_indev_get_next(NULL);
    if (!indev) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);

    if(dir == LV_DIR_LEFT) {
        lv_scr_load(ui_Peaks);
        currentScreen = ui_Peaks;
    } else if(dir == LV_DIR_RIGHT) {
        lv_scr_load(ui_Gforce);
        currentScreen = ui_Gforce;
    }
}

// ---------- ST7701 Display Init Helper ----------
void ST7701_Init_Display() {
    tft.init(240, 240);
    tft.setRotation(0);
    tft.setSPISpeed(40000000);
    tft.fillScreen(ST77XX_BLACK);
}

// ---------- LVGL Task ----------
void lvglTask(void *pvParameters) {
    (void) pvParameters;
    for(;;) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(UPDATE_RATE_MS));
    }
}

// ---------- Setup ----------
void setup() {
    Serial.begin(115200);
    Serial.println("🚀 Starting G-Force UI (Buffered Logging)");

    Wireless_Init();
    I2C_Init();
    PCF85063_Init();
    QMI8658_Init();
    LCD_Init();
    ST7701_Init_Display();
    Touch_Init();
    SD_Init();
    LVGL_Init();

    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = Touch_Read;
    lv_indev_drv_register(&indev_drv);

    ui_init();

    if(!ui_Dot) {
        ui_Dot = lv_obj_create(ui_Gforce);
        lv_obj_set_size(ui_Dot, 12, 12);
        lv_obj_set_style_bg_color(ui_Dot, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(ui_Dot, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(ui_Dot, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_pos(ui_Dot, DIAL_CENTER_X, DIAL_CENTER_Y);
    }

    lv_obj_add_event_cb(ui_Screen1, swipeEventHandler, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(ui_Gforce, swipeEventHandler, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(ui_Peaks, swipeEventHandler, LV_EVENT_GESTURE, NULL);

    if (ui_ResetPeaks)
        lv_obj_add_event_cb(ui_ResetPeaks, resetPeaksEventHandler, LV_EVENT_CLICKED, NULL);

    lv_scr_load(ui_Screen1);
    currentScreen = ui_Screen1;

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

    xTaskCreatePinnedToCore(lvglTask, "LVGL_Task", 4096, NULL, 1, NULL, 1);
}

// ---------- Loop ----------
void loop() {
    float ax, ay, az;
    QMI8658_Read_XYZ(&ax, &ay, &az);

    smoothAccel(ax, ay, az);    // smoothing + peak update
    updateDotImage();            // move UI dot
    updateLabels();              // update labels
    logDataBuffered();           // log buffered
}
