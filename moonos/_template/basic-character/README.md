# MoonOS Pack Template

Copy this folder to `moonos/<Your Pack Name>` and then rename the files and assets to match your pack.

Suggested structure:

```text
your-pack/
  pack.ini
  main.lua
  actors/
  assets/
  textures/
  sound/
  moveset.lua
  custom-anims.lua
  attacks.lua
```

Notes:
- `pack.ini` drives MoonOS metadata and feature flags.
- `main.lua` registers the playable character through `moonOS`.
- `actors/` and `assets/` are where raw DynOS content can live before generation.
- `textures/` can hold life icon and health meter textures.
- `sound/` can hold voice clips for `moonOS.character_add_voice(...)`.
- `moveset.lua`, `custom-anims.lua`, and `attacks.lua` are optional split files if you prefer not to keep everything in `main.lua`.
