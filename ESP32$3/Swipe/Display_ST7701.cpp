#include "Display_ST7701.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"

// ---------------- GLOBALS ----------------
spi_device_handle_t SPI_handle = NULL;
esp_lcd_panel_handle_t panel_handle = NULL;
uint8_t LCD_Backlight = 50;

// ---------------- GPIO HELPERS ----------------
void ST7701_CS_EN() {
    Set_EXIO(EXIO_PIN3, Low);
    vTaskDelay(pdMS_TO_TICKS(10));
}

void ST7701_CS_Dis() {
    Set_EXIO(EXIO_PIN3, High);
    vTaskDelay(pdMS_TO_TICKS(10));
}

void ST7701_Reset() {
    Set_EXIO(EXIO_PIN1, Low);
    vTaskDelay(pdMS_TO_TICKS(10));
    Set_EXIO(EXIO_PIN1, High);
    vTaskDelay(pdMS_TO_TICKS(50));
}

// ---------------- SPI COMMANDS ----------------
void ST7701_WriteCommand(uint8_t cmd) {
    spi_transaction_t spi_tran = {
        .cmd = 0,
        .addr = cmd,
        .length = 0,
        .rxlength = 0,
    };
    spi_device_transmit(SPI_handle, &spi_tran);
}

void ST7701_WriteData(uint8_t data) {
    spi_transaction_t spi_tran = {
        .cmd = 1,
        .addr = data,
        .length = 0,
        .rxlength = 0,
    };
    spi_device_transmit(SPI_handle, &spi_tran);
}

// ---------------- BACKLIGHT ----------------
void Backlight_Init() {
    ledcAttach(LCD_Backlight_PIN, Frequency, Resolution);
    Set_Backlight(LCD_Backlight);
}

void Set_Backlight(uint8_t Light) {
    if (Light > 100) Light = 100;
    uint32_t duty = Light * 10;
    if (duty == 1000) duty = 1024;
    ledcWrite(LCD_Backlight_PIN, duty);
}

// ---------------- LCD INIT ----------------
void ST7701_Init() {
    // SPI bus init
    spi_bus_config_t buscfg = {
        .mosi_io_num = LCD_MOSI_PIN,
        .miso_io_num = -1,
        .sclk_io_num = LCD_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64 * 1024, // 64K max transfer
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {
        .command_bits = 1,
        .address_bits = 8,
        .mode = SPI_MODE0,
        .clock_speed_hz = 40000000,
        .spics_io_num = -1,
        .queue_size = 1,
    };
    spi_bus_add_device(SPI2_HOST, &devcfg, &SPI_handle);

    // Reset and configure ST7701
    ST7701_CS_EN();
    ST7701_Reset();

    // --- Minimal init commands, expand if needed ---
    ST7701_WriteCommand(0x11);  // Sleep out
    vTaskDelay(pdMS_TO_TICKS(480));
    ST7701_WriteCommand(0x29);  // Display on
    ST7701_CS_Dis();

    // RGB panel config for LVGL
    esp_lcd_rgb_panel_config_t rgb_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = ESP_PANEL_LCD_RGB_TIMING_FREQ_HZ,
            .h_res = ESP_PANEL_LCD_HEIGHT,
            .v_res = ESP_PANEL_LCD_WIDTH,
            .hsync_pulse_width = ESP_PANEL_LCD_RGB_TIMING_HPW,
            .hsync_back_porch = ESP_PANEL_LCD_RGB_TIMING_HBP,
            .hsync_front_porch = ESP_PANEL_LCD_RGB_TIMING_HFP,
            .vsync_pulse_width = ESP_PANEL_LCD_RGB_TIMING_VPW,
            .vsync_back_porch = ESP_PANEL_LCD_RGB_TIMING_VBP,
            .vsync_front_porch = ESP_PANEL_LCD_RGB_TIMING_VFP,
        },
        .data_width = ESP_PANEL_LCD_RGB_DATA_WIDTH,
        .bits_per_pixel = ESP_PANEL_LCD_RGB_PIXEL_BITS,
        .num_fbs = ESP_PANEL_LCD_RGB_FRAME_BUF_NUM,
        .bounce_buffer_size_px = 10 * ESP_PANEL_LCD_HEIGHT,
        .psram_trans_align = 64,
        .hsync_gpio_num = ESP_PANEL_LCD_PIN_NUM_RGB_HSYNC,
        .vsync_gpio_num = ESP_PANEL_LCD_PIN_NUM_RGB_VSYNC,
        .de_gpio_num = ESP_PANEL_LCD_PIN_NUM_RGB_DE,
        .pclk_gpio_num = ESP_PANEL_LCD_PIN_NUM_RGB_PCLK,
        .disp_gpio_num = ESP_PANEL_LCD_PIN_NUM_RGB_DISP,
        .data_gpio_nums = {
            ESP_PANEL_LCD_PIN_NUM_RGB_DATA0, ESP_PANEL_LCD_PIN_NUM_RGB_DATA1,
            ESP_PANEL_LCD_PIN_NUM_RGB_DATA2, ESP_PANEL_LCD_PIN_NUM_RGB_DATA3,
            ESP_PANEL_LCD_PIN_NUM_RGB_DATA4, ESP_PANEL_LCD_PIN_NUM_RGB_DATA5,
            ESP_PANEL_LCD_PIN_NUM_RGB_DATA6, ESP_PANEL_LCD_PIN_NUM_RGB_DATA7,
            ESP_PANEL_LCD_PIN_NUM_RGB_DATA8, ESP_PANEL_LCD_PIN_NUM_RGB_DATA9,
            ESP_PANEL_LCD_PIN_NUM_RGB_DATA10, ESP_PANEL_LCD_PIN_NUM_RGB_DATA11,
            ESP_PANEL_LCD_PIN_NUM_RGB_DATA12, ESP_PANEL_LCD_PIN_NUM_RGB_DATA13,
            ESP_PANEL_LCD_PIN_NUM_RGB_DATA14, ESP_PANEL_LCD_PIN_NUM_RGB_DATA15,
        },
        .flags = {
            .disp_active_low = 0,
            .refresh_on_demand = 0,
            .fb_in_psram = true,
            .double_fb = true,
        },
    };
    esp_lcd_new_rgb_panel(&rgb_config, &panel_handle);
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
}

// ---------------- LVGL-FRIENDLY WINDOW ----------------
void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, lv_color_t* color) {
    lv_area_t area = { (lv_coord_t)Xstart, (lv_coord_t)Ystart, (lv_coord_t)Xend, (lv_coord_t)Yend };
    lv_disp_t* disp = lv_disp_get_default();
    if (disp && disp->driver.flush_cb) {
        disp->driver.flush_cb(&disp->driver, &area, color);
    }
}

// ---------------- FULL LCD INIT ----------------
void LCD_Init() {
    ST7701_Reset();
    ST7701_Init();
    Touch_Init();
    Backlight_Init();
}
