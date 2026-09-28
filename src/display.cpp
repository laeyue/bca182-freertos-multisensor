#include "display.h"
#include "board.h"
#include "rtos_objects.h"
#include "task.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
constexpr uint16_t OLED_ADDRESS = 0x3cU << 1;
uint8_t frame[128 * 8] = {};

// Five-column glyphs for uppercase letters, digits and display punctuation.
constexpr uint8_t letters[26][5] = {
  {0x7e,0x11,0x11,0x11,0x7e}, {0x7f,0x49,0x49,0x49,0x36},
  {0x3e,0x41,0x41,0x41,0x22}, {0x7f,0x41,0x41,0x22,0x1c},
  {0x7f,0x49,0x49,0x49,0x41}, {0x7f,0x09,0x09,0x09,0x01},
  {0x3e,0x41,0x49,0x49,0x7a}, {0x7f,0x08,0x08,0x08,0x7f},
  {0x00,0x41,0x7f,0x41,0x00}, {0x20,0x40,0x41,0x3f,0x01},
  {0x7f,0x08,0x14,0x22,0x41}, {0x7f,0x40,0x40,0x40,0x40},
  {0x7f,0x02,0x0c,0x02,0x7f}, {0x7f,0x04,0x08,0x10,0x7f},
  {0x3e,0x41,0x41,0x41,0x3e}, {0x7f,0x09,0x09,0x09,0x06},
  {0x3e,0x41,0x51,0x21,0x5e}, {0x7f,0x09,0x19,0x29,0x46},
  {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7f,0x01,0x01},
  {0x3f,0x40,0x40,0x40,0x3f}, {0x1f,0x20,0x40,0x20,0x1f},
  {0x7f,0x20,0x18,0x20,0x7f}, {0x63,0x14,0x08,0x14,0x63},
  {0x03,0x04,0x78,0x04,0x03}, {0x61,0x51,0x49,0x45,0x43}
};
constexpr uint8_t digits[10][5] = {
  {0x3e,0x51,0x49,0x45,0x3e}, {0x00,0x42,0x7f,0x40,0x00},
  {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4b,0x31},
  {0x18,0x14,0x12,0x7f,0x10}, {0x27,0x45,0x45,0x45,0x39},
  {0x3c,0x4a,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1e}
};
constexpr uint8_t blank[5] = {};
constexpr uint8_t dash[5] = {0x08,0x08,0x08,0x08,0x08};
constexpr uint8_t dot[5] = {0x00,0x60,0x60,0x00,0x00};
constexpr uint8_t percent[5] = {0x63,0x13,0x08,0x64,0x63};
constexpr uint8_t colon[5] = {0x00,0x36,0x36,0x00,0x00};

const uint8_t *glyph(char ch) {
  if (ch >= 'A' && ch <= 'Z') return letters[ch - 'A'];
  if (ch >= '0' && ch <= '9') return digits[ch - '0'];
  if (ch == '-') return dash;
  if (ch == '.') return dot;
  if (ch == '%') return percent;
  if (ch == ':') return colon;
  return blank;
}

void textAt(uint8_t x, uint8_t y, const char *text) {
  while (*text && x <= 122 && y <= 56) {
    const uint8_t *columns = glyph(*text++);
    const uint16_t offset = static_cast<uint16_t>(y / 8U) * 128U + x;
    for (int i = 0; i < 5; ++i) frame[offset + i] = columns[i];
    x = static_cast<uint8_t>(x + 6U);
  }
}

bool command(uint8_t value) {
  uint8_t bytes[2] = {0x00, value};
  return HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDRESS, bytes, sizeof(bytes), 50) == HAL_OK;
}

void oledInit() {
  const uint8_t commands[] = {0xae,0xd5,0x80,0xa8,0x3f,0xd3,0x00,0x40,
      0x8d,0x14,0x20,0x02,0xa1,0xc8,0xda,0x12,0x81,0x8f,
      0xd9,0xf1,0xdb,0x40,0xa4,0xa6,0xaf};
  for (uint8_t item : commands) command(item);
}

void flush() {
  for (uint8_t page = 0; page < 8; ++page) {
    uint8_t position[] = {0x00, static_cast<uint8_t>(0xb0U + page), 0x00, 0x10};
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDRESS, position, sizeof(position), 50);
    uint8_t packet[129];
    packet[0] = 0x40;
    std::memcpy(packet + 1, &frame[static_cast<uint16_t>(page) * 128U], 128);
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDRESS, packet, sizeof(packet), 100);
  }
}

void render(DisplayMode mode, const SensorData &data, bool motion, bool alarm) {
  std::memset(frame, 0, sizeof(frame));
  textAt(0, 0, "ROOM MONITOR");
  char value[24] = {};
  switch (mode) {
    case DisplayMode::TEMPERATURE: {
      textAt(0, 16, "TEMPERATURE");
      const int scaled = static_cast<int>(data.temperature * 10.0f);
      if (data.dhtValid)
        std::snprintf(value, sizeof(value), "%s%d.%d C", scaled < 0 ? "-" : "",
                      std::abs(scaled) / 10, std::abs(scaled) % 10);
      else std::snprintf(value, sizeof(value), "SENSOR ERROR");
      break;
    }
    case DisplayMode::HUMIDITY: {
      textAt(0, 16, "HUMIDITY");
      const int scaled = static_cast<int>(data.humidity * 10.0f);
      if (data.dhtValid)
        std::snprintf(value, sizeof(value), "%d.%d %%", scaled / 10, scaled % 10);
      else std::snprintf(value, sizeof(value), "SENSOR ERROR");
      break;
    }
    case DisplayMode::LIGHT:
      textAt(0, 16, "LIGHT");
      std::snprintf(value, sizeof(value), "%d %%", data.lightLevel);
      break;
    case DisplayMode::MOTION:
      textAt(0, 16, "MOTION");
      std::snprintf(value, sizeof(value), "%s", motion ? "DETECTED" : "NONE");
      break;
  }
  textAt(0, 32, value);
  textAt(0, 56, alarm ? "ALARM" : "ACTIVE");
  flush();
}
}  // namespace

void DisplayTask(void *) {
  oledInit();
  DisplayMode mode = DisplayMode::TEMPERATURE;
  SensorData data = {};
  bool motion = false;
  bool lastAlarm = false;
  bool dirty = true;
  for (;;) {
    if ((xEventGroupGetBits(systemEvents) & EVENT_ACTIVE) == 0) {
      command(0xae);
      xEventGroupWaitBits(systemEvents, EVENT_ACTIVE, pdFALSE, pdFALSE, portMAX_DELAY);
      command(0xaf);
      dirty = true;
    }
    SensorData newData;
    if (xQueueReceive(displaySensorQueue, &newData, pdMS_TO_TICKS(100)) == pdTRUE) {
      data = newData;
      dirty = true;
    }
    DisplayMode newMode;
    if (xQueueReceive(displayModeQueue, &newMode, 0) == pdTRUE) { mode = newMode; dirty = true; }
    bool newMotion;
    if (xQueueReceive(motionDisplayQueue, &newMotion, 0) == pdTRUE) {
      motion = newMotion;
      dirty = true;
    }
    const bool alarm = (xEventGroupGetBits(systemEvents) & EVENT_ALARM) != 0;
    if (alarm != lastAlarm) { lastAlarm = alarm; dirty = true; }
    if (dirty && (xEventGroupGetBits(systemEvents) & EVENT_ACTIVE)) {
      render(mode, data, motion, alarm);
      dirty = false;
    }
  }
}
