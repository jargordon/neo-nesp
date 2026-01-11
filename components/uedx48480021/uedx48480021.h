#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/display/display_buffer.h"
#include "esphome/components/spi/spi.h"

#ifdef USE_ESP_IDF
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_rgb.h"
#endif

namespace esphome {
namespace uedx48480021 {

static const uint8_t ST7701_CMD_SWRESET = 0x01;
static const uint8_t ST7701_CMD_SLPIN = 0x10;
static const uint8_t ST7701_CMD_SLPOUT = 0x11;
static const uint8_t ST7701_CMD_INVOFF = 0x20;
static const uint8_t ST7701_CMD_INVON = 0x21;
static const uint8_t ST7701_CMD_DISPOFF = 0x28;
static const uint8_t ST7701_CMD_DISPON = 0x29;
static const uint8_t ST7701_CMD_MADCTL = 0x36;
static const uint8_t ST7701_CMD_COLMOD = 0x3A;

class UEDX48480021 : public display::DisplayBuffer,
                     public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW,
                                           spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_10MHZ> {
 public:
  void set_reset_pin(GPIOPin *reset_pin) { this->reset_pin_ = reset_pin; }
  void set_backlight_pin(GPIOPin *backlight_pin) { this->backlight_pin_ = backlight_pin; }
  void set_de_pin(InternalGPIOPin *de_pin) { this->de_pin_ = de_pin; }
  void set_pclk_pin(InternalGPIOPin *pclk_pin) { this->pclk_pin_ = pclk_pin; }
  void set_hsync_pin(InternalGPIOPin *hsync_pin) { this->hsync_pin_ = hsync_pin; }
  void set_vsync_pin(InternalGPIOPin *vsync_pin) { this->vsync_pin_ = vsync_pin; }

  void add_red_pin(InternalGPIOPin *pin) { this->red_pins_.push_back(pin); }
  void add_green_pin(InternalGPIOPin *pin) { this->green_pins_.push_back(pin); }
  void add_blue_pin(InternalGPIOPin *pin) { this->blue_pins_.push_back(pin); }

  void setup() override;
  void update() override;
  void loop() override {}
  float get_setup_priority() const override { return setup_priority::PROCESSOR; }

  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_COLOR; }

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  void draw_pixels_();
  int get_width_internal() override { return 480; }
  int get_height_internal() override { return 480; }

  void init_lcd_();
  void send_init_commands_();
  void write_command_(uint8_t cmd);
  void write_data_(uint8_t data);
  void write_init_sequence_(uint8_t cmd, const uint8_t *data, size_t len, uint32_t delay_ms);

  GPIOPin *reset_pin_{nullptr};
  GPIOPin *backlight_pin_{nullptr};
  InternalGPIOPin *de_pin_{nullptr};
  InternalGPIOPin *pclk_pin_{nullptr};
  InternalGPIOPin *hsync_pin_{nullptr};
  InternalGPIOPin *vsync_pin_{nullptr};

  std::vector<InternalGPIOPin *> red_pins_;
  std::vector<InternalGPIOPin *> green_pins_;
  std::vector<InternalGPIOPin *> blue_pins_;

#ifdef USE_ESP_IDF
  esp_lcd_panel_handle_t panel_handle_{nullptr};
  esp_lcd_panel_io_handle_t io_handle_{nullptr};
  uint16_t *fb_{nullptr};  // Pointer to RGB panel's framebuffer
#endif

  bool initialized_{false};
};

}  // namespace uedx48480021
}  // namespace esphome
