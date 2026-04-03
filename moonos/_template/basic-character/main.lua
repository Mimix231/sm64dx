-- name: Template Character
-- description: Starter MoonOS character pack template.
-- pausable: true

local api = rawget(_G, "moonOS") or rawget(_G, "MoonOS") or rawget(_G, "moonos")
if type(api) ~= "table" then
    return 0
end

local modelId = E_MODEL_MARIO
local baseCharacter = CT_MARIO
local lifeIcon = gTextures.mario_head

local charNum = api.character_add(
    "Template Character",
    {
        "Replace this template with your own MoonOS character.",
        "Swap the model, icon, metadata, voices, moveset, attacks, and animations."
    },
    "Your Name",
    { r = 255, g = 255, b = 255 },
    modelId,
    baseCharacter,
    lifeIcon,
    1
)

-- Optional custom voice pack.
-- api.character_add_voice(modelId, {
--     [CHAR_SOUND_YAHOO] = "sound/template-yahoo.ogg",
--     [CHAR_SOUND_HOOHOO] = "sound/template-jump.ogg",
-- })

-- Optional custom life meter.
-- api.character_add_health_meter(charNum, {
--     label = {
--         left = get_texture_info("template_meter_left"),
--         right = get_texture_info("template_meter_right"),
--     },
--     pie = {
--         [1] = get_texture_info("template_meter_1"),
--         [2] = get_texture_info("template_meter_2"),
--         [3] = get_texture_info("template_meter_3"),
--         [4] = get_texture_info("template_meter_4"),
--         [5] = get_texture_info("template_meter_5"),
--         [6] = get_texture_info("template_meter_6"),
--         [7] = get_texture_info("template_meter_7"),
--         [8] = get_texture_info("template_meter_8"),
--     }
-- })

-- Optional custom animations.
-- api.character_add_animations(modelId, {
--     anims = {
--         [api.CS_ANIM_MENU] = MARIO_ANIM_IDLE_HEAD_LEFT,
--     },
--     eyes = {
--         [api.CS_ANIM_MENU] = MARIO_EYES_OPEN,
--     },
-- })

-- Optional moveset hooks.
-- api.character_hook_moveset(charNum, HOOK_MARIO_UPDATE, function (m)
--     -- Add custom movement logic here.
-- end)

-- Optional extra character-select presentation data.
-- api.character_add_graffiti(charNum, get_texture_info("template_graffiti"))
-- api.character_add_menu_instrumental(charNum, audio_stream_load("sound/template-theme.ogg"))

return charNum
