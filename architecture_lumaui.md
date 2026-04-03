# LumaUI Architecture

Status: Draft  
Codename: `LumaUI`  
Related Mode: `LumaStudio`  
Scope: Fullscreen `sm64dx` frontend, in-game UI replacement plan, and adjacent creator-mode architecture  
Audience: Project owner and implementation agent  
Rule: If future UI work conflicts with this document, either update this document first or do not ship the change.

Related design specs:

- [SM64DX Fresh Start Architecture](/G:/AIExperiments/sm64dx/architecture.md)
- [LumaUI Phase Spec](/G:/AIExperiments/sm64dx/lumaui_spec.md)
- [MoonOS And Pause Screen Product Spec](/G:/AIExperiments/sm64dx/docs/moonos-product-spec.md)

## 1. Decision

`LumaUI` is the codename for the new `sm64dx` user interface system.

`LumaStudio` is the codename for the separate creator and machinima mode that will live alongside `LumaUI`.

`LumaUI` replaces `djui` as the main player-facing UI architecture for:

- title flow
- save select
- MoonOS
- pause
- options
- extras
- future frontend scenes

`LumaStudio` is a separate mode, not a skin for the normal frontend.

It exists for:

- machinima creation
- free camera and shot blocking
- actor posing and staging
- timeline editing
- screenshot and video capture workflows
- creator-facing inspection and control panels

`djui` may remain in the repository temporarily as implementation baggage while migration is in progress, but it is no longer allowed to own or render shipped player-facing screens.

Rules:

- no `djui` fallback UI for shipped flows
- no mixed `djui` plus `LumaUI` ownership for the same screen
- once a surface moves to `LumaUI`, `djui` must stop drawing it entirely
- `djui` may stay only until it is removed, but it is no longer part of the product architecture

## 2. Product Goal

`LumaUI` must make `sm64dx` feel like its own game, not a repainted `sm64coopdx` menu stack.

That means:

- fullscreen scene-based UI instead of floating overlay panels
- one coherent visual language across title, save select, MoonOS, pause, and settings
- strong support for controller, mouse, and keyboard
- live 3D preview stages where character identity matters
- data-driven screens backed by save data, MoonOS metadata, palettes, maps, and ROM hack context
- a real content browser for custom levels, campaigns, ROM hacks, and character content
- tooling that makes `sm64dx` feel like a moddable single-player platform instead of a fixed fork
- a separate editor-style mode that lets players create machinima, stage scenes, and direct gameplay like a lightweight `Saturn`-style toolset

## 3. Core Principles

- Fullscreen scenes, not generic panel stacks.
- Custom widgets, not only reused `djui` buttons and columns.
- The game renderer stays in charge. `LumaUI` integrates into it directly.
- Character identity is a first-class product feature, not a side setting.
- Save-file context must be visible in major screens.
- Every scene must support controller-first navigation, with mouse as a first-class second path.
- Style must be intentional and stable across the whole product.
- No shipped screen may be rendered by both `djui` and `LumaUI`.
- Custom levels, ROM hacks, MoonOS packs, and future content types must be first-class citizens in the UI model.
- `sm64dx` is an offline-first moddable single-player game, and the UI must reflect that everywhere.
- `LumaUI` is for shipped player-facing product UX. `LumaStudio` is for creator and machinima tooling.

## 4. Non-Goals

- Do not build a desktop-style application toolkit into `sm64dx`.
- Do not make `Dear ImGui`, `Qt`, or `RmlUi` the primary shipped frontend.
- Do not keep layering more visuals on top of legacy `djui` panels as the long-term answer.
- Do not let MoonOS remain a small options page.
- Do not keep the vanilla pause card as the primary in-game pause UX.
- Do not ship a mixed-mode `djui` fallback path for core scenes.
- Do not make players depend on emulation UX just to browse custom content or choose ROM-backed content.
- Do not force creator tooling constraints on the shipped game UI, or vice versa.

## 5. Technical Direction

`LumaUI` will be a custom UI runtime built on top of the existing game renderer and main loop.

Core shipped-game libraries:

- `SDL3` for input plumbing, controller state, text input, window events, and platform events
- `FreeType` for proper font rasterization, glyph metrics, and higher-quality typography
- `HarfBuzz` for text shaping and better international text support on top of `FreeType`
- `stb_image` for UI textures, thumbnails, preview art, and pack icons
- `Lua 5.4` for scriptable content behavior, content metadata hooks, and moddable runtime surfaces

Content and platform libraries:

- `PhysicsFS` for mounted content roots, virtual filesystem access, and clean content discovery across `resources`, `mods`, `moonos`, custom level packs, and ROM hack directories
- `miniz` or equivalent small archive support for packaged content bundles later
- `nativefiledialog-extended` or equivalent for a proper ROM and content import picker on desktop builds
- `inih` or another lightweight metadata parser for pack, campaign, and content manifests if the existing parser path is not sufficient
- `cgltf` or equivalent lightweight asset/scene manifest ingestion if imported creator content grows beyond the current simple asset layout

Creator-mode libraries for `LumaStudio`:

- `Dear ImGui` for editor panels, inspectors, property grids, and tool windows
- `ImGuizmo` for 3D transform controls, camera rigs, actor positioning, and scene staging
- an `ImSequencer`-style timeline component for shot tracks, camera cuts, keyframes, and animation timing
- `FFmpeg` or an external `ffmpeg` process for video export, image-sequence output, and capture workflows
- `nlohmann/json` or equivalent project serialization support for machinima project files if plain ini manifests become too restrictive

Deferred or optional later:

- `msdf` text rendering if sharper scalable text becomes necessary
- `Yoga` only if manual layout becomes unmanageable
- thumbnail generation and cached preview tooling for heavier content catalogs
- background asset import and preview baking if creator workflows become large enough

Not chosen as the primary architecture:

- `djui`
- `NanoVG`
- `RmlUi`
- `Dear ImGui` for shipped frontend surfaces

## 6. Runtime Layers

`LumaUI` is split into nine layers.

### 6.1 UI Core

Purpose:

- scene stack
- transitions
- focus state
- cursor state
- modal stack
- back handling
- input dispatch
- per-frame UI lifecycle

Ownership target:

- `src/pc/lumaui/lumaui_core.*`
- `src/pc/lumaui/lumaui_scene.*`
- `src/pc/lumaui/lumaui_input.*`

Rules:

- only one primary fullscreen scene is active at a time
- modals may overlay a scene, but scenes do not compose as random panel piles
- scene entry and exit must be explicit

### 6.2 UI Render

Purpose:

- draw rectangles, gradients, borders, shadows, images, icons, and text
- clipping and scissoring
- simple animation support
- common draw helpers for cards, tabs, badges, and action bars

Ownership target:

- `src/pc/lumaui/lumaui_render.*`
- `src/pc/lumaui/lumaui_font.*`
- `src/pc/lumaui/lumaui_texture.*`

Rules:

- rendering helpers do not know product logic
- all shared styling comes through theme tokens

### 6.3 Theme Layer

Purpose:

- color system
- typography selection
- spacing scale
- border radius rules
- motion presets
- scene-specific variants

Ownership target:

- `src/pc/lumaui/lumaui_theme.*`

Rules:

- `sm64dx` must have one visible style system
- title, MoonOS, pause, and save select should feel related, not random

### 6.4 Widget Layer

Purpose:

- buttons
- tile cards
- tab strips
- search field later
- scroll views
- stat rows
- info panels
- bottom action bars
- metadata badges

Ownership target:

- `src/pc/lumaui/widgets/*`

Rules:

- widgets are reusable and scene-agnostic
- scene code composes widgets, not raw draw calls everywhere

### 6.5 Scene Layer

Purpose:

- title
- save select
- MoonOS
- pause
- options
- extras
- dialogs

Ownership target:

- `src/pc/lumaui/scenes/*`

Rules:

- scenes own layout and scene-local behavior
- scenes consume data adapters instead of reaching into random globals directly

### 6.6 Data Adapter Layer

Purpose:

- save summaries
- recent save
- MoonOS catalog
- palette data
- custom map metadata
- ROM hack metadata
- course and castle progress summaries

Ownership target:

- `src/pc/lumaui/lumaui_data_*.*`

Rules:

- adapters convert game/runtime state into UI-ready structures
- adapters do not render anything

### 6.7 Preview Stage Layer

Purpose:

- live 3D model preview
- save-file character preview
- pack selection preview
- costume, palette, and gameplay-affecting preview hooks later

Ownership target:

- `src/pc/lumaui/lumaui_preview.*`

Rules:

- preview stage is not the same as the gameplay camera
- MoonOS, player customization, and possibly save select can request a preview stage
- tile previews may begin as textured thumbnails, but the primary left-side preview should be a real 3D stage

### 6.8 Content Mount And Discovery Layer

Purpose:

- mounted content roots
- custom level discovery
- ROM hack discovery
- MoonOS pack discovery
- campaign metadata
- compatibility classification
- asset and manifest normalization

Ownership target:

- `src/pc/content/*`
- `src/pc/lumaui/lumaui_content_*.*`

Rules:

- scenes do not walk arbitrary folders directly
- content categories are mounted and queried through one shared system
- built-in resources, mods, MoonOS packs, custom levels, and ROM hacks must all resolve through explicit content roots
- ROM-backed content should have desktop-friendly selection and import flows

### 6.9 Studio And Tooling Layer

Purpose:

- machinima project management
- timeline editing
- camera rig editing
- actor and prop staging
- keyframe editing
- capture and export orchestration
- editor overlays and inspectors

Ownership target:

- `src/pc/lumastudio/*`

Rules:

- `LumaStudio` is a separate mode, not a skin over `LumaUI`
- `LumaStudio` may use `Dear ImGui` and tooling-oriented widgets freely
- `LumaStudio` may share render, content, and preview services with `LumaUI`
- creator tools must not dictate shipped frontend presentation decisions

## 7. Scene Model

Each `LumaUI` scene must implement:

- `enter`
- `leave`
- `update`
- `render`
- `handle_input`
- `handle_back`
- `build_layout` or equivalent scene-local layout setup

Minimum scene contract:

```c
struct LumaUiSceneVTable {
    void (*enter)(void *state);
    void (*leave)(void *state);
    void (*update)(void *state, f32 dt);
    void (*render)(void *state);
    bool (*handle_input)(void *state, const struct LumaUiInputFrame *input);
    bool (*handle_back)(void *state);
};
```

The exact ABI can change, but the ownership rule cannot:

- core owns scene switching
- scenes own their own content

## 8. Target Scenes

### 8.1 Title Scene

Purpose:

- branding
- `Start`
- `Continue Last File`
- `Levels`
- `ROMs`
- `Options`
- `MoonOS`
- `Studio`
- `Mods`
- `Extras`

Rules:

- no `Host` / `Join`
- online-first language is removed from the main product flow
- `Studio` is the entry point for the separate creator mode

### 8.2 Save Select Scene

Purpose:

- choose file
- create file
- continue file
- open file management actions

Requirements:

- large file cards
- visible stars and completion summary
- play time
- last played info
- current character setup summary
- file management hidden behind secondary actions

### 8.3 MoonOS Scene

Purpose:

- character locker and browser

Layout target:

- left: large live character preview stage
- center/right: pack tile browser
- top: tabs and filters
- bottom: action bar

Collections:

- `Installed`
- `Native`
- `MoonOS`
- `DynOS`
- `Favorites`
- `Recent`

Detail responsibilities:

- pack metadata
- creator credit
- compatibility
- feature badges
- palette and style access
- save binding actions

### 8.4 Pause Scene

Purpose:

- resume gameplay
- course info
- options
- player
- MoonOS
- exit level

Rules:

- this is the in-game pause owner
- vanilla pause card is retired from normal product flow
- returning to gameplay must fully close the pause scene
- pause should feel like a native `sm64dx` scene, not a debug overlay

### 8.5 Level And Campaign Scene

Purpose:

- browse the main adventure
- browse custom levels
- browse campaigns and map packs
- show level or campaign metadata
- launch compatible content cleanly

Requirements:

- grid and list views
- clear compatibility labels
- author and description support
- local favorites and recent history
- active save-file context

### 8.6 ROM Selector Scene

Purpose:

- choose the active ROM-backed content source
- import `.z64` files
- explain compatibility and limits
- manage recent ROM choices

Requirements:

- desktop file picker support
- drag-and-drop support
- compatibility result messaging
- recent and pinned ROM entries
- clear distinction between vanilla assets and ROM-hack-backed content

### 8.7 Options Scene

Purpose:

- gameplay
- controls
- audio
- visuals
- accessibility
- camera

Rules:

- general options belong here, not inside MoonOS

### 8.8 Mods And Content Scene

Purpose:

- local mod visibility
- content packs
- enabled content summaries
- future import, validation, and creator tooling entry points

Rules:

- this is not a generic debug screen
- it should surface player-meaningful content information

### 8.9 Extras Scene

Purpose:

- credits
- gallery later
- sound test later
- unlocks later
- other non-core content later

### 8.10 Studio Launcher Scene

Purpose:

- launch `LumaStudio`
- browse recent machinima projects
- create a new project from the current map, custom level, or ROM-backed content
- reopen recent captures and project files

Rules:

- this scene is part of the shipped frontend only as a launcher
- the actual editor UX belongs to `LumaStudio`, not `LumaUI`

## 9. Input Model

`LumaUI` must support:

- controller-first focus navigation
- mouse hover and click
- keyboard navigation
- text input where required

Input rules:

- focusable widgets must expose directional navigation
- mouse movement must update hover without breaking controller navigation permanently
- action prompts should display relevant input hints
- pause, save select, and MoonOS must all work fully on controller

## 10. Scrolling And Overflow

Scrolling is a first-class system, not an afterthought.

Required support:

- vertical list scrolling
- tile grid scrolling
- clipped content regions
- scroll indicators
- page-based fallback where appropriate
- virtualized or incremental population later if content catalogs grow large

Rule:

- text should never be half-rendered because of undersized rows or clipped parent panels

## 11. Visual Direction

`LumaUI` should take structural inspiration from modern game frontends:

- strong title area
- clear top-level navigation
- large preview stage
- grid-based content browser
- bottom action strip
- strong selection states
- clear metadata hierarchy

But `LumaUI` must not become a literal clone of another game.

`sm64dx` style should emphasize:

- bright Nintendo-like contrast
- sharper typography than vanilla HUD text
- more deliberate spacing
- stronger preview presentation
- readable dark and mid-tone surfaces behind content
- more authored motion and transitions
- a product identity that can sit between Nintendo-like charm and moddable PC sandbox energy

## 12. Filesystem And Content Model

`sm64dx` should behave like a moddable single-player platform, which means content layout must be deliberate.

Primary content roots:

- `resources/` for built-in game content
- `mods/` for script and gameplay mods
- `moonos/` for character packs and derived content
- `levels/` or future mounted pack roots for custom maps and campaigns
- `romhacks/` for imported `.z64` content
- `projects/` for saved machinima and creator project data
- `captures/` for screenshots, image sequences, and rendered videos

Rules:

- the UI should browse content through mounted roots, not hardcoded one-off folder scans
- content manifests should provide title, author, description, preview art, tags, compatibility, and source path where possible
- local desktop players should be able to select ROM-backed content from the UI without emulator tooling
- custom level names and campaign names must override fallback internal level labels when metadata exists
- machinima projects should reference mounted content by stable ids or mounted paths instead of brittle absolute hardcoded paths where possible

## 13. Content Support Requirements

`LumaUI` must surface:

- native character choices
- MoonOS packs
- DynOS-derived packs
- custom levels
- campaigns and map packs
- local mod content summaries
- ROM-backed content choices
- palette support
- voice support
- life icon and meter support
- moveset support
- animation support
- attack support
- save-bound identity
- custom map names
- active ROM hack identity where present

`LumaStudio` must support:

- free camera tools
- shot and timeline editing
- actor and object staging
- per-shot or per-scene metadata
- capture and export workflows
- opening scenes from main adventure, custom levels, MoonOS character setups, and ROM-backed content where compatible

## 14. Files And Module Layout

Planned source layout:

| Path | Responsibility |
| --- | --- |
| `src/pc/lumaui/` | Core runtime |
| `src/pc/lumaui/widgets/` | Reusable widgets |
| `src/pc/lumaui/scenes/` | Fullscreen scenes |
| `src/pc/lumaui/theme/` | Theme tokens and style helpers if split |
| `src/pc/lumaui/data/` | Data adapters if split |
| `src/pc/content/` | Mounted content, manifests, compatibility, and discovery |
| `src/pc/lumastudio/` | Creator-mode runtime, editor panels, and capture orchestration |

Initial key files:

- `src/pc/lumaui/lumaui_core.c`
- `src/pc/lumaui/lumaui_core.h`
- `src/pc/lumaui/lumaui_render.c`
- `src/pc/lumaui/lumaui_render.h`
- `src/pc/lumaui/lumaui_theme.c`
- `src/pc/lumaui/lumaui_theme.h`
- `src/pc/lumaui/lumaui_preview.c`
- `src/pc/lumaui/lumaui_preview.h`
- `src/pc/lumaui/scenes/lumaui_scene_title.c`
- `src/pc/lumaui/scenes/lumaui_scene_save_select.c`
- `src/pc/lumaui/scenes/lumaui_scene_moonos.c`
- `src/pc/lumaui/scenes/lumaui_scene_pause.c`
- `src/pc/lumaui/scenes/lumaui_scene_levels.c`
- `src/pc/lumaui/scenes/lumaui_scene_roms.c`
- `src/pc/content/content_mount.c`
- `src/pc/content/content_mount.h`
- `src/pc/content/content_catalog.c`
- `src/pc/content/content_catalog.h`
- `src/pc/lumastudio/lumastudio_core.cpp`
- `src/pc/lumastudio/lumastudio_core.h`
- `src/pc/lumastudio/lumastudio_timeline.cpp`
- `src/pc/lumastudio/lumastudio_capture.cpp`
- `src/pc/lumastudio/lumastudio_camera.cpp`

## 15. Migration Plan

### Phase 1: Core Runtime

Deliverables:

- scene manager
- input routing
- base renderer helpers
- theme tokens
- content mount bootstrap
- one test scene

### Phase 2: Title And Save Select

Deliverables:

- title scene
- offline-first `Start` flow
- proper save cards
- continue-last-file support
- no `djui` ownership left on title or save-select flow

### Phase 3: Pause Replacement

Deliverables:

- `LumaUI` pause scene
- course info integration
- proper resume and exit flow
- full removal of the current simple pause overlay from normal gameplay UX

### Phase 4: MoonOS Migration

Deliverables:

- fullscreen character locker scene
- preview stage
- tile browser
- detail/style/gameplay/save binding flows

### Phase 5: Content Browsing

Deliverables:

- level and campaign browser
- ROM selector and importer
- custom map naming and metadata support
- mounted content catalog for custom levels, ROM hacks, and local content packs

### Phase 6: Options, Mods, And Extras

Deliverables:

- general settings scenes
- mods and content scene
- extras scenes
- cleanup of remaining non-`LumaUI` frontend entry points

### Phase 7: Studio Mode Foundation

Deliverables:

- `Studio` launcher scene
- `LumaStudio` boot path
- editor runtime shell
- free camera and basic staging tools
- project save/load format

### Phase 8: Machinima Tooling

Deliverables:

- timeline editing
- keyframes and shot structure
- actor transform tooling
- basic capture and export flow

### Phase 9: DJUI Retirement

Deliverables:

- remove `djui` ownership from shipped player-facing screens
- isolate or delete `djui` paths that can still cause mixed rendering or layout conflicts
- keep only any temporary code required for migration, with deletion as the expected end state

## 16. Success Criteria

`LumaUI` is successful when:

- the player no longer reads the product as a `djui` overlay stack
- MoonOS feels like a real character locker
- save select feels like a real start flow, not a menu afterthought
- pause feels like a designed `sm64dx` scene
- custom levels and ROM-backed content feel like built-in product features, not hacks
- the content browser makes `sm64dx` feel like a moddable single-player platform
- all core screens share one visual language
- controller and mouse navigation both feel deliberate
- UI ownership is clear in code
- no shipped screen depends on `djui` as fallback behavior
- `LumaStudio` can open a scene, stage a shot, and export usable machinima output without feeling like a debug hack

## 17. Immediate Next Step

The next implementation step after this document is:

1. create `src/pc/lumaui/`
2. stand up the `LumaUI` core runtime
3. ship one real fullscreen scene as the architecture seed
4. define the `LumaStudio` boundary before any editor UI starts landing in gameplay code

The best first seed scene is `Pause` or `MoonOS`.

`Pause` is the smaller integration surface.  
`MoonOS` is the stronger proof of the product direction.
