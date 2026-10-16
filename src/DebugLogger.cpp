/**
 * @file DebugLogger.cpp
 * @brief Implementation of advanced logging system for ESP32
 * @author Felipe Dadison
 * @version 1.1.0
 * @date 2026-10-16
 *
 * Implements multi-level console and file logging with ANSI colors,
 * tag filtering, and timestamp support.
 */

#include "DebugLogger.h"
#include <time.h>
#include <algorithm>

namespace {

    constexpr time_t MIN_VALID_EPOCH = 1600000000;

    bool tagAllowed(const std::set<String>& tags, const char* tag) {
        if (!tag || !*tag) return true;
        if (tags.empty()) return true;
        return tags.find(String(tag)) != tags.end();
    }

    void formatTimestamp(char* buf, size_t size) {
        time_t now = time(nullptr);
        if (now >= MIN_VALID_EPOCH) {
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            strftime(buf, size, "%Y-%m-%d %H:%M:%S", &timeinfo);
        } else {
            unsigned long ms = millis();
            snprintf(buf, size, "+%lu.%03lus", ms / 1000, ms % 1000);
        }
    }

    std::vector<String> readLines(FS* fs, const String& path) {
        std::vector<String> lines;
        File file = fs->open(path, FILE_READ);
        if (!file) return lines;

        while (file.available()) {
            String line = file.readStringUntil('\n');
            while (line.endsWith("\r")) line.remove(line.length() - 1);
            lines.push_back(line);
        }
        file.close();
        return lines;
    }
}

DebugLogger::DebugLogger() {
    _mutex = xSemaphoreCreateRecursiveMutex();
    _fileMutex = xSemaphoreCreateRecursiveMutex();
}

DebugLogger::~DebugLogger() {
    if (_mutex) vSemaphoreDelete(_mutex);
    if (_fileMutex) vSemaphoreDelete(_fileMutex);
}

void DebugLogger::addOutput(Print* output) {
    if (!output) return;
    lock();
    if (std::find(_outputs.begin(), _outputs.end(), output) == _outputs.end()) {
        _outputs.push_back(output);
    }
    unlock();
}

void DebugLogger::removeOutput(Print* output) {
    lock();
    _outputs.erase(std::remove(_outputs.begin(), _outputs.end(), output), _outputs.end());
    unlock();
}

void DebugLogger::clearOutputs() {
    lock();
    _outputs.clear();
    unlock();
}

void DebugLogger::setLevel(LogLevel level) {
    _currentLevel = level;
}

LogLevel DebugLogger::getLevel() const {
    return _currentLevel;
}

void DebugLogger::setColors(bool enabled) {
    _colorsEnabled = enabled;
}

void DebugLogger::enableTag(const String& tag) {
    lock();
    _enabledTags.insert(tag);
    unlock();
}

void DebugLogger::disableTag(const String& tag) {
    lock();
    _enabledTags.erase(tag);
    unlock();
}

void DebugLogger::clearTags() {
    lock();
    _enabledTags.clear();
    unlock();
}

bool DebugLogger::beginFileLogging(FS &fs, LogLevel level, const char* filePath, size_t maxLines) {
    lockFile();

    if (!fs.exists(filePath)) {
        File file = fs.open(filePath, FILE_WRITE);
        if (!file) {
            unlockFile();
            return false;
        }
        file.close();
    }

    size_t lineCount = readLines(&fs, filePath).size();

    lock();
    _fs = &fs;
    _logFilePath = filePath;
    _fileLogLevel = level;
    _maxFileLines = maxLines > 0 ? maxLines : 1;
    _fileLineCount = lineCount;
    _fileLoggingEnabled = true;
    unlock();

    unlockFile();
    return true;
}

void DebugLogger::stopFileLogging() {
    lock();
    _fileLoggingEnabled = false;
    _pendingFileLogs.clear();
    unlock();
}

void DebugLogger::setFileLevel(LogLevel level) { _fileLogLevel = level; }
LogLevel DebugLogger::getFileLevel() const { return _fileLogLevel; }

void DebugLogger::enableFileTag(const String& tag) { lock(); _fileTags.insert(tag); unlock(); }
void DebugLogger::disableFileTag(const String& tag) { lock(); _fileTags.erase(tag); unlock(); }
void DebugLogger::clearFileTags() { lock(); _fileTags.clear(); unlock(); }

std::vector<String> DebugLogger::readAllFileLogs() const {
    std::vector<String> lines;

    lockFile();
    lock();
    bool enabled = _fileLoggingEnabled && _fs;
    FS* fs = _fs;
    String path = _logFilePath;
    size_t maxLines = _maxFileLines;
    std::vector<String> pending = _pendingFileLogs;
    unlock();

    if (enabled) {
        lines = readLines(fs, path);
        lines.insert(lines.end(), pending.begin(), pending.end());
        if (lines.size() > maxLines) {
            lines.erase(lines.begin(), lines.begin() + (lines.size() - maxLines));
        }
    }

    unlockFile();
    return lines;
}

bool DebugLogger::clearFileLogs() {
    lockFile();
    lock();
    bool enabled = _fileLoggingEnabled && _fs;
    FS* fs = _fs;
    String path = _logFilePath;
    if (enabled) _pendingFileLogs.clear();
    unlock();

    if (!enabled) {
        unlockFile();
        return false;
    }

    File file = fs->open(path, FILE_WRITE);
    bool ok = (bool)file;
    if (file) file.close();

    if (ok) {
        lock();
        _fileLineCount = 0;
        unlock();
    }

    unlockFile();
    return ok;
}

bool DebugLogger::update() {
    lockFile();
    lock();

    if (!_fileLoggingEnabled || !_fs || _pendingFileLogs.empty()) {
        unlock();
        unlockFile();
        return false;
    }

    std::vector<String> pending;
    pending.swap(_pendingFileLogs);
    FS* fs = _fs;
    String path = _logFilePath;
    size_t maxLines = _maxFileLines;
    size_t lineCount = _fileLineCount;
    unlock();

    File file = fs->open(path, FILE_APPEND);
    if (!file) {
        lock();
        pending.insert(pending.end(), _pendingFileLogs.begin(), _pendingFileLogs.end());
        _pendingFileLogs.swap(pending);
        if (_pendingFileLogs.size() > _maxFileLines) {
            _pendingFileLogs.erase(_pendingFileLogs.begin(), _pendingFileLogs.begin() + (_pendingFileLogs.size() - _maxFileLines));
        }
        unlock();
        unlockFile();
        return false;
    }

    for (const auto& line : pending) {
        file.print(line);
        file.print('\n');
    }
    file.close();
    lineCount += pending.size();

    if (lineCount > maxLines * 2) {
        lineCount = compactFile(fs, path, maxLines);
    }

    lock();
    _fileLineCount = lineCount;
    unlock();

    unlockFile();
    return true;
}

size_t DebugLogger::compactFile(FS* fs, const String& path, size_t maxLines) {
    std::vector<String> lines = readLines(fs, path);
    if (lines.size() > maxLines) {
        lines.erase(lines.begin(), lines.begin() + (lines.size() - maxLines));
    }

    File file = fs->open(path, FILE_WRITE);
    if (!file) return lines.size();

    for (const auto& line : lines) {
        file.print(line);
        file.print('\n');
    }
    file.close();
    return lines.size();
}

void DebugLogger::error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LogLevel::Error, nullptr, format, args);
    va_end(args);
}

void DebugLogger::warn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LogLevel::Warn, nullptr, format, args);
    va_end(args);
}

void DebugLogger::info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LogLevel::Info, nullptr, format, args);
    va_end(args);
}

void DebugLogger::debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LogLevel::Debug, nullptr, format, args);
    va_end(args);
}

void DebugLogger::errorTagged(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LogLevel::Error, tag, format, args);
    va_end(args);
}

void DebugLogger::warnTagged(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LogLevel::Warn, tag, format, args);
    va_end(args);
}

void DebugLogger::infoTagged(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LogLevel::Info, tag, format, args);
    va_end(args);
}

void DebugLogger::debugTagged(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vlog(LogLevel::Debug, tag, format, args);
    va_end(args);
}

void DebugLogger::infoEveryN(const char* tag, unsigned long interval_ms, const char* format, ...) {
    unsigned long now = millis();
    String key = tag ? tag : "";

    lock();
    auto it = _lastLogTimes_ms.find(key);
    bool due = (it == _lastLogTimes_ms.end()) || (now - it->second >= interval_ms);
    if (due) _lastLogTimes_ms[key] = now;
    unlock();

    if (!due) return;

    va_list args;
    va_start(args, format);
    vlog(LogLevel::Info, tag, format, args);
    va_end(args);
}

const char* DebugLogger::getLevelLabel(LogLevel level) const {
    if (!_colorsEnabled) return getLevelLabelPlain(level);

    switch (level) {
        case LogLevel::Error: return "\033[1;31m[ERROR]\033[0m"; // Red
        case LogLevel::Warn:  return "\033[1;33m[WARN]\033[0m";  // Yellow
        case LogLevel::Info:  return "\033[1;32m[INFO]\033[0m";  // Green
        case LogLevel::Debug: return "\033[1;34m[DEBUG]\033[0m"; // Blue
        default: return "[UNKNOWN]";
    }
}

const char* DebugLogger::getLevelLabelPlain(LogLevel level) const {
    switch (level) {
        case LogLevel::Error: return "[ERROR]";
        case LogLevel::Warn:  return "[WARN]";
        case LogLevel::Info:  return "[INFO]";
        case LogLevel::Debug: return "[DEBUG]";
        default: return "[UNKNOWN]";
    }
}

void DebugLogger::vlog(LogLevel level, const char* tag, const char* format, va_list args) {
    if (level == LogLevel::None || !format) return;

    lock();

    bool toConsole = level <= _currentLevel && !_outputs.empty() && tagAllowed(_enabledTags, tag);
    bool toFile = _fileLoggingEnabled && _fs && level <= _fileLogLevel && tagAllowed(_fileTags, tag);

    if (!toConsole && !toFile) {
        unlock();
        return;
    }

    char stackBuf[256];
    std::vector<char> heapBuf;
    const char* msg = stackBuf;

    va_list copy;
    va_copy(copy, args);
    int len = vsnprintf(stackBuf, sizeof(stackBuf), format, copy);
    va_end(copy);

    if (len < 0) {
        unlock();
        return;
    }

    if ((size_t)len >= sizeof(stackBuf)) {
        heapBuf.resize(len + 1);
        vsnprintf(heapBuf.data(), heapBuf.size(), format, args);
        msg = heapBuf.data();
    }

    bool hasTag = tag && *tag;

    if (toConsole) {
        const char* levelLabel = getLevelLabel(level);

        for (auto* output : _outputs) {
            if (hasTag && _colorsEnabled) output->printf("%s \033[1;36m[%s]\033[0m %s\n", levelLabel, tag, msg);
            else if (hasTag) output->printf("%s [%s] %s\n", levelLabel, tag, msg);
            else output->printf("%s %s\n", levelLabel, msg);
        }
    }

    if (toFile) {
        char timeBuf[24];
        formatTimestamp(timeBuf, sizeof(timeBuf));

        String logLine = String(timeBuf) + " " + getLevelLabelPlain(level) + " ";
        if (hasTag) logLine += "[" + String(tag) + "] ";
        logLine += msg;
        logLine.replace("\r", " ");
        logLine.replace("\n", " ");

        _pendingFileLogs.push_back(logLine);
        if (_pendingFileLogs.size() > _maxFileLines) {
            _pendingFileLogs.erase(_pendingFileLogs.begin());
        }
    }

    unlock();
}

void DebugLogger::lock() const {
    if (_mutex) xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
}

void DebugLogger::unlock() const {
    if (_mutex) xSemaphoreGiveRecursive(_mutex);
}

void DebugLogger::lockFile() const {
    if (_fileMutex) xSemaphoreTakeRecursive(_fileMutex, portMAX_DELAY);
}

void DebugLogger::unlockFile() const {
    if (_fileMutex) xSemaphoreGiveRecursive(_fileMutex);
}

DebugLogger Logger;