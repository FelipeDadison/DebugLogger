/**
 * @file LoggerCLI.cpp
 * @brief Implementation of CommandInterface integration for DebugLogger
 * @author Felipe Dadison
 * @version 1.0.0
 * @date 2026-03-11
 * 
 * Registers runtime logger management commands with the CLI system.
 */

#if __has_include("CommandInterface.h")

#include "LoggerCLI.h"
#include "DebugLogger.h"
#include "CommandInterface.h"

void registerLoggerCLI(DebugLogger& logger) {
    Cli.registerTypedCommand<int>("LOGGER_SET_LEVEL", "Sets the logger level (0 to 4).", {{"level", "int"}}, [&](int level) {
        if (level < 0 || level > 4) Cli.printlnToAll("Invalid input range (0 to 4).");
        else {
            logger.setLevel((LogLevel)level);
            Cli.printlnToAll("Logger level: " + String(level));
        }
    });

    Cli.registerTypedCommand<>("LOGGER_GET_LEVEL", "Gets the logger level (0 to 4).", {}, [&]() {
        Cli.printlnToAll("Logger level: " + String((int)logger.getLevel()));
    });

    Cli.registerTypedCommand<>("LOGGER_GET_FILE_LEVEL", "Gets the logger file level (0 to 4).", {}, [&]() {
        Cli.printlnToAll("File logger level: " + String((int)logger.getFileLevel()));
    });

    Cli.registerTypedCommand<>("LOGGER_CLEAR_FILE", "Clears the logger file.", {}, [&]() {
        if (logger.clearFileLogs()) {
            Cli.printlnToAll("Done.");
        } else {
            Cli.printlnToAll("Fail to clear file.");
        }
    });

    Cli.registerTypedCommand<>("LOGGER_GET_FILE", "Gets the logger file.", {}, [&]() {
        auto logs = logger.readAllFileLogs();
        for (auto &line : logs) {
            Cli.printlnToAll(line);
        }
        Cli.printlnToAll("");
    });

    Cli.registerTypedCommand<String>("LOGGER_ENABLE_TAG", "Enables a logger tag.", {{"tag", "String"}}, [&](String tag) {
        logger.enableTag(tag);
        Cli.printlnToAll(tag + " enabled.");
    });

    Cli.registerTypedCommand<String>("LOGGER_DISABLE_TAG", "Disables a logger tag.", {{"tag", "String"}}, [&](String tag) {
        logger.disableTag(tag);
        Cli.printlnToAll(tag + " disabled.");
    });

    Cli.registerTypedCommand<>("LOGGER_CLEAR_TAG", "Disables all logger tags.", {}, [&]() {
        logger.clearTags();
        Cli.printlnToAll("Tags disabled.");
    });
}

#endif