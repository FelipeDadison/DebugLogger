# DebugLogger

Advanced logging system for ESP32 with multi-level filtering, file logging, tag support, and optional CommandInterface integration.

## Installation

Add to your `platformio.ini`:

```ini
lib_deps =
    DebugLogger
```

## Usage

### Basic Setup
```cpp
#include "DebugLogger.h"

void setup() {
    Serial.begin(115200);
    
    // Add output stream
    Logger.addOutput(&Serial);
    
    // Set logging level
    Logger.setLevel(LogLevel::DEBUG);
    
    // Log messages
    Logger.info("System started");
    Logger.debug("Debug information");
}

void loop() {
    Logger.info("Main loop running");
    delay(1000);
}
```

### Tagged Logging
```cpp
void setup() {
    Logger.addOutput(&Serial);
    
    // Enable specific tags
    Logger.enableTag("WiFi");
    Logger.enableTag("Sensor");
    
    // Log with tags
    Logger.infoTagged("WiFi", "Connecting to network");
    Logger.infoTagged("Sensor", "Temperature: 25.5°C");
    Logger.infoTagged("Other", "This won't appear - tag not enabled");
}
```

### File Logging
```cpp
#include <SPIFFS.h>

void setup() {
    SPIFFS.begin(true);
    
    // Initialize both console and file logging
    Logger.addOutput(&Serial);
    Logger.setLevel(LogLevel::DEBUG);
    Logger.beginFileLogging(SPIFFS, LogLevel::WARN, "/logs.txt", 100);
    
    Logger.info("This goes to console");
    Logger.errorTagged("System", "This goes to both console and file");
}

void loop() {
    Logger.update();  // Flush file logs regularly
    delay(100);
}
```

### Throttled Logging
```cpp
void loop() {
    // Log at most once per 5 seconds
    Logger.infoEveryN("perf", 5000, "Free heap: %d bytes", ESP.getFreeHeap());
}
```

### With CommandInterface (Optional)
```cpp
#include "DebugLogger.h"
#include "CommandInterface.h"

void setup() {
    Serial.begin(115200);
    
    Cli.addInputStream(Serial);
    Cli.addOutputStream(Serial);
    Cli.setProjectInfo("MyDevice", "1.0.0", "ESP32");
    
    // Register logger CLI (automatic if CommandInterface included)
    registerLoggerCLI(Logger);
    
    Logger.addOutput(&Serial);
    Logger.setLevel(LogLevel::INFO);
}

void loop() {
    Cli.update();
    Logger.update();
}

// Now use CLI commands: LOGGER_SET_LEVEL\4, LOGGER_GET_FILE, etc.
```

## API

### Constructors
- `DebugLogger()` - Create logger instance (or use global `Logger`)

### Output Management
- `void addOutput(Print* output)` - Add output stream
- `void clearOutputs()` - Remove all output streams

### Console Logging Control
- `void setLevel(LogLevel level)` - Set console log level
- `LogLevel getLevel()` - Get current console log level

### Console Tag Filtering
- `void enableTag(const String& tag)` - Enable logging for tag
- `void disableTag(const String& tag)` - Disable logging for tag
- `void clearTags()` - Clear all tag filters

### File Logging
- `bool beginFileLogging(FS &fs, LogLevel level, const char* path, size_t maxLines)` - Start file logging
- `void stopFileLogging()` - Stop file logging
- `LogLevel getFileLevel()` - Get file log level
- `std::vector<String> readAllFileLogs()` - Read all log lines
- `bool clearFileLogs()` - Clear log file

### File Tag Filtering
- `void enableFileTag(const String& tag)` - Enable file logging for tag
- `void disableFileTag(const String& tag)` - Disable file logging for tag
- `void clearFileTags()` - Clear all file tag filters

### Core Logging
- `bool update()` - Flush pending file logs (call in loop)
- `void error(format, ...)` - Log ERROR message
- `void warn(format, ...)` - Log WARN message
- `void info(format, ...)` - Log INFO message
- `void debug(format, ...)` - Log DEBUG message

### Tagged Logging
- `void errorTagged(tag, format, ...)` - Log ERROR with tag
- `void warnTagged(tag, format, ...)` - Log WARN with tag
- `void infoTagged(tag, format, ...)` - Log INFO with tag
- `void debugTagged(tag, format, ...)` - Log DEBUG with tag

### Throttled Logging
- `void infoEveryN(tag, interval_ms, format, ...)` - Log at most once per interval

## Log Levels

- `LogLevel::NONE` - No logging
- `LogLevel::ERROR` - Errors only
- `LogLevel::WARN` - Warnings and errors
- `LogLevel::INFO` - Info, warnings, and errors
- `LogLevel::DEBUG` - All messages

## Dependencies

- Arduino Framework
- FS (for file logging)

## Optional Dependencies

- CommandInterface (for CLI management commands)

## License

MIT
