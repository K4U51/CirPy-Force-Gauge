// ---------- Swipe handler ----------
static lv_obj_t* currentScreen = nullptr;

static void swipeEventHandler(lv_event_t* e) {
    lv_indev_t* indev = lv_indev_get_act();
    lv_gesture_dir_t dir = lv_indev_get_gesture_dir(indev);

    if(dir == LV_GESTURE_LEFT) {
        if(currentScreen == ui_scrSplash) {
            lv_scr_load(ui_scrGForce);
            currentScreen = ui_scrGForce;
        } else if(currentScreen == ui_scrGForce) {
            lv_scr_load(ui_scrPeaks);
            currentScreen = ui_scrPeaks;
        }
    } else if(dir == LV_GESTURE_RIGHT) {
        if(currentScreen == ui_scrPeaks) {
            lv_scr_load(ui_scrGForce);
            currentScreen = ui_scrGForce;
        } else if(currentScreen == ui_scrGForce) {
            lv_scr_load(ui_scrSplash);
            currentScreen = ui_scrSplash;
        }
    }
}

// ---------- Main ----------
void app_main(void) {
    printf("🚀 Starting Minimal G-Force UI\n");

    // ---------- Hardware init ----------
    Wireless_Init();
    I2C_Init();
    PCF85063_Init();
    QMI8658_Init();
    LCD_Init();
    Touch_Init();
    SD_Init();
    LVGL_Init();

    // ---------- UI init ----------
    ui_init();

    // Set initial screen to splash
    lv_scr_load(ui_scrSplash);
    currentScreen = ui_scrSplash;

    // Attach swipe gesture callbacks (after UI objects are created)
    lv_obj_add_event_cb(ui_scrSplash, swipeEventHandler, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(ui_scrGForce, swipeEventHandler, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(ui_scrPeaks, swipeEventHandler, LV_EVENT_GESTURE, NULL);

    // Optional: auto-transition splash to GForce after 2 seconds
    vTaskDelay(pdMS_TO_TICKS(2000));
    lv_scr_load(ui_scrGForce);
    currentScreen = ui_scrGForce;

    // Initialize moving dot position
    if(ui_imgDot) lv_obj_set_pos(ui_imgDot, DIAL_CENTER_X, DIAL_CENTER_Y);

    // Attach Reset Peaks button
    if(ui_btnResetPeaks)
        lv_obj_add_event_cb(ui_btnResetPeaks, resetPeaksEventHandler, LV_EVENT_CLICKED, NULL);

    // ---------- Create log file ----------
    char filename[128];
    generateLogFilename(filename, sizeof(filename));
    logFile = fopen(filename, "w");
    if(logFile) {
        fprintf(logFile, "Timestamp,Ax,Ay,Az,PeakAccel,PeakBrake,PeakLeft,PeakRight\n");
        fflush(logFile);
        printf("✅ Logging to %s\n", filename);
    } else {
        printf("⚠️ Could not open SD log file: %s\n", filename);
    }

    // ---------- Main loop ----------
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(UPDATE_RATE_MS));
        lv_timer_handler();

        // Get and smooth accelerometer data
        getAccelerometerData(); // populates ax, ay, az
        smoothAccel(ax, ay, az);

        // Update peaks, dot position, labels, and log
        updatePeaks();
        updateDotImage();
        updateLabels();
        logData();
    }

    if(logFile) fclose(logFile);
}
