#pragma once

#include <string>

namespace Reboot::Runtime
{
struct BootConfig
{
    std::string profile = "br";
    std::string mode = "gui";

    bool headless = false;
    bool enableGui = true;
    bool disableVerboseFortLogs = false;

    bool noMcp = false;
    bool creative = false;
    bool playEvent = false;

    int startupDelaySeconds = 5;
    int botCount = 0;
    int warmupRequiredPlayers = 1;
    int autoStartPlayerCount = 2;

    std::string playlistName;
    bool hasPlaylistOverride = false;
};

const BootConfig& GetBootConfig();
void ApplyBootConfig();
}
