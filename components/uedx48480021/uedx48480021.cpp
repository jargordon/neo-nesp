#include "uedx48480021.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#ifdef USE_ESP_IDF
#include "esp_task_wdt.h"
#endif

namespace esphome {
namespace uedx48480021 {

static const char *const TAG = "uedx48480021";

// ST7701S initialization commands for UEDX48480021-MD80ET
// Based on working Arduino library and vendor specifications
static const uint8_t INIT_SEQUENCE[][2] = {
    {0xFF, 0x77}, {0x01, 0x00}, {0x00, 0x13},  // Enable Command2 BK3
    {0xEF, 0x08},                               // Unknown
    {0xFF, 0x77}, {0x01, 0x00}, {0x00, 0x10},  // Enable Command2 BK0
    {0xC0, 0x3B}, {0x00, 0x00},                 // Display Line Setting
    {0xC1, 0x0D}, {0x02, 0x00},                 // Porch Control
    {0xC2, 0x21}, {0x08, 0x00},                 // Inversion Selection
    {0xCD, 0x08},                               // Unknown
    // Positive Voltage Gamma Control
    {0xB0, 0x00}, {0x11, 0x18}, {0x0E, 0x11}, {0x06, 0x07}, {0x08, 0x07},
    {0x10, 0x29}, {0x04, 0x0F}, {0x0E, 0x1E}, {0x03, 0x00},
    // Negative Voltage Gamma Control  
    {0xB1, 0x00}, {0x11, 0x19}, {0x0E, 0x12}, {0x07, 0x08}, {0x08, 0x08},
    {0x03, 0x29}, {0x06, 0x0F}, {0x0E, 0x1E}, {0x00, 0x00},
    {0xFF, 0x77}, {0x01, 0x00}, {0x00, 0x11},  // Enable Command2 BK1
    {0xB0, 0x6D},                               // Vop Amplitude Setting
    {0xB1, 0x37},                               // VCOM Amplitude Setting
    {0xB2, 0x81},                               // VGH Clamp Level
    {0xB3, 0x80},                               // Unknown
    {0xB5, 0x43},                               // VGL Clamp Level
    {0xB7, 0x85},                               // Unknown
    {0xB8, 0x20},                               // Unknown
    {0xC1, 0x78},                               // Unknown
    {0xC2, 0x78},                               // Unknown
    {0xD0, 0x88},                               // Unknown
    {0xE0, 0x00}, {0x00, 0x02},                 // Unknown
    {0xE1, 0x03}, {0xA0, 0x00}, {0x00, 0x04}, {0xA0, 0x00}, {0x00, 0x06},
    {0xA0, 0x00}, {0x00, 0x00},
    {0xE2, 0x00}, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00},
    {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00},
};

void UEDX48480021::setup() {
  ESP_LOGCONFIG(TAG, "Setting up UEDX48480021 display...");

  // Initialize reset pin
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(true);
    delay(10);
    yield();
    this->reset_pin_->digital_write(false);
    delay(10);
    yield();
    this->reset_pin_->digital_write(true);
    delay(120);
    yield();
  }

  // Initialize backlight pin
  if (this->backlight_pin_ != nullptr) {
    ESP_LOGD(TAG, "Initializing backlight pin...");
    this->backlight_pin_->setup();
    this->backlight_pin_->digital_write(false);  // Try LOW first
    delay(10);
    this->backlight_pin_->digital_write(true);   // Then HIGH
    delay(10);
    this->backlight_pin_->digital_write(false);  // Back to LOW
    delay(10);
    this->backlight_pin_->digital_write(true);   // Finally HIGH
    ESP_LOGD(TAG, "Backlight toggled - final state: HIGH");
  } else {
    ESP_LOGW(TAG, "No backlight pin configured!");
  }

#ifdef USE_ESP_IDF
  this->init_lcd_();
#else
  ESP_LOGE(TAG, "This component requires ESP-IDF framework");
  Component::mark_failed();
  return;
#endif

  this->initialized_ = true;
  ESP_LOGCONFIG(TAG, "UEDX48480021 display setup complete");
}

#ifdef USE_ESP_IDF
void UEDX48480021::init_lcd_() {
  ESP_LOGD(TAG, "Initializing ST7701S via SPI...");

  // Step 1: Send ST7701S initialization commands via SPI
  this->spi_setup();
  
  ESP_LOGD(TAG, "Sending ST7701S SLPOUT (sleep out)...");
  this->enable();
  this->write_byte(0x11);  // SLPOUT command
  this->disable();
  delay(120);
  yield();
  
  ESP_LOGD(TAG, "ST7701S initialization complete");
  // Note: SPI already disabled by disable() above
  
  ESP_LOGD(TAG, "Creating RGB panel with ST7701S configuration...");
  yield();

  // Configure RGB panel with exact timing from working firmware
  esp_lcd_rgb_panel_config_t panel_config = {};
  panel_config.clk_src = LCD_CLK_SRC_DEFAULT;
  panel_config.timings.pclk_hz = 16 * 1000 * 1000;  // Try 16MHz first (ST7701S datasheet typical)
  panel_config.timings.h_res = 480;
  panel_config.timings.v_res = 480;
  panel_config.timings.hsync_pulse_width = 8;
  panel_config.timings.hsync_back_porch = 50;  // Increased from 20
  panel_config.timings.hsync_front_porch = 50;  // Increased from 40
  panel_config.timings.vsync_pulse_width = 8;
  panel_config.timings.vsync_back_porch = 20;
  panel_config.timings.vsync_front_porch = 20;  // Reduced from 50
  panel_config.timings.flags.pclk_active_neg = true;  // Try inverted clock
  panel_config.timings.flags.hsync_idle_low = false;
  panel_config.timings.flags.vsync_idle_low = false;

  // RGB data pins
  panel_config.data_width = 16;
  panel_config.bits_per_pixel = 16;
  panel_config.num_fbs = 1;
  panel_config.bounce_buffer_size_px = 480 * 10;  // Avoid screen drift

  panel_config.hsync_gpio_num = this->hsync_pin_->get_pin();
  panel_config.vsync_gpio_num = this->vsync_pin_->get_pin();
  panel_config.de_gpio_num = this->de_pin_->get_pin();
  panel_config.pclk_gpio_num = this->pclk_pin_->get_pin();
  panel_config.disp_gpio_num = -1;

  // Assign data pins in correct order: B0-B4, G0-G5, R0-R4
  for (size_t i = 0; i < this->blue_pins_.size() && i < 5; i++) {
    panel_config.data_gpio_nums[i] = this->blue_pins_[i]->get_pin();
  }
  for (size_t i = 0; i < this->green_pins_.size() && i < 6; i++) {
    panel_config.data_gpio_nums[5 + i] = this->green_pins_[i]->get_pin();
  }
  for (size_t i = 0; i < this->red_pins_.size() && i < 5; i++) {
    panel_config.data_gpio_nums[11 + i] = this->red_pins_[i]->get_pin();
  }

  panel_config.flags.fb_in_psram = 1;

  esp_err_t err = esp_lcd_new_rgb_panel(&panel_config, &this->panel_handle_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create RGB panel: %s", esp_err_to_name(err));
    Component::mark_failed();
    return;
  }

  err = esp_lcd_panel_reset(this->panel_handle_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Panel reset failed: %s", esp_err_to_name(err));
  }

  err = esp_lcd_panel_init(this->panel_handle_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Panel init failed: %s", esp_err_to_name(err));
    Component::mark_failed();
    return;
  }

  // Reset the panel
  err = esp_lcd_panel_reset(this->panel_handle_);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Panel reset warning: %s", esp_err_to_name(err));
  }

  // Get the framebuffer from RGB panel
  esp_lcd_rgb_panel_get_frame_buffer(this->panel_handle_, 1, (void**)&this->fb_, nullptr);
  if (this->fb_ == nullptr) {
    ESP_LOGE(TAG, "Failed to get framebuffer from RGB panel");
    Component::mark_failed();
    return;
  }
  ESP_LOGD(TAG, "Framebuffer obtained at %p", this->fb_);

  // Test: Fill entire framebuffer with bright red (RGB565: 0xF800)
  ESP_LOGD(TAG, "Filling framebuffer with red for testing...");
  uint16_t red565 = 0xF800;  // Pure red in RGB565
  for (int i = 0; i < 480 * 480; i++) {
    this->fb_[i] = red565;
  }
  ESP_LOGD(TAG, "Test fill complete");

  ESP_LOGD(TAG, "RGB panel created successfully");
}

void UEDX48480021::send_init_commands_() {
  ESP_LOGD(TAG, "Sending ST7701S init commands...");

  // Sleep out
  ESP_LOGD(TAG, "Sending SLPOUT...");
  this->write_command_(ST7701_CMD_SLPOUT);
  delay(120);
  yield();  // Let scheduler run during long delay
  ESP_LOGD(TAG, "SLPOUT complete");

  // Color mode - RGB565 (16-bit)
  ESP_LOGD(TAG, "Setting color mode...");
  this->write_command_(ST7701_CMD_COLMOD);
  this->write_data_(0x55);  // 16-bit/pixel

  // Memory Access Control
  ESP_LOGD(TAG, "Setting MADCTL...");
  this->write_command_(ST7701_CMD_MADCTL);
  this->write_data_(0x00);  // No mirror/rotation

  // Display inversion on
  ESP_LOGD(TAG, "Setting INVON...");
  this->write_command_(ST7701_CMD_INVON);

  // Send vendor-specific initialization sequence
  ESP_LOGD(TAG, "Sending %d vendor init commands...", sizeof(INIT_SEQUENCE) / sizeof(INIT_SEQUENCE[0]));
  size_t idx = 0;
  while (idx < sizeof(INIT_SEQUENCE) / sizeof(INIT_SEQUENCE[0])) {
    uint8_t cmd = INIT_SEQUENCE[idx][0];
    uint8_t data = INIT_SEQUENCE[idx][1];
    this->write_command_(cmd);
    this->write_data_(data);
    idx++;
    
    // Yield every 5 commands to prevent watchdog
    if (idx % 5 == 0) {
      yield();
      ESP_LOGD(TAG, "Progress: %d/%d commands", idx, sizeof(INIT_SEQUENCE) / sizeof(INIT_SEQUENCE[0]));
    }
  }

  delay(10);
  yield();

  // Display on
  ESP_LOGD(TAG, "Sending DISPON...");
  this->write_command_(ST7701_CMD_DISPON);
  delay(50);
  yield();

  ESP_LOGD(TAG, "Init commands sent successfully");
}

void UEDX48480021::write_command_(uint8_t cmd) {
  this->enable();
  // 3-wire SPI: D/C bit is embedded in first bit (0 = command)
  this->write_byte(0x00);  // D/C low for command
  this->write_byte(cmd);
  this->disable();
}

void UEDX48480021::write_data_(uint8_t data) {
  this->enable();
  // 3-wire SPI: D/C bit is embedded in first bit (1 = data)
  this->write_byte(0x01);  // D/C high for data
  this->write_byte(data);
  this->disable();
}
#endif

void UEDX48480021::update() {
  if (!this->initialized_) {
    return;
  }
  ESP_LOGD(TAG, "Update called - drawing frame");
  this->do_update_();
  ESP_LOGD(TAG, "Frame drawn to framebuffer");
}

void UEDX48480021::draw_pixels_() {
  // Not needed - we draw directly to the RGB panel's framebuffer
  // The panel automatically displays whatever is in fb_
}

void UEDX48480021::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x >= this->get_width_internal() || y >= this->get_height_internal() || x < 0 || y < 0) {
    return;
  }

#ifdef USE_ESP_IDF
  if (this->fb_ == nullptr) {
    return;
  }

  // Convert RGB888 to RGB565
  uint16_t color565 = ((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | (color.b >> 3);
  
  // Swap bytes for correct endianness (ESP32 is little-endian, display expects big-endian RGB565)
  color565 = (color565 >> 8) | (color565 << 8);

  // Write directly to RGB panel's framebuffer
  int index = y * this->get_width_internal() + x;
  this->fb_[index] = color565;
#endif
}

}  // namespace uedx48480021
}  // namespace esphome
