/**
 * @example tagged_logging.ino
 * @brief DebugLogger with tag-based filtering
 * 
 * Features:
 * - Log messages with tags
 * - Enable/disable tags to filter output
 * - Useful for controlling which subsystems log
 */

#include <Arduino.h>
#include "DebugLogger.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Logger.addOutput(&Serial);
    Logger.setLevel(LogLevel::Debug);
    
    // Only enabled tags are shown (with no tags enabled, all tags are shown)
    // Untagged messages are always shown
    Logger.enableTag("WiFi");
    Logger.enableTag("Sensor");
    Logger.enableTag("System");
    
    Serial.println("\n[Tagged Logging Example]\n");
    Serial.println("Enabled tags: WiFi, Sensor, System");
    Serial.println("(Messages with other tags will be filtered)\n");
}

void loop() {
    static unsigned long lastLog = 0;
    unsigned long now = millis();
    
    if (now - lastLog > 3000) {
        lastLog = now;
        
        // These tags are enabled - will appear
        Logger.infoTagged("WiFi", "WiFi signal strength: -45 dBm");
        Logger.infoTagged("Sensor", "Temperature: 24.5°C, Humidity: 65%%");
        Logger.warnTagged("System", "Memory usage: 45%%");
        
        // This tag is not enabled - will not appear
        Logger.infoTagged("Database", "Query executed in 125ms");
        
        // No tag - always appears
        Logger.info("Untagged message (always shown)");
        
        Serial.println();
    }
    
    delay(100);
}
