/**
 * @example with_cli.ino
 * @brief DebugLogger with CommandInterface integration (optional)
 * 
 * Features:
 * - Configure logger at runtime via CLI commands
 * - Control log levels and tag filtering
 * - Read and manage log files
 * 
 * Requires: CommandInterface library
 */

#include <Arduino.h>
#include <SPIFFS.h>
#include "DebugLogger.h"
#include "DebugLoggerCLI.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize filesystem
    SPIFFS.begin(true);
    
    // Setup CommandInterface
    Cli.addInputStream(Serial);
    Cli.addOutputStream(Serial);
    Cli.setProjectInfo("Logger CLI Example", "1.0.0", "ESP32");
    
    // Register logger commands (DebugLoggerCLI.h)
    registerLoggerCLI(Logger);
    
    // Setup logger
    Logger.addOutput(&Serial);
    Logger.setLevel(LogLevel::Info);
    Logger.beginFileLogging(SPIFFS, LogLevel::Warn, "/logs.txt", 50);
    
    // Enable some tags
    Logger.enableTag("Main");
    Logger.enableTag("Test");
    
    Serial.println("\n[Logger with CLI Example]\n");
    Serial.println("Available CLI commands:");
    Serial.println("  LOGGER_SET_LEVEL\\<0-4> - Set console log level");
    Serial.println("  LOGGER_GET_LEVEL - Show current log level");
    Serial.println("  LOGGER_SET_FILE_LEVEL\\<0-4> - Set file log level");
    Serial.println("  LOGGER_GET_FILE - Read log file");
    Serial.println("  LOGGER_CLEAR_FILE - Clear log file");
    Serial.println("  LOGGER_ENABLE_TAG\\<tag> - Enable logging for tag");
    Serial.println("  LOGGER_DISABLE_TAG\\<tag> - Disable tag");
    Serial.println("  Type '?' for help\n");
}

void loop() {
    // Process CLI commands
    Cli.update();
    
    // Log some messages periodically
    static unsigned long lastLog = 0;
    unsigned long now = millis();
    
    if (now - lastLog > 3000) {
        lastLog = now;
        
        Logger.infoTagged("Main", "Main loop running");
        Logger.infoTagged("Test", "Test message");
        Logger.warnTagged("System", "Memory: %lu bytes", (unsigned long)ESP.getFreeHeap());
    }
    
    // Flush file logs
    Logger.update();
    
    delay(100);
}
