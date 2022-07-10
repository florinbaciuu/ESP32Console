#pragma once

#if !defined(ESP32)
#error This library depends on ESP-IDF and only works on ESP32!
#endif

#include "esp_console.h"
#include "Arduino.h"
#include "./ConsoleCommandBase.h"
#include "freertos/task.h"
#include "linenoise/linenoise.h"

namespace ESP32Console
{
    class Console
    {
    private:
        const char *prompt_ = "ESP32> ";
        const uint32_t task_priority_;
        const BaseType_t task_stack_size_;

        uint16_t max_history_len_ = 40;
        const char* history_save_path_ = nullptr;

        const size_t max_cmdline_len_;
        const size_t max_cmdline_args_;

        TaskHandle_t task_;

        static void repl_task(void *args);

        void beginCommon();

    public:
        /**
         * @brief Create a new ESP32Console with the default parameters
         */
        Console(const uint32_t task_stack_size = 4096, const BaseType_t task_priority = 2, int max_cmdline_len = 256, int max_cmdline_args = 8) : task_priority_(task_priority), task_stack_size_(task_stack_size), max_cmdline_len_(max_cmdline_len), max_cmdline_args_(max_cmdline_args){};

        ~Console()
        {
            vTaskDelete(task_);
            end();
        }

        /**
         * @brief Register the given command, using the raw ESP-IDF structure.
         *
         * @param cmd The command that should be registered
         * @return Return true, if the registration was successfull, false if not.
         */
        bool registerCommand(const esp_console_cmd_t *cmd)
        {
            log_v("Registering new command %s", cmd->command);

            auto code = esp_console_cmd_register(cmd);
            if (code != ESP_OK)
            {
                log_e("Error registering command (Reason %s)", esp_err_to_name(code));
                return false;
            }

            return true;
        }

        /**
         * @brief Register the given command
         *
         * @param cmd The command that should be registered
         * @return true If the command was registered successful.
         * @return false If the command was not registered because of an error.
         */
        bool registerCommand(const ConsoleCommandBase &cmd)
        {
            auto c = cmd.toCommandStruct();
            return registerCommand(&c);
        }

        /**
         * @brief Registers the given command
         *
         * @param command The name under which the command can be called (e.g. "ls"). Must not contain spaces.
         * @param func A pointer to the function which should be run, when this command is called
         * @param help A text shown in output of "help" command describing this command. When empty it is not shown in help.
         * @param hint A text describing the usage of the command in help output
         * @return true If the command was registered successful.
         * @return false If the command was not registered because of an error.
         */
        bool registerCommand(const char *command, esp_console_cmd_func_t func, const char *help, const char *hint = "")
        {
            const esp_console_cmd_t cmd = {
                .command = command,
                .help = help,
                .hint = hint,
                .func = func,
            };

            return registerCommand(&cmd);
        };

        void registerCoreCommands();

        void registerSystemCommands();

        void registerNetworkCommands();

        /**
         * @brief Set the command prompt. Default is "ESP32>". Must be called before begin() to work!
         *
         * @param prompt
         */
        void setPrompt(const char *prompt) { prompt_ = prompt; };

        void setHistoryMaxLength(uint16_t max_length)
        {
            max_history_len_ = max_length;
            linenoiseHistorySetMaxLen(max_length);
        }

        /**
         * @brief Enable saving of command history, which makes history persistent over resets. SPIFF need to be enabled, or you need to pass the filename to use. Must be called before begin() to work!
         *
         * @param history_save_path The file which will be used to save command history
         */
        void enablePersistentHistory(const char *history_save_path = "/spiffs/.history.txt") { history_save_path_ = history_save_path; };

        void begin(int baud, int rxPin = -1, int txPin = -1, uint8_t channel = 0);

        void end();
    };
};