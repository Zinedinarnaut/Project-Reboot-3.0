# Project Reboot Repository Structure Plan

This is the incremental cleanup plan for converting the current launcher/inject flow into a dedicated-host-ready structure.

## Proposed top-level layout

```text
Project-Reboot-3.0/
  apps/
    injector-gui/          # existing GUI injector frontend
    dedicated-host/        # headless host runtime entrypoint
  core/
    matchmaking/           # queue/session adapters
    mission-runtime/       # BR/STW mission launch logic
    profile-bridge/        # profile/session bridge hooks
    telemetry/             # logs/metrics/events
  providers/
    lawin/                 # Lawin API integration
    native/                # local process/runtime provider
  configs/
    default.json
    environments/
      local.json
      staging.json
      production.json
  scripts/
    host/
      start-br.*
      start-stw.*
      stop-session.*
  docs/
    architecture.md
    operations.md
```

## Migration stages

1. Keep current GUI working and add `apps/dedicated-host` alongside it.
   Status: in progress. Runtime profile + headless host mode now exists via `runtime/boot_config.*`.
2. Move reusable code into `core/*` modules.
3. Split integrations/providers from runtime logic.
4. Move environment-specific values into `configs/environments/*`.
5. Add stable host scripts used by remote relay/orchestrator.

## Completed in this pass

- Added runtime boot configuration module:
  - `Project Reboot 3.0/runtime/boot_config.h`
  - `Project Reboot 3.0/runtime/boot_config.cpp`
- Added env and CLI driven host profiles (`br`, `stw`, `creative`, `event`) with mode presets (`gui`/`headless`).
- Added headless-safe startup path in `dllmain.cpp` by gating GUI thread creation.
- Added optional suppression for verbose Fort logging to stabilize host mode.
- Added documentation and launch helpers:
  - `docs/runtime_profiles.md`
  - `scripts/launch_headless_host.cmd`

## Why this helps

- Lets GUI and dedicated-host share one runtime core.
- Makes BR and STW launch paths explicit and testable.
- Makes remote orchestration simpler (Lawin -> relay -> dedicated host).
- Reduces breakage from ad-hoc path/layout assumptions.
