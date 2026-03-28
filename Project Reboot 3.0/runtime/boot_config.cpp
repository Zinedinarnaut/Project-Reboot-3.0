#include "boot_config.h"

#include <Windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

#include "../globals.h"
#include "../log.h"

extern int SecondsUntilTravel;
extern int AmountOfBotsToSpawn;
extern int WarmupRequiredPlayerCount;
extern int NumRequiredPlayersToStart;

namespace
{
using OptionalBool = std::optional<bool>;
using OptionalInt = std::optional<int>;
using OptionalString = std::optional<std::string>;

struct BootOverrides
{
    OptionalString profile;
    OptionalString mode;

    OptionalBool headless;
    OptionalBool gui;
    OptionalBool disableVerboseFortLogs;

    OptionalBool noMcp;
    OptionalBool creative;
    OptionalBool playEvent;

    OptionalInt startupDelaySeconds;
    OptionalInt botCount;
    OptionalInt warmupRequiredPlayers;
    OptionalInt autoStartPlayerCount;

    OptionalString playlistName;

    void mergeFrom(const BootOverrides& other)
    {
        if (other.profile) profile = other.profile;
        if (other.mode) mode = other.mode;

        if (other.headless.has_value()) headless = other.headless;
        if (other.gui.has_value()) gui = other.gui;
        if (other.disableVerboseFortLogs.has_value()) disableVerboseFortLogs = other.disableVerboseFortLogs;

        if (other.noMcp.has_value()) noMcp = other.noMcp;
        if (other.creative.has_value()) creative = other.creative;
        if (other.playEvent.has_value()) playEvent = other.playEvent;

        if (other.startupDelaySeconds.has_value()) startupDelaySeconds = other.startupDelaySeconds;
        if (other.botCount.has_value()) botCount = other.botCount;
        if (other.warmupRequiredPlayers.has_value()) warmupRequiredPlayers = other.warmupRequiredPlayers;
        if (other.autoStartPlayerCount.has_value()) autoStartPlayerCount = other.autoStartPlayerCount;

        if (other.playlistName) playlistName = other.playlistName;
    }
};

std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string trim(const std::string& value)
{
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
    {
        ++start;
    }

    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
    {
        --end;
    }

    return value.substr(start, end - start);
}

OptionalString readEnvValue(const char* key)
{
    const DWORD needed = GetEnvironmentVariableA(key, nullptr, 0);
    if (needed == 0)
    {
        return std::nullopt;
    }

    std::string value;
    value.resize(needed - 1);

    const DWORD copied = GetEnvironmentVariableA(key, value.data(), needed);
    if (copied == 0)
    {
        return std::nullopt;
    }

    return trim(value);
}

OptionalString readEnvAlias(std::initializer_list<const char*> keys)
{
    for (const char* key : keys)
    {
        if (const auto value = readEnvValue(key); value.has_value())
        {
            return value;
        }
    }

    return std::nullopt;
}

OptionalBool parseBool(const std::string& raw)
{
    const auto normalized = toLower(trim(raw));

    if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on")
    {
        return true;
    }

    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off")
    {
        return false;
    }

    return std::nullopt;
}

OptionalInt parseInt(const std::string& raw)
{
    const auto cleaned = trim(raw);

    try
    {
        size_t parsed = 0;
        const int value = std::stoi(cleaned, &parsed, 10);
        if (parsed == cleaned.size())
        {
            return value;
        }
    }
    catch (...)
    {
    }

    return std::nullopt;
}

int clampInt(int value, int min, int max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

std::string wideToUtf8(const std::wstring& value)
{
    if (value.empty())
    {
        return {};
    }

    const int needed = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (needed <= 0)
    {
        return {};
    }

    std::string output(needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), output.data(), needed, nullptr, nullptr);
    return output;
}

OptionalString parseArgValue(const std::string& arg, const std::string& key)
{
    const auto normalized = toLower(arg);

    const std::vector<std::string> prefixes = {
        "--" + key + "=",
        "-" + key + "=",
        "/" + key + "="
    };

    for (const auto& prefix : prefixes)
    {
        if (normalized.rfind(prefix, 0) == 0)
        {
            return arg.substr(prefix.size());
        }
    }

    return std::nullopt;
}

bool hasArgFlag(const std::string& arg, const std::string& key)
{
    const auto normalized = toLower(arg);
    return normalized == "--" + key || normalized == "-" + key || normalized == "/" + key;
}

BootOverrides readEnvOverrides()
{
    BootOverrides overrides;

    if (const auto profile = readEnvAlias({ "PR_PROFILE", "REBOOT_PROFILE" }))
    {
        overrides.profile = profile;
    }

    if (const auto mode = readEnvAlias({ "PR_MODE", "REBOOT_MODE" }))
    {
        overrides.mode = mode;
    }

    if (const auto value = readEnvAlias({ "PR_HEADLESS", "REBOOT_HEADLESS" }); value)
    {
        overrides.headless = parseBool(*value);
    }

    if (const auto value = readEnvAlias({ "PR_GUI", "REBOOT_GUI" }); value)
    {
        overrides.gui = parseBool(*value);
    }

    if (const auto value = readEnvAlias({ "PR_DISABLE_VERBOSE_FORT_LOGS", "REBOOT_DISABLE_VERBOSE_FORT_LOGS" }); value)
    {
        overrides.disableVerboseFortLogs = parseBool(*value);
    }

    if (const auto value = readEnvAlias({ "PR_NO_MCP", "REBOOT_NO_MCP" }); value)
    {
        overrides.noMcp = parseBool(*value);
    }

    if (const auto value = readEnvAlias({ "PR_CREATIVE", "REBOOT_CREATIVE" }); value)
    {
        overrides.creative = parseBool(*value);
    }

    if (const auto value = readEnvAlias({ "PR_EVENT", "REBOOT_EVENT" }); value)
    {
        overrides.playEvent = parseBool(*value);
    }

    if (const auto value = readEnvAlias({ "PR_STARTUP_DELAY", "REBOOT_STARTUP_DELAY" }); value)
    {
        overrides.startupDelaySeconds = parseInt(*value);
    }

    if (const auto value = readEnvAlias({ "PR_BOTS", "REBOOT_BOTS" }); value)
    {
        overrides.botCount = parseInt(*value);
    }

    if (const auto value = readEnvAlias({ "PR_WARMUP_PLAYERS", "REBOOT_WARMUP_PLAYERS" }); value)
    {
        overrides.warmupRequiredPlayers = parseInt(*value);
    }

    if (const auto value = readEnvAlias({ "PR_AUTO_START_PLAYERS", "REBOOT_AUTO_START_PLAYERS" }); value)
    {
        overrides.autoStartPlayerCount = parseInt(*value);
    }

    if (const auto value = readEnvAlias({ "PR_PLAYLIST", "REBOOT_PLAYLIST" }); value)
    {
        overrides.playlistName = trim(*value);
    }

    return overrides;
}

BootOverrides readCommandLineOverrides()
{
    BootOverrides overrides;

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
    {
        return overrides;
    }

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = wideToUtf8(argv[i]);
        if (arg.empty())
        {
            continue;
        }

        if (hasArgFlag(arg, "pr-headless"))
        {
            overrides.headless = true;
            continue;
        }

        if (hasArgFlag(arg, "pr-gui"))
        {
            overrides.gui = true;
            continue;
        }

        if (hasArgFlag(arg, "pr-no-mcp"))
        {
            overrides.noMcp = true;
            continue;
        }

        if (hasArgFlag(arg, "pr-creative"))
        {
            overrides.creative = true;
            continue;
        }

        if (hasArgFlag(arg, "pr-event"))
        {
            overrides.playEvent = true;
            continue;
        }

        if (hasArgFlag(arg, "pr-disable-verbose-fort-logs"))
        {
            overrides.disableVerboseFortLogs = true;
            continue;
        }

        if (const auto value = parseArgValue(arg, "pr-profile"))
        {
            overrides.profile = trim(*value);
            continue;
        }

        if (const auto value = parseArgValue(arg, "pr-mode"))
        {
            overrides.mode = trim(*value);
            continue;
        }

        if (const auto value = parseArgValue(arg, "pr-headless"))
        {
            overrides.headless = parseBool(*value);
            continue;
        }

        if (const auto value = parseArgValue(arg, "pr-gui"))
        {
            overrides.gui = parseBool(*value);
            continue;
        }

        if (const auto value = parseArgValue(arg, "pr-no-mcp"))
        {
            overrides.noMcp = parseBool(*value);
            continue;
        }

        if (const auto value = parseArgValue(arg, "pr-creative"))
        {
            overrides.creative = parseBool(*value);
            continue;
        }

        if (const auto value = parseArgValue(arg, "pr-event"))
        {
            overrides.playEvent = parseBool(*value);
            continue;
        }

        if (const auto value = parseArgValue(arg, "pr-disable-verbose-fort-logs"))
        {
            overrides.disableVerboseFortLogs = parseBool(*value);
            continue;
        }

        if (const auto value = parseArgValue(arg, "pr-startup-delay"))
        {
            overrides.startupDelaySeconds = parseInt(*value);
            continue;
        }

        if (const auto value = parseArgValue(arg, "pr-bots"))
        {
            overrides.botCount = parseInt(*value);
            continue;
        }

        if (const auto value = parseArgValue(arg, "pr-warmup-players"))
        {
            overrides.warmupRequiredPlayers = parseInt(*value);
            continue;
        }

        if (const auto value = parseArgValue(arg, "pr-auto-start-players"))
        {
            overrides.autoStartPlayerCount = parseInt(*value);
            continue;
        }

        if (const auto value = parseArgValue(arg, "pr-playlist"))
        {
            overrides.playlistName = trim(*value);
            continue;
        }
    }

    LocalFree(argv);

    return overrides;
}

void applyProfilePreset(Reboot::Runtime::BootConfig& config)
{
    const auto profile = toLower(config.profile);

    if (profile == "stw" || profile == "save_the_world" || profile == "save-the-world")
    {
        config.noMcp = true;
        config.disableVerboseFortLogs = true;
    }
    else if (profile == "creative")
    {
        config.creative = true;
    }
    else if (profile == "event")
    {
        config.playEvent = true;
    }
}

void applyModePreset(Reboot::Runtime::BootConfig& config)
{
    const auto mode = toLower(config.mode);

    if (mode == "host" || mode == "dedicated" || mode == "headless")
    {
        config.headless = true;
        config.enableGui = false;
        config.disableVerboseFortLogs = true;
    }
    else if (mode == "gui" || mode == "interactive")
    {
        config.headless = false;
        config.enableGui = true;
    }
}

void applyExplicitOverrides(const BootOverrides& overrides, Reboot::Runtime::BootConfig& config)
{
    if (overrides.headless.has_value())
    {
        config.headless = *overrides.headless;
        if (*overrides.headless)
        {
            config.enableGui = false;
        }
    }

    if (overrides.gui.has_value())
    {
        config.enableGui = *overrides.gui;
        if (*overrides.gui)
        {
            config.headless = false;
        }
    }

    if (overrides.disableVerboseFortLogs.has_value())
    {
        config.disableVerboseFortLogs = *overrides.disableVerboseFortLogs;
    }

    if (overrides.noMcp.has_value())
    {
        config.noMcp = *overrides.noMcp;
    }

    if (overrides.creative.has_value())
    {
        config.creative = *overrides.creative;
    }

    if (overrides.playEvent.has_value())
    {
        config.playEvent = *overrides.playEvent;
    }

    if (overrides.startupDelaySeconds.has_value())
    {
        config.startupDelaySeconds = clampInt(*overrides.startupDelaySeconds, 0, 240);
    }

    if (overrides.botCount.has_value())
    {
        config.botCount = clampInt(*overrides.botCount, 0, 200);
    }

    if (overrides.warmupRequiredPlayers.has_value())
    {
        config.warmupRequiredPlayers = clampInt(*overrides.warmupRequiredPlayers, 1, 100);
    }

    if (overrides.autoStartPlayerCount.has_value())
    {
        config.autoStartPlayerCount = clampInt(*overrides.autoStartPlayerCount, 1, 100);
    }

    if (overrides.playlistName && !overrides.playlistName->empty())
    {
        config.playlistName = *overrides.playlistName;
        config.hasPlaylistOverride = true;
    }
}

Reboot::Runtime::BootConfig gConfig;
bool gConfigInitialized = false;

const char* yesNo(bool value)
{
    return value ? "yes" : "no";
}

void initializeConfig()
{
    gConfig = Reboot::Runtime::BootConfig{};

    gConfig.profile = "br";
    gConfig.mode = "gui";

    gConfig.headless = Globals::bHeadlessMode;
    gConfig.enableGui = Globals::bEnableGui;
    gConfig.disableVerboseFortLogs = Globals::bDisableVerboseFortLogs;

    gConfig.noMcp = Globals::bNoMCP;
    gConfig.creative = Globals::bCreative;
    gConfig.playEvent = Globals::bGoingToPlayEvent;

    gConfig.startupDelaySeconds = clampInt(SecondsUntilTravel, 0, 240);
    gConfig.botCount = clampInt(AmountOfBotsToSpawn, 0, 200);
    gConfig.warmupRequiredPlayers = clampInt(WarmupRequiredPlayerCount, 1, 100);
    gConfig.autoStartPlayerCount = clampInt(NumRequiredPlayersToStart, 1, 100);

    gConfig.playlistName = PlaylistName;

    BootOverrides merged = readEnvOverrides();
    merged.mergeFrom(readCommandLineOverrides());

    if (merged.profile)
    {
        gConfig.profile = trim(*merged.profile);
    }

    if (merged.mode)
    {
        gConfig.mode = trim(*merged.mode);
    }

    applyProfilePreset(gConfig);
    applyModePreset(gConfig);
    applyExplicitOverrides(merged, gConfig);

    gConfigInitialized = true;
}
} // namespace

namespace Reboot::Runtime
{
const BootConfig& GetBootConfig()
{
    if (!gConfigInitialized)
    {
        initializeConfig();
    }

    return gConfig;
}

void ApplyBootConfig()
{
    const auto& config = GetBootConfig();

    Globals::bNoMCP = config.noMcp;
    Globals::bCreative = config.creative;
    Globals::bGoingToPlayEvent = config.playEvent;

    Globals::bHeadlessMode = config.headless;
    Globals::bEnableGui = config.enableGui;
    Globals::bDisableVerboseFortLogs = config.disableVerboseFortLogs;

    SecondsUntilTravel = config.startupDelaySeconds;
    AmountOfBotsToSpawn = config.botCount;
    WarmupRequiredPlayerCount = config.warmupRequiredPlayers;
    NumRequiredPlayersToStart = config.autoStartPlayerCount;

    if (config.hasPlaylistOverride)
    {
        PlaylistName = config.playlistName;
    }

    LOG_INFO(LogInit, "Runtime profile '{}' mode '{}'.", config.profile, config.mode);
    LOG_INFO(LogInit, "Runtime switches: headless={} gui={} no_mcp={} creative={} event={}.",
        yesNo(config.headless),
        yesNo(config.enableGui),
        yesNo(config.noMcp),
        yesNo(config.creative),
        yesNo(config.playEvent));
    LOG_INFO(LogInit, "Runtime tuning: delay={}s bots={} warmup_players={} auto_start_players={} verbose_fort_logs={}.",
        config.startupDelaySeconds,
        config.botCount,
        config.warmupRequiredPlayers,
        config.autoStartPlayerCount,
        yesNo(!config.disableVerboseFortLogs));

    if (config.hasPlaylistOverride)
    {
        LOG_INFO(LogPlaylist, "Runtime playlist override: {}", config.playlistName);
    }
}
}
