# MoonOS And Pause Screen Product Spec

Status: Draft  
Scope: MoonOS browser/runtime direction and in-game DJUI pause replacement for `sm64dx`  
Audience: Project owner and implementation agent

## 1. Product Direction

MoonOS should not remain a small character settings page.

MoonOS should become the character platform for `sm64dx`.

That means MoonOS owns:

- character browsing
- save-bound character identity
- pack metadata and compatibility
- style and presentation choices
- gameplay-affecting character features when a pack supports them
- creator-facing pack structure and expectations

The player should think of MoonOS as:

"This is where I choose who this save file is."

## 2. Core Product Pillars

### 2.1 Character Browser First

MoonOS should behave like a proper content browser, not a plain option list.

Required behavior:

- large preview area
- grid or list pack browser
- categories and filters
- favorites and recents
- search and sorting later
- pack detail page instead of one flat panel

### 2.2 Save Identity

Each save file should own its active character setup.

The saved setup should include:

- source type: `native`, `moonos`, or `dynos`
- source id
- selected pack id
- base character
- active palette
- active costume or variant later
- voice profile later
- HUD style later

### 2.3 Unified Runtime

MoonOS should present one system to the player even if the backend is mixed.

Backend ownership:

- MoonOS owns browsing, metadata, save binding, pack selection, favorites, and recent state
- DynOS owns visual asset override and generated pack data
- Character Select owns custom character runtime features such as voices, life icons, health meter, movesets, animations, and attacks

The player should not have to care which backend a pack uses.

### 2.4 Curated Packs

MoonOS should treat packs as authored content, not random folders.

Every pack should aim to provide:

- display name
- author
- description
- tags
- compatibility label
- feature flags
- preview character
- base character

## 3. Player-Facing MoonOS Screen Spec

## 3.1 Browser Screen

This should become the main MoonOS screen and replace the current simple panel in [djui_panel_moonos.c](/G:/AIExperiments/sm64dx/src/pc/djui/djui_panel_moonos.c).

Layout:

- Left side: large live preview area
- Right side: pack browser grid or list
- Top row: collection tabs and global actions
- Bottom row: apply and secondary actions for the selected pack

Top row:

- `Installed`
- `Native`
- `MoonOS`
- `DynOS`
- `Favorites`
- `Recent`
- `Settings`

Preview panel:

- large character preview
- pack title
- author
- description
- compatibility badge
- feature badges
- base character
- save file currently targeted

Browser panel:

- grid mode for visual browsing
- list mode for information-dense browsing
- hover or selection highlight
- favorite icon
- active icon
- source icon

Primary actions:

- `Apply`
- `Favorite`
- `Details`
- `Style`
- `Gameplay`

Later actions:

- `Compare`
- `Preview Voice`
- `Open Creator Page` if pack metadata supports it

## 3.2 Pack Detail Screen

This screen should open when the player selects `Details`.

Purpose:

- show the selected pack as a full authored profile

Content:

- large preview
- description
- author and credit
- compatibility notes
- feature matrix
- base character
- palette or costume support
- source type
- install path for debugging

Action row:

- `Apply`
- `Favorite`
- `Back`

## 3.3 Style Screen

This screen should own visual presentation choices for the selected pack.

Content:

- palette presets
- costume or variant selector
- life icon preview
- health meter preview
- HUD theme preview later
- graffiti or menu art preview later

Purpose:

- make visual tuning part of MoonOS, not scattered through generic player settings

## 3.4 Gameplay Screen

This screen should exist only when the selected pack exposes gameplay features.

Content:

- moveset enabled state
- attack profile
- animation profile
- warnings for gameplay changes
- per-save gameplay toggle where supported

Purpose:

- separate "how the character looks" from "how the character plays"

## 3.5 Save Binding Screen

This is a small supporting screen or panel, not a main page.

Content:

- `Apply To Current Save`
- `Set As Global Default`
- `Reset Save To Default`

Default rule:

- current save owns the active pack

## 3.6 MoonOS Settings Screen

This is not general game settings.

MoonOS settings should only include:

- browser mode: grid or list
- show hidden template packs: off by default
- sort mode
- show unsupported packs
- preview behavior

## 4. Content Rules For Packs

## 4.1 Supported Folder Shapes

MoonOS should support packs anywhere under:

- `moonos/packs/<pack>`
- `moonos/packs/<category>/<pack>`
- deeper nested folders if needed

Reserved convention:

- folders starting with `_` are ignored by the in-game browser and loaders

This is where template, scratch, or archive content can live.

## 4.2 Template Pack

The starter template should remain under:

[main.lua](/G:/AIExperiments/sm64dx/moonos/_template/basic-character/main.lua)  
[pack.ini](/G:/AIExperiments/sm64dx/moonos/_template/basic-character/pack.ini)  
[README.md](/G:/AIExperiments/sm64dx/moonos/_template/basic-character/README.md)

Usage:

- copy `_template/basic-character`
- rename it into a live pack folder
- replace metadata and assets
- add optional voice, moveset, animation, and attack files

## 4.3 Pack Features

MoonOS should continue to recognize and surface these features:

- Lua runtime logic
- DynOS visuals
- voices
- life icon
- health meter
- moveset
- animations
- attacks

Future feature flags:

- custom HUD
- menu music
- pause theme
- star icon
- save-card art

## 5. Pause Screen Revamp Spec

## 5.1 Current Problem

The in-game pause flow is still owned by the vanilla renderer in [ingame_menu.c](/G:/AIExperiments/sm64dx/src/game/ingame_menu.c).

Current behavior:

- the standard course pause card still renders
- DJUI pause exists, but it is only opened through `R_TRIG`
- this makes the DJUI pause feel like a side panel, not the real pause screen

Relevant current files:

- [ingame_menu.c](/G:/AIExperiments/sm64dx/src/game/ingame_menu.c)
- [djui_panel_pause.c](/G:/AIExperiments/sm64dx/src/pc/djui/djui_panel_pause.c)
- [djui_panel_course_info.c](/G:/AIExperiments/sm64dx/src/pc/djui/djui_panel_course_info.c)

## 5.2 Target Pause Product

The pause menu should become a DJUI-owned in-game hub.

It should feel like a remaster pause screen, not a vanilla overlay.

Primary actions:

- `Resume`
- `Course Info`
- `Options`
- `Player`
- `MoonOS`
- `Exit Level`

Optional later actions:

- `Restart`
- `Warp To Castle`
- `Photo Mode`
- `Challenge Info`

## 5.3 Pause Layout

Recommended layout:

- left rail: primary pause actions
- center panel: course or castle summary
- right panel: live info cards or contextual widgets

Left rail:

- big readable buttons
- keyboard and controller friendly
- current selection highlight

Center panel:

- course name
- stars collected in current course
- 100-coin star state
- red coin state if relevant
- act name or selected objective
- save file summary

Right panel:

- active MoonOS pack
- play time
- current character preview
- optional quick toggles later

## 5.4 Pause Behavior Rules

Pause should behave differently based on context.

In a course:

- show course info
- allow exit level
- allow restart later if desired

In castle or hub levels:

- show castle progress instead of course progress
- hide invalid actions like `Exit Level` if they do not make sense

During cutscenes or locked states:

- either block pause or open a limited pause panel

## 5.5 Technical Replacement Plan

The vanilla pause render should stop being the primary UI.

Implementation target:

1. Keep game-paused state from the engine.
2. Automatically open DJUI pause when pause mode begins.
3. Stop drawing the vanilla pause course/castle box once DJUI owns the pause flow.
4. Let DJUI panels handle navigation and submenu stack.
5. Keep engine transitions only for actual exit or resume actions.

Concrete file direction:

- [ingame_menu.c](/G:/AIExperiments/sm64dx/src/game/ingame_menu.c)
  Replace the current `R_TRIG` side-door behavior.
  When pause mode is entered and `gDjuiPanelPauseCreated` is false, create the DJUI pause panel automatically.
  Once DJUI pause is active, skip the legacy pause card rendering instead of drawing both systems.

- [djui_panel_pause.c](/G:/AIExperiments/sm64dx/src/pc/djui/djui_panel_pause.c)
  Expand from the current simple button list into a real pause hub panel with contextual sections.

- [djui_panel_course_info.c](/G:/AIExperiments/sm64dx/src/pc/djui/djui_panel_course_info.c)
  Reuse as a subpanel, but it should feel like one branch of pause, not a detached debug page.

- [djui_panel_player.c](/G:/AIExperiments/sm64dx/src/pc/djui/djui_panel_player.c)
  Keep player name and palette work here, but let MoonOS remain the owner of character identity.

- [djui_panel_moonos.c](/G:/AIExperiments/sm64dx/src/pc/djui/djui_panel_moonos.c)
  Add a pause-entry path so MoonOS can open directly from the pause menu.

## 5.6 Pause Transition Rules

Required:

- pause opens immediately into DJUI
- resume closes DJUI and returns to gameplay
- exit level confirmation stays in DJUI
- no visual overlap with the vanilla pause panel

Desired:

- blurred or shaded gameplay background
- slight panel animation on open
- preserve the current course scene behind the UI

## 6. MoonOS And Pause Relationship

MoonOS should be reachable from the pause menu because character identity is part of an active run.

But the pause menu should not dump the player into a raw settings stack.

Pause-to-MoonOS should land on:

- current save file
- current active pack highlighted
- quick `Apply` and `Back` flow

Rule:

- changes that are purely visual can apply immediately
- gameplay-affecting pack changes should either warn or defer until level reload if needed

## 7. Recommended Implementation Order

### Phase 1: Pause Ownership

- make DJUI pause open automatically
- stop drawing the vanilla pause card while DJUI pause is active
- keep current actions: `Resume`, `Course Info`, `Options`, `Player`, `Exit Level`

### Phase 2: Pause Layout Revamp

- replace the basic pause button stack with a three-section layout
- add course and save summary content
- add MoonOS entry point

### Phase 3: MoonOS Browser Revamp

- replace the current list-heavy browser with preview + browser layout
- add real collection tabs and selection states
- add detail page

### Phase 4: MoonOS Style And Gameplay Screens

- add style screen
- add gameplay screen
- add save binding controls

### Phase 5: Pack Authoring Support

- keep improving the template pack
- add validation and better missing-asset warnings
- define optional metadata for previews, variants, and voice packs

## 8. Success Criteria

MoonOS is successful when:

- a player can browse characters like a proper in-game catalog
- a save file clearly owns its character identity
- native, MoonOS, and DynOS packs feel like one system
- custom character features feel supported rather than bolted on
- the pause menu is a DJUI menu first, not a vanilla pause card with a hidden extra overlay
