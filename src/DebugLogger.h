/**
 * @file DebugLogger.h
 * @brief Advanced logging system for ESP32 with console and file output support
 * @author Felipe Dadison
 * @version 1.0.0
 * @date 2026-03-11
 * 
 * Provides multi-level logging to console and file with tag filtering, throttling,
 * and optional CommandInterface integration for runtime log management.
 */

#pragma once

#include <Arduino.h>
#include <FS.h>
#include <vector>
#include <set>
#include <map>

#include "integrations/LoggerCLI.h"

/**
 * @enum LogLevel
 * @brief Logging severity levels
 */
enum class LogLevel : uint8_t {
    NONE
    ERROR,
    WARN,
    INFO,
    DEBUG
};

/**
 * @class DebugLogger
 * @brief Advanced logging system with multi-level filtering, file logging, and tag support
 * 
 * Features:
 * - Multiple output streams (Serial, WebSerial, etc.)
 * - Console and file logging with independent level controls
 * - Tag-based filtering for console and file outputs
 * - Throttled logging (infoEveryN)
 * - Colored ANSI output for console
 * - Automatic timestamps for file logs
 * - Optional CommandInterface integration for runtime configuration
 */
class DebugLogger {
    public:
        /**
         * @brief Construct a new DebugLogger instance
         * Initializes with INFO level and no outputs configured
         */
        DebugLogger();

        /**
         * @brief Add an output stream for console logging
         * @param output Pointer to Print-compatible stream (Serial, WebSerial, etc.)
         */
        void addOutput(Print* output);

        /**
         * @brief Remove all output streams
         */
        void clearOutputs();

        /**
         * @brief Set the console logging level
         * @param level Minimum LogLevel to display
         */
        void setLevel(LogLevel level);

        /**
         * @brief Get the current console logging level
         * @return Current LogLevel
         */
        LogLevel getLevel() const;

        /**
         * @brief Enable logging for a specific tag
         * @param tag Tag name to enable
         * @note If tags are enabled, only messages with enabled tags are logged
         */
        void enableTag(const String& tag);

        /**
         * @brief Disable logging for a specific tag
         * @param tag Tag name to disable
         */
        void disableTag(const String& tag);

        /**
         * @brief Clear all enabled tags (enables all tags by default)
         */
        void clearTags();

        /**
         * @brief Initialize file logging to the specified filesystem
         * @param fs Reference to filesystem (SPIFFS, LittleFS, etc.)
         * @param level File logging level (default WARN)
         * @param filePath Path to log file (default "/logs.txt")
         * @param maxLines Maximum lines in log file before rotation (default 20)
         * @return true if initialization successful, false otherwise
         */
        bool beginFileLogging(FS &fs, LogLevel level = LogLevel::WARN, const char* filePath = "/logs.txt", size_t maxLines = 20);

        /**
         * @brief Stop file logging and clear pending logs
         */
        void stopFileLogging();

        /**
         * @brief Enable file logging for a specific tag
         * @param tag Tag name to enable
         */
        void enableFileTag(const String& tag);

        /**
         * @brief Disable file logging for a specific tag
         * @param tag Tag name to disable
         */
        void disableFileTag(const String& tag);

        /**
         * @brief Clear all file tags (enables all tags by default)
         */
        void clearFileTags();

        /**
         * @brief Get the current file logging level
         * @return File LogLevel
         */
        LogLevel getFileLevel() const;

        /**
         * @brief Read all log lines from the log file
         * @return Vector of log line strings
         */
        std::vector<String> readAllFileLogs() const;

        /**
         * @brief Clear all contents of the log file
         * @return true if successful, false otherwise
         */
        bool clearFileLogs();

        /**
         * @brief Process pending file logs (flush to disk)
         * Call this regularly in main loop for reliable file logging
         * @return true if logs were written, false otherwise
         */
        bool update();

        /**
         * @brief Log an ERROR level message
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void error(const char* format, ...);

        /**
         * @brief Log a WARN level message
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void warn(const char* format, ...);

        /**
         * @brief Log an INFO level message
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void info(const char* format, ...);

        /**
         * @brief Log a DEBUG level message
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void debug(const char* format, ...);

        /**
         * @brief Log an ERROR level message with tag
         * @param tag Tag identifier
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void errorTagged(const char* tag, const char* format, ...);

        /**
         * @brief Log a WARN level message with tag
         * @param tag Tag identifier
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void warnTagged(const char* tag, const char* format, ...);

        /**
         * @brief Log an INFO level message with tag
         * @param tag Tag identifier
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void infoTagged(const char* tag, const char* format, ...);

        /**
         * @brief Log a DEBUG level message with tag
         * @param tag Tag identifier
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void debugTagged(const char* tag, const char* format, ...);

        /**
         * @brief Log an INFO message at most once per interval (throttled)
         * Useful for periodic status messages without spam
         * @param tag Tag identifier for this throttled message
         * @param interval_ms Minimum milliseconds between logs
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void infoEveryN(const char* tag, unsigned long interval_ms, const char* format, ...);

    private:
        FS* _fs;

        std::vector<Print*> _outputs;
        LogLevel _currentLevel;
        LogLevel _fileLogLevel;
        std::set<String> _enabledTags;
        std::map<String, unsigned long> _lastLogTimes_ms;
        std::set<String> _fileTags;
        std::vector<String> _pendingFileLogs;

        String _logFilePath;
        size_t _maxFileLines;
        bool _fileLoggingEnabled;

        /**
         * @brief Get colored ANSI label for log level (for console output)
         * @param level Log level to format
         * @return ANSI-colored level string
         */
        const char* getLevelLabel(LogLevel level);

        /**
         * @brief Get plain text label for log level (for file output)
         * @param level Log level to format
         * @return Plain text level string
         */
        const char* getLevelLabelPlain(LogLevel level);

        /**
         * @brief Core logging implementation with va_list argument handling
         * @param level Log level for this message
         * @param tag Optional message tag identifier
         * @param format Printf-style format string
         * @param args Variable argument list from variadic caller
         */
        void vlog(LogLevel level, const char* tag, const char* format, va_list args);
};

extern DebugLogger Logger;