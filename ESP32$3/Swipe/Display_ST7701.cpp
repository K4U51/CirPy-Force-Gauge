/*****************************************************************************
  | File        :   Display_ST7701.cpp
  | Help        :   Driver for ST7701 LCD controller (Waveshare ESP32-S3)
******************************************************************************/
#include "Display_ST7701.h"

esp_lcd_panel_handle_t panel_handle = NULL;
spi_device_handle_t SPI_handle;

/* --- SPI helpers --- */
void ST7701_WriteCommand(uint8_t cmd)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    // gpio_set_level(LCD_DC_PIN, 0); // Command mode (optional)
    spi_device_transmit(SPI_handle, &t);
}

void ST7701_WriteData(uint8_t data)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &data;
    // gpio_set_level(LCD_DC_PIN, 1); // Data mode (optional)
    spi_device_transmit(SPI_handle, &t);
}

/* --- Initialization sequence --- */
void ST7701_Reset(void)
{
    pinMode(LCD_RST_PIN, OUTPUT);
    digitalWrite(LCD_RST_PIN, LOW);
    delay(120);
    digitalWrite(LCD_RST_PIN, HIGH);
    delay(120);
}

/* Configure the ST7701 and RGB panel driver */
void ST7701_Init(void)
{
    Serial.println("ST7701: Init start");

    /* RGB configuration */
    esp_lcd_rgb_panel_config_t rgb_config = {
        .data_width = 16,
        .psram_trans_align = 64,
        .num_fbs = 2,
        .bounce_buffer_size_px = 0,
        .clk_src = LCD_CLK_SRC_PLL160M,
        .disp_gpio_num = -1,
        .pclk_hz = 16000000,
        .hsync_pulse_width = 8,
        .hsync_back_porch = 8,
        .hsync_front_porch = 8,
        .vsync_pulse_width = 8,
        .vsync_back_porch = 8,
        .vsync_front_porch = 8,
        .flags = {
            .fb_in_psram = 1,
            .double_fb = 1,
        },
        .timings = {
            .pclk_hz = 16000000,
            .h_res = ESP_PANEL_LCD_WIDTH,
            .v_res = ESP_PANEL_LCD_HEIGHT,
            .hsync_pulse_width = 8,
            .hsync_back_porch = 8,
            .hsync_front_porch = 8,
            .vsync_pulse_width = 8,
            .vsync_back_porch = 8,
            .vsync_front_porch = 8,
            .flags = {
                .pclk_active_neg = false,
                .de_idle_high = false,
                .pclk_idle_high = false,
            },
        },
    };

    esp_err_t err = esp_lcd_new_rgb_panel(&rgb_config, &panel_handle);
    if (err != ESP_OK || panel_handle == NULL) {
        Serial.printf("ERROR: esp_lcd_new_rgb_panel failed: 0x%X\n", err);
        return;
    }

    esp_lcd_panel_reset(panel_handle);
    err = esp_lcd_panel_init(panel_handle);
    if (err != ESP_OK) {
        Serial.printf("ERROR: esp_lcd_panel_init failed: 0x%X\n", err);
    }

    Serial.println("ST7701: panel_handle created and initialized");
}

/* Add a window region for direct bitmap draw */
void LCD_addWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint8_t *data)
{
    if (panel_handle == NULL) {
        Serial.println("ERROR: LCD_addWindow() panel_handle NULL");
        return;
    }

    esp_lcd_panel_draw_bitmap(panel_handle, xs, ys, xe + 1, ye + 1, data);
}
