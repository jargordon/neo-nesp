# VIEWESMART UEDX48480021-MD80ET Porting Notes

## Hardware Comparison

### Display Specifications
Both displays share:
- **Resolution**: 480x480 px
- **Size**: 2.1 inch
- **ESP32**: ESP32-S3-R8 with 8MB PSRAM (Octal), 16MB Flash
- **Rotary Encoder**: Same GPIO pins (PHA=6, PHB=5)

### Key GPIO Differences

| Function | Current (WT32S3-21S) | New (VIEWESMART MD80ET) | Change Required |
|----------|---------------------|-------------------------|-----------------|
| **Backlight** | GPIO 38 | GPIO 7 | ✅ **Must change** |
| **Button** | GPIO 3 | GPIO 0 (boot button) | ✅ **Must change** |
| **Encoder A** | GPIO 6 | GPIO 6 | ✓ Same |
| **Encoder B** | GPIO 5 | GPIO 5 | ✓ Same |
| **Touch SDA** | N/A | GPIO 16 | ⭐ **New feature** |
| **Touch SCL** | N/A | GPIO 15 | ⭐ **New feature** |
| **RGB Ring** | GPIO 4 | ? | ⚠️ **Check compatibility** |
| **Vibration** | GPIO 7 | ? | ⚠️ **Conflict with backlight!** |

### Display Driver Differences
- **Current**: Uses LVGL with direct RGB565 interface
- **New**: ST7701S chip with 3-wire SPI-RGB (requires ESP32_Display_Panel library)
  - DE: GPIO 17
  - VSYNC: GPIO 3
  - HSYNC: GPIO 46
  - PCLK: GPIO 9
  - RGB Data lines: GPIO 10-14, 21, 47, 48, 45, 38-42, 2, 1

## Critical Changes Needed in neo-nesp.yaml

### 1. Backlight (CONFLICT!)
```yaml
# OLD (line ~58)
output:
  - platform: ledc
    id: display_led
    pin: 38  # <-- Change to 7

# CONFLICT: Vibration motor currently on GPIO 7!
switch:
  - platform: gpio
    id: vibration
    pin: 7  # <-- Must move to different GPIO or remove
```

### 2. Button
```yaml
# OLD (line ~143)
binary_sensor:
  - platform: gpio
    name: Button
    id: push_button
    pin:
      number: 3  # <-- Change to 0 (boot button)
      mode: INPUT
      inverted: true
      ignore_strapping_warning: true  # <-- Keep this for GPIO 0
```

### 3. Touch Support (NEW)
Add to neo-nesp.yaml:
```yaml
i2c:
  sda: 16
  scl: 15
  scan: true
  id: bus_a

touchscreen:
  - platform: cst226  # or cst816 - check ESPHome docs
    interrupt_pin: GPIO_NUM  # Need to identify from schematic
    id: touch_panel
    on_touch:
      - lambda: |-
          ESP_LOGI("touch", "x=%d, y=%d", touch.x, touch.y);
```

### 4. Display Driver
**GOOD NEWS**: ESPHome natively supports the ST7701S display driver!

The VIEWESMART UEDX48480021-MD80ET is **pre-configured** in ESPHome's codebase. You have two options:

**Option A: Use the newer MIPI RGB driver** (Recommended - deprecated warning on ST7701S)
```yaml
display:
  - platform: mipi_rgb
    model: UEDX48480021-MD80ET  # Pre-configured in ESPHome!
    id: neo_display
    update_interval: never
    auto_clear_enabled: false
    spi_id: spi_bus  # Reference to your SPI bus
```

**Option B: Use the ST7701S driver** (Works but being phased out)
```yaml
spi:
  id: spi_bus
  clk_pin: 12
  mosi_pin: 13

display:
  - platform: st7701s
    dimensions:
      width: 480
      height: 480
    cs_pin: 18
    reset_pin: 8
    de_pin: 17
    hsync_pin: 46
    vsync_pin: 3
    pclk_pin: 9
    data_pins:
      red: [10, 11, 12, 13, 14]
      green: [21, 47, 48, 45, 38, 39]
      blue: [40, 41, 42, 2, 1]
```

**Requirements**:
- ESP-IDF framework (already configured ✓)
- PSRAM (already have Octal 8MB ✓)
- ESP32-S3 (already using ✓)

## Files to Modify

1. **neo-nesp.yaml**
   - Line 58: Change backlight pin to 7
   - Line 87: Remove or relocate vibration motor
   - Line 143: Change button pin to 0
   - Add I2C and touchscreen configuration
   - Add display driver configuration for ST7701S

2. **_neo-nesp_UI.yaml**
   - May need adjustments for touch input handling
   - Encoder should work as-is (same pins)

3. **_common.yaml**
   - No changes expected

## Migration Strategy

### Phase 1: Basic Hardware (No Touch)
1. Update GPIO pins (backlight, button)
2. Test rotary encoder (should work as-is)
3. Remove/disable vibration motor
4. Verify display output with ST7701S driver

### Phase 2: Touch Integration
1. Add I2C bus configuration
2. Configure CST826 touchscreen
3. Add touch event handlers to LVGL pages
4. Test touch calibration

### Phase 3: Optimization
1. Re-enable or relocate vibration motor if desired
2. Add touch gestures (swipe for page navigation?)
3. Calibrate backlight levels
4. Test anti-burn-in with new display

## Resources

- [VIEWESMART GitHub](https://github.com/VIEWESMART/UEDX48480021-MD80ESP32-2.1inch-Touch-Knob-Display)
- [ESP32_Display_Panel Library](https://github.com/esp-arduino-libs/ESP32_Display_Panel)
- [ESPHome I2C Component](https://esphome.io/components/i2c.html)
- [ESPHome Touchscreen](https://esphome.io/components/touchscreen/index.html)
- [ST7701S Datasheet](https://github.com/VIEWESMART/UEDX48480021-MD80ESP32-2.1inch-Touch-Knob-Display/blob/main/information/ALL-UE021WV-RB40-A009A%20V1.0%20SPEC.pdf)

## Next Steps

1. ✅ Create git branch: `viewesmart-display`
2. ✅ Update GPIO pin assignments in neo-nesp.yaml
   - ✅ Backlight: GPIO 38 → GPIO 7
   - ✅ Button: GPIO 3 → GPIO 0 (boot button)
   - ✅ Vibration motor: Disabled (GPIO 7 conflict)
3. ✅ **CONFIRMED: ESPHome has native ST7701S support!**
   - Model `UEDX48480021-MD80ET` is pre-configured in ESPHome
   - Can use either `mipi_rgb` (recommended) or `st7701s` platform
4. ✅ **Display driver integrated!**
   - Replaced old `st7701s` platform with `mipi_rgb`
   - Using pre-configured model - no manual pin config needed!
   - Removed 100+ lines of manual init sequences
5. ⬜ Test basic functionality (encoder, button, backlight, display)
6. ⬜ Add touch support (CST826 via I2C)
7. ⬜ Update copilot-instructions.md with new hardware details

**Ready to flash and test!**
