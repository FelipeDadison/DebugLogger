/**
 * @file DebugLogger.h
 * @brief Advanced logging system for ESP32 with console and file output support
 * @author Felipe Dadison
 * @version 1.1.0
 * @date 2026-10-16
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
#include <stdarg.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @enum LogLevel
 * @brief Logging severity levels
 */
enum class LogLevel : uint8_t {
    None,
    Error,
    Warn,
    Info,
    Debug
};

/**
 * @class DebugLogger
 * @brief Advanced logging system with multi-level filtering, file logging, and tag support
 *
 * Features:
 * - Multiple output streams (Serial, NetSerial, etc.)
 * - Console and file logging with independent level controls
 * - Tag-based filtering for console and file outputs
 * - Throttled logging (infoEveryN)
 * - Colored ANSI output for console (can be disabled)
 * - Timestamps for file logs (date and time when the clock is set, uptime otherwise)
 * - Thread-safe: can be called from any task (not from interrupts)
 * - Optional CommandInterface integration for runtime configuration (DebugLoggerCLI.h)
 */
class DebugLogger {
    public:
        /**
         * @brief Construct a new DebugLogger instance
         * Initializes with Info level and no outputs configured
         */
        DebugLogger();

        /**
         * @brief Destructor - releases internal resources
         */
        ~DebugLogger();

        DebugLogger(const DebugLogger&) = delete;
        DebugLogger& operator=(const DebugLogger&) = delete;

        /**
         * @brief Add an output stream for console logging
         * @param output Pointer to Print-compatible stream (Serial, NetSerial, etc.)
         */
        void addOutput(Print* output);

        /**
         * @brief Remove an output stream
         * @param output Pointer to the output to remove
         */
        void removeOutput(Print* output);

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
         * @brief Enable or disable ANSI colors on console output (enabled by default)
         * Disable for terminals without ANSI support, such as the Arduino IDE Serial Monitor
         * @param enabled true to use colors, false for plain text
         */
        void setColors(bool enabled);

        /**
         * @brief Enable logging for a specific tag
         * @param tag Tag name to enable
         * @note With no tags enabled, all tags are logged. Once a tag is enabled, only
         * enabled tags are logged. Untagged messages are always logged.
         */
        void enableTag(const String& tag);

        /**
         * @brief Disable logging for a specific tag
         * @param tag Tag name to disable
         */
        void disableTag(const String& tag);

        /**
         * @brief Clear all enabled tags (all tags are logged again)
         */
        void clearTags();

        /**
         * @brief Initialize file logging to the specified filesystem
         * @param fs Reference to filesystem (SPIFFS, LittleFS, etc.)
         * @param level File logging level (default Warn)
         * @param filePath Path to log file (default "/logs.txt")
         * @param maxLines Number of most recent lines to keep (default 20)
         * @return true if initialization successful, false otherwise
         */
        bool beginFileLogging(FS &fs, LogLevel level = LogLevel::Warn, const char* filePath = "/logs.txt", size_t maxLines = 20);

        /**
         * @brief Stop file logging and clear pending logs
         */
        void stopFileLogging();

        /**
         * @brief Set the file logging level
         * @param level Minimum LogLevel to write to file
         */
        void setFileLevel(LogLevel level);

        /**
         * @brief Get the current file logging level
         * @return File LogLevel
         */
        LogLevel getFileLevel() const;

        /**
         * @brief Enable file logging for a specific tag
         * @param tag Tag name to enable
         * @note Same rules as enableTag(), applied to file output
         */
        void enableFileTag(const String& tag);

        /**
         * @brief Disable file logging for a specific tag
         * @param tag Tag name to disable
         */
        void disableFileTag(const String& tag);

        /**
         * @brief Clear all file tags (all tags are logged to file again)
         */
        void clearFileTags();

        /**
         * @brief Read the most recent log lines, including lines not yet written to the file
         * @return Vector of log line strings
         */
        std::vector<String> readAllFileLogs() const;

        /**
         * @brief Clear all contents of the log file and pending logs
         * @return true if successful, false otherwise
         */
        bool clearFileLogs();

        /**
         * @brief Write pending file logs to the file
         * Call this regularly in main loop for reliable file logging
         * @return true if logs were written, false otherwise
         */
        bool update();

        /**
         * @brief Log an Error level message
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void error(const char* format, ...) __attribute__((format(printf, 2, 3)));

        /**
         * @brief Log a Warn level message
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void warn(const char* format, ...) __attribute__((format(printf, 2, 3)));

        /**
         * @brief Log an Info level message
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void info(const char* format, ...) __attribute__((format(printf, 2, 3)));

        /**
         * @brief Log a Debug level message
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void debug(const char* format, ...) __attribute__((format(printf, 2, 3)));

        /**
         * @brief Log an Error level message with tag
         * @param tag Tag identifier
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void errorTagged(const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));

        /**
         * @brief Log a Warn level message with tag
         * @param tag Tag identifier
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void warnTagged(const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));

        /**
         * @brief Log an Info level message with tag
         * @param tag Tag identifier
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void infoTagged(const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));

        /**
         * @brief Log a Debug level message with tag
         * @param tag Tag identifier
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void debugTagged(const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));

        /**
         * @brief Log an Info message at most once per interval (throttled)
         * Useful for periodic status messages without spam. The first call always logs.
         * @param tag Tag identifier, also used as the throttle key (use one tag per message)
         * @param interval_ms Minimum milliseconds between logs
         * @param format Printf-style format string
         * @param ... Variable arguments
         */
        void infoEveryN(const char* tag, unsigned long interval_ms, const char* format, ...) __attribute__((format(printf, 4, 5)));

    private:
        FS* _fs = nullptr;

        std::vector<Print*> _outputs;
        LogLevel _currentLevel = LogLevel::Info;
        LogLevel _fileLogLevel = LogLevel::Warn;
        std::set<String> _enabledTags;
        std::map<String, unsigned long> _lastLogTimes_ms;
        std::set<String> _fileTags;
        std::vector<String> _pendingFileLogs;

        String _logFilePath;
        size_t _maxFileLines = 20;
        size_t _fileLineCount = 0;
        bool _fileLoggingEnabled = false;
        bool _colorsEnabled = true;

        SemaphoreHandle_t _mutex = nullptr;
        SemaphoreHandle_t _fileMutex = nullptr;

        /**
         * @brief Get the level label for console output (colored if enabled)
         * @param level Log level to format
         * @return Level string
         */
        const char* getLevelLabel(LogLevel level) const;

        /**
         * @brief Get plain text label for log level (for file output)
         * @param level Log level to format
         * @return Plain text level string
         */
        const char* getLevelLabelPlain(LogLevel level) const;

        /**
         * @brief Core logging implementation with va_list argument handling
         * @param level Log level for this message
         * @param tag Optional message tag identifier
         * @param format Printf-style format string
         * @param args Variable argument list from variadic caller
         */
        void vlog(LogLevel level, const char* tag, const char* format, va_list args);

        /**
         * @brief Rewrite the log file keeping only the most recent lines
         * @param fs Filesystem
         * @param path Log file path
         * @param maxLines Number of lines to keep
         * @return Number of lines left in the file
         */
        size_t compactFile(FS* fs, const String& path, size_t maxLines);

        /// @brief Take the state mutex
        void lock() const;

        /// @brief Release the state mutex
        void unlock() const;

        /// @brief Take the file access mutex (always before the state mutex)
        void lockFile() const;

        /// @brief Release the file access mutex
        void unlockFile() const;
};

extern DebugLogger Logger;