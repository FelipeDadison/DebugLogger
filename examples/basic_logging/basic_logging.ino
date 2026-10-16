/**
 * @example basic_logging.ino
 * @brief Basic DebugLogger example with console output
 * 
 * Features:
 * - Add output stream (Serial)
 * - Set logging level
 * - Log messages at different levels
 */

#include <Arduino.h>
#include "DebugLogger.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Add Serial as output
    Logger.addOutput(&Serial);
    
    // Set logging level (DEBUG shows all messages)
    Logger.setLevel(LogLevel::Debug);
    
    // Use plain text on terminals without ANSI colors (e.g. Arduino IDE Serial Monitor)
    // Logger.setColors(false);
    
    Serial.println("\n[Basic Logging Example]\n");
}

void loop() {
    static unsigned long lastLog = 0;
    unsigned long now = millis();
    
    if (now - lastLog > 2000) {
        lastLog = now;
        
        // Log messages at different levels
        Logger.debug("Debug message - detailed information");
        Logger.info("Info message - general information");
        Logger.warn("Warn message - warning condition");
        Logger.error("Error message - error condition");
        
        Serial.println();
    }
    
    delay(100);
}
