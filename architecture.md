# SM64DX Fresh Start Architecture

Status: Draft  
Scope: Full reset plan for rebuilding `sm64dx` from a fresh `sm64coopdx` clone  
Audience: Project owner and implementation agent  
Rule: This document is the source of truth for the reset. If code and this document disagree during the reset, fix the code or update this document before continuing.

Related design specs:

- [LumaUI Architecture](/G:/AIExperiments/sm64dx/architecture_lumaui.md)
- [MoonOS And Pause Screen Product Spec](/G:/AIExperiments/sm64dx/docs/moonos-product-spec.md)

Frontend note:

- frontend-specific architecture decisions are now owned by [LumaUI Architecture](/G:/AIExperiments/sm64dx/architecture_lumaui.md)
- the `LumaUI` document also owns the current `LumaStudio` creator-mode boundary and supporting library direction

## 1. Decision

`sm64dx` will not continue as a salvage job on top of the current mixed tree.

The project will restart from a clean upstream `sm64coopdx` repository and be ported forward in controlled phases.

The goal is not to make another custom shell for its own sake. The goal is to ship a stable offline-first `sm64dx` build with a dedicated `LumaUI` frontend, player customization, MoonOS support, custom levels, ROM-backed content support, a separate `LumaStudio` machinima mode, and a clean local workflow.

## 2. Why We Are Resetting

The current tree has too many overlapping problems:

- partial rewrites and restored backups are mixed together
- runtime behavior depends on stale build outputs and roaming data
- frontend work is being debugged inside a moving target
- crashes are landing in Lua internals instead of in a clear feature boundary
- old and new systems have been allowed to coexist for too long

That is not a good base for continuing implementation.

## 3. Core Principles

- Fresh upstream base first. No more repair-first workflow.
- One source of truth per system. No parallel legacy and replacement systems.
- Frontend pages are Lua modules. The host runtime stays in C.
- Reuse upstream rendering, input, and runtime systems where that reduces risk, but shipped UI ownership moves to `LumaUI`, not `djui`.
- Port in slices. Every phase must build and boot before the next phase starts.
- Current `sm64dx` tree becomes a donor/reference tree only.
- Never port `build/`, user config, cache files, or roaming data.

## 4. Non-Goals

- No attempt to preserve the current broken runtime exactly as-is.
- No custom shell rewrite unless upstream systems are proven insufficient.
- No big-bang copy of the old `sm64dx` tree into the new repo.
- No compatibility hacks that keep dead page implementations alive indefinitely.
- No reliance on stale `mod.cache`, roaming `sm64coopdx` config, or build leftovers.

## 5. Reset Strategy

The reset uses three repositories or trees:

| Role | Purpose |
| --- | --- |
| Fresh `sm64coopdx` clone | New implementation base |
| Current `sm64dx` tree | Donor/reference only |
| New `sm64dx` branch in fresh clone | Actual port target |

The current dirty tree is not the place where the long-term port is stabilized.

## 6. Hard Porting Rules

- Do not copy `build/`, `tmp/`, `mod.cache`, `sav/`, or roaming config files.
- Do not copy generated binaries as source.
- Do not copy old crashes or work around them with suppressions.
- Do not port a system until its ownership is defined.
- Do not keep old and new versions of the same page active at the same time.
- Do not let a page exist in both C and Lua after the Lua version is accepted.
- Do not add new logic directly to random legacy files if the target module is already known.

## 7. Product Goals

`sm64dx` is an offline-first, moddable, singleplayer-focused fork with these headline outcomes:

- local startup flow
- stable first-run experience
- safe-mode and recovery behavior
- Lua-driven frontend pages
- local save management
- player customization
- MoonOS pack browser and apply flow
- DynOS browsing where appropriate
- custom level and campaign support
- desktop-friendly ROM selector and ROM-backed content flow
- creator-facing machinima and staging mode
- mod visibility and profile management for local use
- clean branding and local filesystem separation from `sm64coopdx`

## 8. Target Architecture

The new architecture is split into six layers.

### 8.1 Platform and Bootstrap

Purpose:

- executable startup
- save/config path selection
- renderer init
- audio init
- ROM checks
- crash handler
- startup session marker and recovery logic

Ownership:

- `src/pc/pc_main.c`
- `src/pc/platform.c`
- `src/pc/configfile.*`
- `src/pc/startup_experience.*`

Rules:

- `sm64dx` must prefer its own roaming/config directory, not inherit `sm64coopdx` runtime state by default
- startup safety and config loading happen before frontend flow
- offline mode is the default and primary flow

### 8.2 Core Game Runtime

Purpose:

- main game loop
- gameplay engine
- audio
- renderer
- DynOS integration
- local player state

This layer should stay as close to upstream as possible unless a change is required for the `sm64dx` product.

### 8.3 Frontend Host Runtime in C

Purpose:

- page registry
- page stack
- transitions
- input capture
- focus and cursor
- confirmation dialogs
- toasts
- page lifecycle calls
- page API bindings into Lua

This is the host. It is not the page content.

Ownership:

- `src/pc/mxui/*.c`
- `src/pc/mxui/*.h`

Rules:

- host decides page flow
- pages decide their own content and layout
- host owns fallback behavior for missing pages or failed callbacks
- host does not own page-specific business logic

### 8.4 Frontend Pages in Lua

Purpose:

- screen-specific content
- navigation choices
- feature-specific layout
- page-local state
- feature-local interactions

Ownership:

- `mxui_pages/<page>/main.lua`
- `mxui_pages/shared.lua`

Rules:

- each page is its own Lua module
- page state is local to the page instance
- pages only talk to C through the bound `mxui` API
- pages do not reach into random globals unless a specific API does not yet exist

### 8.5 Feature Domain Systems

Purpose:

- save slot operations
- accessibility presets
- player palette editing
- MoonOS scanning and apply flow
- DynOS pack visibility
- local mod profiles

Ownership:

- `src/pc/mxui/mxui_player_customizer.*`
- `src/pc/mxui/mxui_moonos.*`
- `src/pc/mods/*`
- other targeted support modules

Rules:

- these systems expose data and actions
- they do not draw screens
- they do not own routing

### 8.6 Content and Resources

Purpose:

- language files
- palettes
- MoonOS packs
- frontend Lua pages
- branding assets

Ownership:

- `lang/`
- `palettes/`
- `moonos/`
- `mxui_pages/`
- `textures/`, `sound/`, and other normal resource paths

Rules:

- build output must copy these directories deliberately
- build output must not silently reuse stale copies

## 9. Filesystem and Resource Layout

The target layout for the new port is:

| Path | Ownership |
| --- | --- |
| `src/pc/mxui/` | Frontend host runtime and C bridge |
| `mxui_pages/` | Built-in Lua frontend pages |
| `moonos/` | MoonOS pack manifests and assets |
| `palettes/` | Player palette presets |
| `lang/` | Interface language files |
| `docs/` | Design docs, migration docs, user docs |

Expected build copies:

- `build/us_pc/mxui_pages`
- `build/us_pc/moonos`
- `build/us_pc/lang`
- `build/us_pc/palettes`

## 10. Frontend Page Model

Every frontend page is a built-in Lua module registered by name.

Expected page contract:

```lua
mxui.register_page({
    screen = "boot",

    enter = function(state)
    end,

    leave = function(state)
    end,

    render = function(state)
    end,

    footer = function(state)
    end,

    back = function(state)
        return false
    end,
})
```

State rules:

- `state` is a per-page-instance Lua table
- page state is created by the host
- page state survives while the page remains in the stack
- page state is destroyed on pop/clear

Host responsibilities:

- register page
- push/open/pop/clear
- confirm dialogs
- toast messages
- common layout helpers
- common widgets
- settings and data bridge

## 11. Frontend API Shape

The Lua API exposed to pages should stay narrow and intentional.

### 11.1 Navigation

- `mxui.open(screen, tag?)`
- `mxui.push(screen, tag?)`
- `mxui.pop()`
- `mxui.clear()`

### 11.2 Feedback

- `mxui.toast(message, frames?)`
- `mxui.open_confirm(title, message, callback?)`

### 11.3 Layout

- centered layout helper
- inset layout helper
- stacked rows
- split rows
- sections
- footer controls

### 11.4 Widgets

- button
- toggle
- select
- slider
- tabs
- preview
- future search and icon grid if needed

### 11.5 Domain Data

- settings get/set
- language list
- save slot list and actions
- mod list and profiles
- DynOS packs
- player character and palette controls
- MoonOS list/apply/clear/rescan

## 12. Page Inventory

These pages are part of the target frontend:

- `boot`
- `language`
- `first_run`
- `save_select`
- `home`
- `pause`
- `settings_hub`
- `settings_display`
- `settings_sound`
- `settings_controls`
- `settings_controls_n64`
- `settings_hotkeys`
- `settings_camera`
- `settings_free_camera`
- `settings_romhack_camera`
- `settings_accessibility`
- `settings_misc`
- `settings_menu_options`
- `mods`
- `mod_details`
- `dynos`
- `player`
- `manage_saves`
- `manage_slot`
- `info`

Rule:

- if a page is shipped, it must have exactly one active implementation
- page-specific behavior belongs to that page or its dedicated domain helper

## 13. MoonOS Architecture

MoonOS is not a normal gameplay mod.

MoonOS is a content-pack system consumed by the frontend and player-customization runtime.

### 13.1 Pack Format

Primary format:

- `moonos/<pack>/main.lua`

That manifest returns or registers pack metadata and variants.

Example responsibilities:

- pack name
- author
- pack description
- variant list
- geo or geoBin references
- optional credits and descriptions per variant

### 13.2 Runtime Rules

- MoonOS packs are scanned from `/moonos`
- packs are not loaded as ordinary user mods
- the player page is the main browser for MoonOS
- applying a MoonOS variant updates the local player model configuration
- old Character Select style packs may need a conversion path, but runtime support should target the MoonOS manifest format directly

## 14. Mod System Architecture

The mod system remains supported, but the product is local-first.

What stays:

- Lua mods
- local mod scanning
- local enable/disable
- local mod profiles
- compatibility gating

What changes:

- UI becomes local-use-first instead of online-session-first
- safe mode and recovery behavior must be obvious from the frontend
- product defaults should not assume networking

## 15. Save, Config, and Recovery Rules

- `sm64dx` gets its own save/config directory
- config writes happen only on actual change
- first-run flow must not mutate config every frame
- recovery marker is managed at session start/end
- backup save behavior is explicit and stable
- no stale `sm64coopdx` roaming state should be required for boot

## 16. Build System Rules

- resource copy rules must be explicit
- build output must include `mxui_pages`, `moonos`, `lang`, and `palettes`
- stale build directories should be cleared before copying
- the build must be runnable from the produced output without donor-tree assumptions

## 17. Phase Plan

This port will be executed in phases. No phase is complete until it builds and smoke-boots.

### Phase 0: Clean Base

Deliverables:

- fresh `sm64coopdx` clone
- new `sm64dx` branch
- this architecture doc committed
- current repo marked as donor/reference only

Exit criteria:

- upstream project builds cleanly
- no `sm64dx` logic has been ported yet

### Phase 1: Product Identity

Deliverables:

- app name, title strings, version labels, branding assets
- dedicated `sm64dx` config/save path
- offline-first messaging in README and product copy

Exit criteria:

- build launches with `sm64dx` identity
- no roaming dependency on `sm64coopdx` folder

### Phase 2: Offline Runtime Split

Deliverables:

- offline-first startup flow
- safe-mode and recovery behavior
- local session boot path
- removal or neutralization of network-first assumptions from the default flow

Exit criteria:

- boot reaches menu without requiring network flow
- save/config/recovery path works locally

### Phase 3: Frontend Host Runtime

Deliverables:

- C-side frontend host
- Lua page registry
- Lua page lifecycle
- navigation and widget bindings
- fallback behavior for page errors

Exit criteria:

- host can load built-in Lua pages
- missing page errors are safe and visible
- no duplicate active page implementation path

### Phase 4: Core Frontend Pages

Deliverables:

- `boot`
- `language`
- `first_run`
- `save_select`
- `home`
- `pause`

Exit criteria:

- player can boot, choose language, finish first run, select a save, and start local play

### Phase 5: Settings Suite

Deliverables:

- settings hub
- display, sound, controls, camera, accessibility, misc, menu options pages
- settings bridge hardened to only save on change

Exit criteria:

- settings pages are stable
- config only saves on actual mutation

### Phase 6: Player and MoonOS

Deliverables:

- player page
- palette editor
- preset save/apply/reset
- MoonOS scanner, manifest reader, apply/clear flow

Exit criteria:

- local player model and palette can be changed from the frontend
- MoonOS packs in `/moonos` are detected and applied reliably

### Phase 7: Mods and DynOS

Deliverables:

- mods browser
- mod detail page
- local mod profile operations
- DynOS page integration

Exit criteria:

- local mod browsing and profile actions work in offline flow
- DynOS visibility is stable

### Phase 8: Save Management and Recovery Polish

Deliverables:

- save management pages
- recovery messaging
- safe-mode clarity
- cleanup of old startup assumptions

Exit criteria:

- local save operations are stable
- safe-mode and recovery are understandable and testable

### Phase 9: Stabilization and Release Prep

Deliverables:

- regression pass
- asset audit
- docs pass
- release checklist

Exit criteria:

- no known frontend boot crashes
- no stale resource-copy issues
- release build reproducible from clean clone

## 18. Verification Rules

Every phase must pass the following as appropriate:

- clean build
- boot to frontend
- idle on boot page without crash
- navigate relevant pages without crash
- mutate settings and verify config writes only once per change
- relaunch and verify persistence

For release readiness:

- run from clean build output
- run from clean roaming directory
- run with first-run flow enabled
- run with existing save/config present

## 19. Immediate Next Steps

The next actions after this document are:

1. Clone a fresh `sm64coopdx` repo into a new workspace.
2. Create a new `sm64dx` branch there.
3. Copy this `architecture.md` into that clean workspace if this current repo is not the new base.
4. Begin Phase 0 and Phase 1 only.
5. Do not port frontend code before the clean base and product identity are established.

## 20. Final Rule

If a change does not clearly fit one phase, one owner, and one target module, it should not be merged yet.
