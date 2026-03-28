# Runtime Profiles

Project Reboot now supports a runtime configuration layer so host behavior can be changed without recompiling.

## Environment variables

- `PR_PROFILE`: `br`, `stw`, `creative`, `event`
- `PR_MODE`: `gui`, `interactive`, `headless`, `host`, `dedicated`
- `PR_GUI`: `1|0`
- `PR_HEADLESS`: `1|0`
- `PR_NO_MCP`: `1|0`
- `PR_CREATIVE`: `1|0`
- `PR_EVENT`: `1|0`
- `PR_PLAYLIST`: playlist asset path
- `PR_STARTUP_DELAY`: seconds before initial travel
- `PR_BOTS`: bot count
- `PR_WARMUP_PLAYERS`: required players to warm up
- `PR_AUTO_START_PLAYERS`: required players to auto start
- `PR_DISABLE_VERBOSE_FORT_LOGS`: `1|0`

Legacy aliases with `REBOOT_` prefix are still accepted.

## Command-line options

The DLL also parses command-line options from the Fortnite process:

- `-pr-profile=stw`
- `-pr-mode=headless`
- `-pr-headless`
- `-pr-gui`
- `-pr-no-mcp=1`
- `-pr-playlist=/Game/...`
- `-pr-startup-delay=5`
- `-pr-bots=30`

Command-line options override environment variables.

## Presets

`PR_PROFILE` presets:

- `br`: default battle royale profile.
- `stw`: enables `no_mcp` and reduces verbose Fort logging by default.
- `creative`: enables creative mode.
- `event`: enables event mode.

`PR_MODE` presets:

- `headless|host|dedicated`: disables GUI and runs in host mode.
- `gui|interactive`: enables GUI mode.

## Host examples

### STW local host profile

```bash
export PR_PROFILE=stw
export PR_MODE=headless
export PR_NO_MCP=1
export PR_GUI=0
export PR_DISABLE_VERBOSE_FORT_LOGS=1
```

### BR local host profile

```bash
export PR_PROFILE=br
export PR_MODE=headless
export PR_GUI=0
export PR_BOTS=30
export PR_WARMUP_PLAYERS=1
export PR_AUTO_START_PLAYERS=1
```

## Notes for Lawin integration

When launched by the Lawin host orchestrator, set `PR_MODE=headless` and the desired `PR_PROFILE` in the launch environment. The startup log will print the resolved runtime config so it is easy to verify which profile is active.
