#include "Window.h"
#include "Common.h"
#include "System.h"
#include "Bar.h"
#include "AudioFlyin.h"
#include "BluetoothDevices.h"
#include "Plugin.h"
#include "Config.h"

#include <cmath>
#include <cstdio>
#include <unistd.h>

const char* audioTmpFilePath = "/tmp/gBar__audio";
const char* bluetoothTmpFilePath = "/tmp/gBar__bluetooth";

static bool tmpFileOpen = false;

void OpenAudioFlyin(Window& window, int32_t monitor, AudioFlyin::Type type)
{
    tmpFileOpen = true;
    if (access(audioTmpFilePath, F_OK) != 0)
    {
        FILE* audioTempFile = fopen(audioTmpFilePath, "w");
        AudioFlyin::Create(window, monitor, type);
        fclose(audioTempFile);
    }
    else
    {
        // Already open, close
        LOG("Audio flyin already open (/tmp/gBar__audio exists)! Exiting...");
        exit(0);
    }
}

void CloseTmpFiles(int sig)
{
    if (tmpFileOpen)
    {
        remove(audioTmpFilePath);
        remove(bluetoothTmpFilePath);
    }
    if (sig != 0)
        exit(1);
}

void PrintHelp()
{
    LOG("==============================================\n"
        "|                gBar                        |\n"
        "==============================================\n"
        "gBar, a fast status bar + widgets\n"
        "\n"
        "Basic usage: \n"
        "\tgBar [OPTIONS...] WIDGET [MONITOR]\n"
        "\n"
        "Sample usage:\n"
        "\tgBar bar 0\tOpens the status bar on monitor 0\n"
        "\tgBar audio\tOpens the audio flyin on the current monitor\n"
        "\n"
        "All options:\n"
        "\t--help/-h \tPrints this help page and exits afterwards\n"
        "\n"
        "All available widgets:\n"
        "\tbar       \tThe main status bar\n"
        "\taudio     \tAn audio volume slider flyin\n"
        "\tmic       \tA microphone volume slider flyin\n"
        "\tbluetooth \tA bluetooth connection widget\n"
        "\t[plugin]  \tTries to open and run the plugin lib[plugin].so\n");
}

int main(int argc, char** argv)
{
    std::string widget;
    int32_t monitor = -1;

    // Arg parsing
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg.size() < 1 || arg[0] != '-')
        {
            // This must be the widget selection
            widget = arg;
            if (i + 1 < argc)
            {
                std::string mon = argv[i + 1];
                if (mon.size() < 1 || mon[0] != '-')
                {
                    // Next comes the monitor
                    monitor = std::stoi(mon);
                    i += 1;
                }
                else
                {
                    // Not the monitor
                    continue;
                }
            }
        }
        else if (arg == "-h" || arg == "--help")
        {
            PrintHelp();
            return 0;
        }
        else
        {
            LOG("Warning: Unknown CLI option \"" << arg << "\"")
        }
    }
    if (widget == "")
    {
        LOG("Error: Widget to open not specified!\n");
        PrintHelp();
        return 0;
    }
    if (argc <= 1)
    {
        LOG("Error: Too little arguments\n");
        PrintHelp();
        return 0;
    }

    signal(SIGINT, CloseTmpFiles);
    System::Init();

    Window window(monitor);
    window.Init(argc, argv);
    if (widget == "bar")
    {
        Bar::Create(window, monitor);
    }
    else if (widget == "audio")
    {
        OpenAudioFlyin(window, monitor, AudioFlyin::Type::Speaker);
    }
    else if (widget == "mic")
    {
        OpenAudioFlyin(window, monitor, AudioFlyin::Type::Microphone);
    }
#ifdef WITH_BLUEZ
    else if (widget == "bluetooth")
    {
        if (RuntimeConfig::Get().hasBlueZ)
        {
            if (access(bluetoothTmpFilePath, F_OK) != 0)
            {
                tmpFileOpen = true;
                FILE* bluetoothTmpFile = fopen(bluetoothTmpFilePath, "w");
                BluetoothDevices::Create(window, monitor);
                fclose(bluetoothTmpFile);
            }
            else
            {
                // Already open, close
                LOG("Bluetooth widget already open (/tmp/gBar__bluetooth exists)! Exiting...");
                exit(0);
            }
        }
        else
        {
            LOG("Blutooth disabled, cannot open bluetooth widget!");
            exit(1);
        }
    }
#endif
    else
    {
        Plugin::LoadWidgetFromPlugin(widget, window, monitor);
    }

    window.Run();

    System::FreeResources();
    CloseTmpFiles(0);
    return 0;
}
