/**
 * @example file_logging.cpp
 * @brief DebugLogger with file logging to filesystem
 * 
 * Features:
 * - Console and file logging simultaneously
 * - Independent log levels for console and file
 * - File size management with line limit
 * - Read and clear logs from file
 */

#include <Arduino.h>
#include <SPIFFS.h>
#include "DebugLogger.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize filesystem
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed!");
        return;
    }
    
    // Add Serial output
    Logger.addOutput(&Serial);
    
    // Console: log everything (DEBUG level)
    Logger.setLevel(LogLevel::DEBUG);
    
    // File: only log warnings and errors (WARN level)
    Logger.beginFileLogging(SPIFFS, LogLevel::WARN, "/logs.txt", 50);
    
    Serial.println("\n[File Logging Example]\n");
    Serial.println("Console level: DEBUG (all messages)");
    Serial.println("File level: WARN (only warnings and errors)\n");
}

void loop() {
    static unsigned long lastLog = 0;
    unsigned long now = millis();
    
    if (now - lastLog > 2000) {
        lastLog = now;
        
        // All messages appear on console
        Logger.debug("Debug message");
        Logger.info("Info message");
        Logger.warn("Warning message");
        Logger.error("Error message");
        
        Serial.println();
        
        // Flush file logs periodically
        Logger.update();
    }
    
    // Handle simple CLI to read logs
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "show") {
            Serial.println("\n=== Log File Contents ===");
            auto logs = Logger.readAllFileLogs();
            for (const auto& line : logs) {
                Serial.println(line);
            }
            Serial.println("========================\n");
        } else if (cmd == "clear") {
            Logger.clearFileLogs();
            Serial.println("Logs cleared.");
        }
    }
    
    delay(100);
}
