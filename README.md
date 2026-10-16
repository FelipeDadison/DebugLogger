# DebugLogger

Advanced logging system for ESP32 with multi-level filtering, file logging, tag support, and optional CommandInterface integration.

## Installation

### PlatformIO

Add to your `platformio.ini`:

```ini
lib_deps =
    https://github.com/FelipeDadison/DebugLogger.git
```

### Arduino IDE

Download the ZIP from the [latest release](https://github.com/FelipeDadison/DebugLogger/releases/latest), then use *Sketch → Include Library → Add .ZIP Library...*.

## Usage

### Basic Setup
```cpp
#include "DebugLogger.h"

void setup() {
    Serial.begin(115200);
    
    // Add output stream
    Logger.addOutput(&Serial);
    
    // Set logging level
    Logger.setLevel(LogLevel::Debug);
    
    // Log messages (printf-style)
    Logger.info("System started");
    Logger.debug("Free heap: %lu bytes", (unsigned long)ESP.getFreeHeap());
}

void loop() {
    Logger.info("Main loop running");
    delay(1000);
}
```

Format strings are checked at compile time: a wrong specifier (e.g. `%d` for a `uint32_t`) produces a compiler warning.

### Colors

Console output uses ANSI colors by default. Terminals without ANSI support, such as the Arduino IDE Serial Monitor, show escape codes instead; disable them with:

```cpp
Logger.setColors(false);
```

### Tagged Logging
```cpp
void setup() {
    Logger.addOutput(&Serial);
    
    // Log with tags
    Logger.infoTagged("WiFi", "Connecting to network");
    Logger.infoTagged("Sensor", "Temperature: 25.5°C");
    
    // Show only some tags
    Logger.enableTag("WiFi");
    Logger.infoTagged("WiFi", "Shown");
    Logger.infoTagged("Sensor", "Hidden - tag not enabled");
    Logger.info("Shown - untagged messages are always shown");
}
```

Tag rules (the same for console and file):

- With no tags enabled, all tags are shown.
- Once a tag is enabled, only enabled tags are shown.
- Untagged messages are always shown.
- `clearTags()` removes the filter, showing all tags again.

### File Logging
```cpp
#include <SPIFFS.h>

void setup() {
    SPIFFS.begin(true);
    
    // Console shows everything, the file keeps only warnings and errors
    Logger.addOutput(&Serial);
    Logger.setLevel(LogLevel::Debug);
    Logger.beginFileLogging(SPIFFS, LogLevel::Warn, "/logs.txt", 100);
    
    Logger.info("This goes to console");
    Logger.errorTagged("System", "This goes to both console and file");
}

void loop() {
    Logger.update();  // Write pending file logs
    delay(100);
}
```

- Log calls never write to flash themselves: lines wait in memory until `update()` appends them to the file. Call `update()` regularly in your loop.
- The file keeps the last `maxLines` lines. New lines are appended; the file is trimmed only when it grows past twice the limit, to reduce flash wear.
- At most `maxLines` lines wait in memory; if `update()` isn't called, the oldest are dropped.
- `readAllFileLogs()` returns the most recent lines, including lines not yet written to the file.
- Each line starts with the date and time when the clock is set (e.g. via NTP), or the uptime otherwise: `2026-10-16 14:03:12 [ERROR] [System] ...` or `+123.456s [ERROR] ...`.

### Throttled Logging
```cpp
void loop() {
    // Log at most once per 5 seconds (the first call always logs)
    Logger.infoEveryN("perf", 5000, "Free heap: %lu bytes", (unsigned long)ESP.getFreeHeap());
}
```

The tag is also the throttle key: use a different tag for each throttled message.

### With CommandInterface (Optional)
```cpp
#include "DebugLogger.h"
#include "DebugLoggerCLI.h"

void setup() {
    Serial.begin(115200);
    
    Cli.addInputStream(Serial);
    Cli.addOutputStream(Serial);
    Cli.setProjectInfo("MyDevice", "1.0.0", "ESP32");
    
    // Register logger CLI commands
    registerLoggerCLI(Logger);
    
    Logger.addOutput(&Serial);
    Logger.setLevel(LogLevel::Info);
}

void loop() {
    Cli.update();
    Logger.update();
}

// Now use CLI commands: LOGGER_SET_LEVEL\4, LOGGER_GET_FILE, etc.
```

`DebugLoggerCLI.h` adds `registerLoggerCLI()` and includes `CommandInterface.h` for you. It requires [CommandInterface](https://github.com/FelipeDadison/CommandInterface) 1.1.0 or newer; projects that don't include `DebugLoggerCLI.h` don't need CommandInterface at all.

| Command | Description |
|---|---|
| `LOGGER_SET_LEVEL\level` | Set console log level (0 None, 1 Error, 2 Warn, 3 Info, 4 Debug) |
| `LOGGER_GET_LEVEL` | Get console log level |
| `LOGGER_SET_FILE_LEVEL\level` | Set file log level |
| `LOGGER_GET_FILE_LEVEL` | Get file log level |
| `LOGGER_GET_FILE` | Show the log file |
| `LOGGER_CLEAR_FILE` | Clear the log file |
| `LOGGER_ENABLE_TAG\tag` | Show only enabled tags |
| `LOGGER_DISABLE_TAG\tag` | Remove a tag from the enabled tags |
| `LOGGER_CLEAR_TAG` | Clear the tag filter (all tags shown) |

Complete sketches are in [`examples/`](examples).

## API

### Constructors
- `DebugLogger()` - Create logger instance (or use global `Logger`)

### Output Management
- `void addOutput(Print* output)` - Add output stream (Serial, NetSerial, etc.)
- `void removeOutput(Print* output)` - Remove output stream
- `void clearOutputs()` - Remove all output streams
- `void setColors(bool enabled)` - Enable/disable ANSI colors (default: enabled)

### Console Logging Control
- `void setLevel(LogLevel level)` - Set console log level
- `LogLevel getLevel()` - Get current console log level

### Console Tag Filtering
- `void enableTag(const String& tag)` - Show only enabled tags
- `void disableTag(const String& tag)` - Remove a tag from the enabled tags
- `void clearTags()` - Clear the filter (all tags shown)

### File Logging
- `bool beginFileLogging(FS &fs, LogLevel level, const char* path, size_t maxLines)` - Start file logging
- `void stopFileLogging()` - Stop file logging
- `void setFileLevel(LogLevel level)` - Set file log level
- `LogLevel getFileLevel()` - Get file log level
- `std::vector<String> readAllFileLogs()` - Read the most recent log lines
- `bool clearFileLogs()` - Clear log file and pending lines

### File Tag Filtering
- `void enableFileTag(const String& tag)` - Write only enabled tags to file
- `void disableFileTag(const String& tag)` - Remove a tag from the enabled file tags
- `void clearFileTags()` - Clear the file filter (all tags written)

### Core Logging
- `bool update()` - Write pending file logs (call in loop)
- `void error(format, ...)` - Log Error message
- `void warn(format, ...)` - Log Warn message
- `void info(format, ...)` - Log Info message
- `void debug(format, ...)` - Log Debug message

### Tagged Logging
- `void errorTagged(tag, format, ...)` - Log Error with tag
- `void warnTagged(tag, format, ...)` - Log Warn with tag
- `void infoTagged(tag, format, ...)` - Log Info with tag
- `void debugTagged(tag, format, ...)` - Log Debug with tag

### Throttled Logging
- `void infoEveryN(tag, interval_ms, format, ...)` - Log at most once per interval

## Log Levels

- `LogLevel::None` - No logging
- `LogLevel::Error` - Errors only
- `LogLevel::Warn` - Warnings and errors
- `LogLevel::Info` - Info, warnings, and errors
- `LogLevel::Debug` - All messages

## Thread Safety

All methods can be called from any FreeRTOS task, e.g. WiFi event handlers or async web server callbacks. Lines from different tasks are never mixed. Logging from interrupt handlers (ISRs) is not supported.

## Dependencies

- Arduino Framework
- FS (for file logging)

## Optional Dependencies

- [CommandInterface](https://github.com/FelipeDadison/CommandInterface) 1.1.0+ (only when using `DebugLoggerCLI.h`)

## License

MIT
