# neo-nESP AI Coding Instructions

## Project Overview
ESPHome-based Nest Thermostat clone for WT32S3-21S rotary display (480x480, 2.1"). Controls Home Assistant climate entities via rotary knob and center button. Features ambient weather display when HVAC is off, zone support via covers, and boost timers.

## Core Architecture

### File Structure & Responsibility
- **neo-nesp.yaml**: Main ESPHome config - hardware (ESP32-S3, PSRAM, display backlight, RGB ring, vibration motor), Home Assistant entity substitutions, sensors (rotary encoder, button), scripts, and device-specific logic
- **_common.yaml**: Shared connectivity (WiFi, API, OTA, web server) - never hardcode credentials here, use `!secret` references
- **_neo-nesp_UI.yaml**: Complete LVGL UI definition - fonts (Roboto + Material Design Icons), pages (main thermostat, menu, mode/preset selection), widgets, event handlers
- **colors/HOMEASSISTANT**: Color palette (Material Design-based) used via `packages:` include - defines `black`, `white`, `gray*`, `red*`, `orange*`, `blue*` color IDs
- **fonts/**: Contains `materialdesignicons-webfont.ttf` - specific Unicode glyphs declared in font definitions (thermometer, HVAC modes, weather icons)
- **secrets.yaml**: Credentials (WiFi SSID/password, API/OTA keys) - NOT in git, referenced via `!secret` syntax

### Entity Integration Pattern
All Home Assistant entities are configured via `substitutions:` at the top of `neo-nesp.yaml`:
```yaml
substitutions:
  climate_entity: climate.house_heating
  weather_entity: weather.met_office_tyn_y_rhos_hall
  boost_input_boolean: input_boolean.heating_boost
```
When adding features, always use substitutions for entity IDs to allow easy customization. Import HA entities using `platform: homeassistant` with `entity_id:` or `attribute:` for reading values.

## Critical Patterns

### LVGL UI State Management
- **Active page tracking**: Global `active_lvgl_page` (std::string) stores current page name. Lambda conditions use `strcmp(id(active_lvgl_page).c_str(), "main_page") == 0` for page-specific logic
- **Temperature handling**: ESPHome doesn't support decimals in arc/meter values - multiply temperatures by 10 (e.g., `22.5°C` → `225`). Update patterns:
  ```yaml
  - lvgl.arc.update:
      id: current_temp_arc
      value: !lambda return x * 10;
  ```
- **Widget state control**: Always disable navigation buttons on page unload (`on_unload: - lvgl.widget.disable:`) and enable on load to prevent encoder interaction on hidden pages
- **Focus management**: Use `lvgl.widget.focus:` to set initial selection when pages load

### Rotary Encoder Interaction
The rotary encoder (`sensor: rotary`) has dual purpose:
1. **Main page**: Adjusts climate set temperature (±0.5°C per detent) when HVAC is not off/fan_only. Auto-turns on HVAC if turned while off
2. **Menu pages**: Navigation between buttons (encoder is LVGL input device)

Always reset idle timer and backlight on interaction:
```yaml
on_value:
  - script.stop: idle_screen_timer
  - script.execute: idle_screen_timer
  - light.turn_on: backlight
```

### Script Patterns
- **Screen management**: `goto_*_page` scripts handle page transitions with `lvgl.page.show:`. Always update `active_lvgl_page` global in page `on_load:`
- **Idle behavior**: `idle_screen_timer` (configurable delay) triggers screen dimming/sleep. Scripts should reset this on any user interaction
- **Preset/Mode changes**: Use `homeassistant.action:` to call HA services (climate.set_preset_mode, climate.set_hvac_mode) rather than trying to control climate directly

### Anti-Burn-In Feature
Scheduled switch (`switch_antiburn`) runs at specific hours (2-5 AM) for 30 minutes. Uses `lvgl.pause: show_snow: true` to display animated snow pattern. Always check `lvgl.is_paused` before resuming in other scripts.

## Development Workflow

### Building/Flashing
1. Install ESPHome: `pip install esphome`
2. Validate config: `esphome config neo-nesp.yaml`
3. First flash (USB): `esphome run neo-nesp.yaml` (select USB port)
4. OTA updates: `esphome run neo-nesp.yaml` (select network device)

### Testing Changes
- Use ESPHome logs: `esphome logs neo-nesp.yaml` for real-time debugging
- LVGL UI changes require full reflash (no hot reload)
- Home Assistant entity changes need HA restart to register new sensors/services

### Common Pitfalls
- **YAML includes**: `<<: !include` for single file, `packages:` for color schemes. Order matters - includes must come after entity IDs they reference
- **Lambda syntax**: Must return values explicitly. Use `id(sensor_name).state` to access values. String comparisons need `strcmp()`
- **Font glyphs**: Adding new icons requires Unicode glyph declaration in font `extras:` section - find codes at https://pictogrammers.github.io
- **PSRAM config**: ESP32-S3 requires `psram: mode: octal` and specific `sdkconfig_options` - don't modify unless necessary
- **Time-based actions**: Use `time: platform: homeassistant` with `on_time:` for scheduled behaviors, not delay loops

## Zone & Weather Features
- **Zones**: Controlled via cover entities (e.g., `cover.heating_zone_1`) referenced in menu buttons. Optional - can be removed
- **Weather display**: Shows when HVAC is off. Maps HA weather states to Material Design Icons via switch/case in lambda. Update `weather_icon_label` text with Unicode glyph

## Color Customization
To change theme, edit `colors/HOMEASSISTANT` or create new color file and update package include. Colors referenced by ID (e.g., `bg_color: black`) throughout UI definitions. Follow Material Design contrast guidelines for legibility.

## Key Files for Reference
- Temperature arc logic: [neo-nesp.yaml](neo-nesp.yaml#L182-L210)
- Main thermostat UI: [_neo-nesp_UI.yaml](_neo-nesp_UI.yaml#L146-L324)
- Weather icon mapping: Search "weather_icon_label" in neo-nesp.yaml
- Menu navigation: [_neo-nesp_UI.yaml](_neo-nesp_UI.yaml#L326-L473)
