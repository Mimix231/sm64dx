# LumaUI Phase Spec

Status: Draft  
Codename: `LumaUI`  
Related Mode: `LumaStudio`  
Scope: Phase-by-phase implementation spec for replacing shipped `djui` frontend ownership with `LumaUI` in `sm64dx`  
Audience: Project owner and implementation agent  
Rule: If implementation order changes, update this document first.

Related design specs:

- [LumaUI Architecture](/G:/AIExperiments/sm64dx/architecture_lumaui.md)
- [SM64DX Fresh Start Architecture](/G:/AIExperiments/sm64dx/architecture.md)
- [MoonOS And Pause Screen Product Spec](/G:/AIExperiments/sm64dx/docs/moonos-product-spec.md)

## 1. Decision

`LumaUI` is implemented in controlled phases, not as a single giant rewrite.

The point of this spec is to define:

- implementation order
- hard boundaries between phases
- what must be finished before the next phase starts
- what is allowed to ship temporarily
- what is explicitly forbidden during migration

`SDL3` is now the target platform/input dependency for `LumaUI`.

## 2. Product Direction

`sm64dx` should become:

- an offline-first single-player game
- a moddable content platform
- a character-driven experience through MoonOS
- a game with first-class custom levels and ROM-backed content support
- a game with a separate creator and machinima mode through `LumaStudio`

`LumaUI` is the frontend foundation for that product direction.

## 3. Hard Rules

- No shipped screen may be owned by `djui`.
- `djui` may remain in-tree temporarily, but it is not a fallback UI.
- `LumaUI` scenes are fullscreen scenes, not panel piles.
- `SDL3` support must land before heavy `LumaUI` input work starts.
- MoonOS, pause, title, and save select must all converge on one visual language.
- Temporary bridges are allowed only if they do not render legacy UI.
- Every phase must build before the next phase begins.
- Every phase must leave the repo bootable unless the phase explicitly exists to change bootstrap behavior.

## 4. Current Starting Point

At the start of this spec:

- `src/game` is already detached from direct `djui` ownership
- the active game path no longer depends on `djui` title/pause rendering
- a neutral `sm64dx_ui` bridge exists
- the old `djui` title and pause entrypoints are disconnected from shipped runtime ownership

That means the project is ready to start real `LumaUI` implementation instead of more salvage work.

## 5. Phase Structure

Each phase must define:

- goal
- implementation area
- deliverables
- exit criteria
- explicit non-goals

The next phase does not start until the previous phase reaches its exit criteria.

## 6. Phase 0: Foundation Freeze

Goal:

- lock the migration baseline before major frontend construction begins

Implementation area:

- repo structure
- architecture docs
- temporary bridge ownership
- legacy runtime disconnect verification

Deliverables:

- `architecture_lumaui.md`
- `lumaui_spec.md`
- neutral `sm64dx_ui` bridge kept buildable
- no active shipped `djui` title/pause path
- agreed folder targets for `src/pc/lumaui/`

Exit criteria:

- build succeeds
- no non-`djui` module directly owns old shipped `djui` frontend surfaces
- architecture and phase docs are accepted as the current plan

Non-goals:

- no real `LumaUI` scene yet
- no visual polish work
- no `LumaStudio` implementation

## 7. Phase 1: SDL3 Platform Migration

Goal:

- move the frontend-facing platform layer to `SDL3` so `LumaUI` is not built on a dependency you already decided to replace

Implementation area:

- window creation
- input events
- controller state
- text input
- mouse capture
- clipboard or desktop hooks if needed later

Deliverables:

- `SDL3` build path working on supported targets
- controller and keyboard input parity with the current playable build
- text input path suitable for search fields, rename dialogs, and future creator tools
- mouse lock and relative motion support under `SDL3`
- no active `SDL2` dependency in the frontend path

Exit criteria:

- game boots
- gameplay input still works
- text input works
- mouse capture works
- controller hotplug works well enough for menu work

Non-goals:

- no new frontend scenes yet
- no MoonOS redesign yet

## 8. Phase 2: LumaCore Runtime

Goal:

- create the base `LumaUI` runtime so new scenes stop depending on ad-hoc menu code

Implementation area:

- `src/pc/lumaui/lumaui_core.*`
- `src/pc/lumaui/lumaui_scene.*`
- `src/pc/lumaui/lumaui_input.*`
- `src/pc/lumaui/lumaui_render.*`
- `src/pc/lumaui/lumaui_theme.*`
- `src/pc/lumaui/lumaui_assets.*`

Deliverables:

- scene stack
- fullscreen root scene ownership
- transition hooks
- focus and cursor state
- modal layer
- action bar support
- basic widget set:
  - button
  - card
  - tab strip
  - scroll region
  - text block
  - badge
  - icon slot

Exit criteria:

- a placeholder `LumaUI` scene can take over the full screen
- input and cursor flow through `LumaUI` instead of `djui`
- scene switching works
- fullscreen rendering works without reviving legacy `djui` screens

Non-goals:

- no final title flow
- no final pause flow
- no content browser yet

## 9. Phase 3: Bootstrap And Title Flow

Goal:

- replace the startup and title flow with the first real shipped `LumaUI` experience

Implementation area:

- title scene
- splash and startup messaging
- language or first-run flow if required
- error or recovery screen hooks

Deliverables:

- `LumaUI` title scene
- `Start`
- `Continue Last File`
- `Options`
- `Quit`
- first-run routing hooks
- version footer and branding layer

Exit criteria:

- boot goes directly into `LumaUI`
- no legacy title panel renders
- title input works with controller, mouse, and keyboard
- startup path remains stable with offline boot

Non-goals:

- save select does not need to be final yet
- MoonOS does not need to be final yet

## 10. Phase 4: Save Select And Save UX

Goal:

- ship the new save-first offline frontend flow

Implementation area:

- save slot browser
- file metadata adapters
- recent save handling
- copy and erase flows
- launch routing into gameplay

Deliverables:

- fullscreen save selector
- large slot cards
- progress summary
- play time
- last played
- caps, keys, coin, and completion summary
- `Continue`, `New Game`, and `Manage` actions
- destructive actions behind secondary flow

Exit criteria:

- `Start` from title goes to `LumaUI` save select
- launching a file enters gameplay correctly
- save management works
- no legacy save panel renders

Non-goals:

- MoonOS integration can still be basic here

## 11. Phase 5: Pause Rebuild

Goal:

- replace the in-game pause flow with a real `LumaUI` pause scene

Implementation area:

- pause scene takeover
- gameplay freeze and input capture
- course info and file context panels
- pause resume and exit behavior

Deliverables:

- fullscreen pause scene
- `Resume`
- `Course Info`
- `Options`
- `Player`
- `MoonOS`
- `Exit Level`
- proper unpause handoff back to gameplay

Exit criteria:

- pressing pause opens `LumaUI`, not `djui`
- resume returns cleanly to gameplay
- pause scene does not leave legacy overlays behind
- course and map metadata are visible

Non-goals:

- MoonOS browser does not need its final locker design yet

## 12. Phase 6: MoonOS Browser 1.0

Goal:

- turn MoonOS into a first-class character browser instead of a settings submenu

Implementation area:

- MoonOS catalog adapters
- pack metadata
- favorites and recent logic
- style and gameplay tabs
- live preview stage

Deliverables:

- fullscreen MoonOS scene
- left-side large preview stage
- right-side grid or browser
- collection tabs:
  - native
  - MoonOS
  - DynOS
  - favorites
  - recent
- pack details view
- author, description, compatibility, feature badges
- apply flow
- save-bound setup persistence

Exit criteria:

- MoonOS no longer behaves like a simple options page
- per-save character setup works through `LumaUI`
- pack browsing is stable and fullscreen

Non-goals:

- no creator pack editor yet

## 13. Phase 7: MoonOS Runtime Expansion

Goal:

- make MoonOS the runtime character platform, not only a browser

Implementation area:

- character application pipeline
- save-bound setup loading
- metadata and fallback rules
- DynOS and native bridge ownership

Deliverables:

- support for:
  - voice packs
  - life icon
  - life meter cake
  - custom movesets
  - custom animations
  - custom attacks
  - variant selection
  - palette selection
  - save-bound favorites and recent
- fallback rules for missing content
- clean distinction between native base character and visual override

Exit criteria:

- MoonOS setup persists and reloads correctly
- character runtime selection is no longer spread across unrelated menus
- packs feel like curated characters, not folder entries

Non-goals:

- no in-engine pack authoring UI yet

## 14. Phase 8: Options, Extras, And Settings Convergence

Goal:

- move the rest of the shipped player-facing frontend into `LumaUI`

Implementation area:

- options scenes
- extras scenes
- accessibility and controls pages
- audio and display settings

Deliverables:

- `LumaUI` settings stack
- controls scene
- audio scene
- display scene
- camera scene
- accessibility scene
- extras or credits scene

Exit criteria:

- the main shipped frontend surfaces are now all in `LumaUI`
- old `djui` settings surfaces are no longer part of shipped flow

Non-goals:

- no full deletion of `djui` yet

## 15. Phase 9: Levels, Campaigns, And ROM Browser

Goal:

- make custom content browsing a first-class frontend feature

Implementation area:

- level catalog
- campaign metadata
- ROM import and selection
- content mount presentation

Deliverables:

- custom levels scene
- campaign browser
- map detail scene
- custom level names and metadata
- ROM selector for `.z64` content
- desktop import flow
- mounted content visibility for campaigns and ROM-backed packs

Exit criteria:

- players can browse and launch custom level content from `LumaUI`
- ROM-backed content is selectable without emulator-first workflow
- metadata is visible and stable

Non-goals:

- no creator editing tools yet

## 16. Phase 10: LumaStudio Bootstrap

Goal:

- stand up the separate creator and machinima mode

Implementation area:

- `LumaStudio` entrypoint
- editor-mode boot flow
- project serialization
- basic shot and stage tools

Deliverables:

- separate `Studio` launcher path
- editor UI shell
- basic camera rig and shot save support
- actor placement and transform handles
- timeline prototype
- image-sequence or video export prototype

Exit criteria:

- `LumaStudio` boots as a distinct mode
- scene staging and shot capture are possible
- gameplay frontend and studio frontend remain separate

Non-goals:

- no attempt to merge editor UI into shipped gameplay UI

## 17. Phase 11: Legacy Removal

Goal:

- remove the remaining dead frontend baggage once `LumaUI` and `LumaStudio` own the intended surfaces

Implementation area:

- legacy bridge cleanup
- `djui` shipped-surface removal
- dead code and old data-path cleanup

Deliverables:

- removal of legacy title and pause implementation baggage
- removal of old frontend-only `djui` dependencies no longer needed
- simplified runtime ownership map

Exit criteria:

- shipped frontend path no longer depends on `djui`
- bridge shims that only existed for migration are removed

Non-goals:

- do not remove useful low-level utility code unless it is actually dead

## 18. Forbidden Shortcuts

- Do not rebuild `LumaUI` as another three-panel `djui` shell.
- Do not keep temporary fullscreen overlays and call them final scenes.
- Do not revive legacy title or pause UI to unblock unrelated work.
- Do not put MoonOS back behind generic settings pages.
- Do not merge `LumaStudio` editor widgets into the shipped gameplay frontend.
- Do not add SDL3 work piecemeal inside random scene files.
- Do not ship mixed `SDL2` and `SDL3` frontend logic longer than necessary.

## 19. Success Definition

`LumaUI` is successful when:

- the title, save select, pause, MoonOS, and settings flows all belong to one coherent frontend
- custom levels and ROM-backed content are browsable through the game itself
- MoonOS feels like a true character platform
- `sm64dx` no longer looks or behaves like a patched-over `sm64coopdx` menu stack
- `LumaStudio` exists as a separate creator mode instead of hacked gameplay menus

## 20. Next Immediate Step

The first implementation step after this spec is:

1. Phase 1: `SDL3` migration
2. create `src/pc/lumaui/`
3. stand up a minimal fullscreen `LumaUI` root scene after `SDL3` input/platform work is stable
