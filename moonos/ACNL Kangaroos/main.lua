-- name: [CS] ACNL Kangaroos
-- description: Write blurb/description here!\n\n\\#ff7777\\This Pack requires Character Select\nto use as a Library!

--[[
    API Documentation for Character Select can be found below:
    https://github.com/Squishy6094/character-select-coop/wiki/API-Documentation

    Use this if you're curious on how anything here works >v<
	(This is an edited version of the Template File by Squishy)
]]

local TEXT_MOD_NAME = "ACNL Kangaroos"

-- Stops mod from loading if Character Select isn't on
if not _G.charSelectExists then
    mxui_popup_create("\\#ffffdc\\\n"..TEXT_MOD_NAME.."\nRequires Character Select DX\nto use as a library!\n\nTurn on Character Select and\nrestart SM64DX.", 5)
    return 0
end

local E_MODEL_CUSTOM_MODEL = smlua_model_util_get_id("kangaroo_base_geo") -- Located in "actors"

local TEX_CUSTOM_LIFE_ICON = get_texture_info("kangaroo_life.png") -- Located in "textures"

-- All Located in "sound" Name them whatever you want. Remember to include the .ogg extension
local VOICETABLE_ACNL_KANGAROOS = {
    --[CHAR_SOUND_OKEY_DOKEY] = 'StartLevel.ogg', -- Starting game
	[CHAR_SOUND_LETS_A_GO] = 'StartLevel.ogg', -- Starting level
	[CHAR_SOUND_PUNCH_YAH] = 'Punch1.ogg', -- Punch 1
	[CHAR_SOUND_PUNCH_WAH] = 'Punch2.ogg', -- Punch 2
	[CHAR_SOUND_PUNCH_HOO] = 'Punch3.ogg', -- Punch 3
	[CHAR_SOUND_YAH_WAH_HOO] = {'Jump1.ogg', 'Jump2.ogg', 'Jump3.ogg'}, -- First/Second jump sounds
	[CHAR_SOUND_HOOHOO] = 'DoubleJump.ogg', -- Third jump sound
	[CHAR_SOUND_YAHOO_WAHA_YIPPEE] = {'TripleJump1.ogg', 'TripleJump2.ogg'}, -- Triple jump sounds
	[CHAR_SOUND_UH] = 'Bonk.ogg', -- Wall bonk
	[CHAR_SOUND_UH2] = 'Silent.ogg', -- Landing after long jump
	[CHAR_SOUND_UH2_2] = 'Silent.ogg', -- Same sound as UH2; jumping onto ledge
	[CHAR_SOUND_HAHA] = 'Silent.ogg', -- Landing triple jump
	[CHAR_SOUND_YAHOO] = 'Jump1.ogg', -- Long jump
	[CHAR_SOUND_DOH] = 'Damaged.ogg', -- Long jump wall bonk
	[CHAR_SOUND_WHOA] = 'GrabLedge.ogg', -- Grabbing ledge
	[CHAR_SOUND_EEUH] = 'Silent.ogg', -- Climbing over ledge
	[CHAR_SOUND_WAAAOOOW] = 'Burned.ogg', -- Falling a long distance
	[CHAR_SOUND_TWIRL_BOUNCE] = 'Jump3.ogg', -- Bouncing off of a flower spring
	[CHAR_SOUND_GROUND_POUND_WAH] = 'Silent.ogg', 
	[CHAR_SOUND_HRMM] = 'GrabLedge.ogg', -- Lifting something
	[CHAR_SOUND_HERE_WE_GO] = 'GetStar.ogg', -- Star get
	[CHAR_SOUND_SO_LONGA_BOWSER] = 'ThrowBowser.ogg', -- Throwing Bowser
--DAMAGE
	[CHAR_SOUND_ATTACKED] = 'Damaged.ogg', -- Damaged
	[CHAR_SOUND_PANTING] = 'Silent.ogg', -- Low health
	[CHAR_SOUND_ON_FIRE] = 'Burned.ogg', -- Burned
--SLEEP SOUNDS
	[CHAR_SOUND_IMA_TIRED] = 'Falling.ogg', -- Mario feeling tired
	[CHAR_SOUND_YAWNING] = 'Silent.ogg', -- Mario yawning before he sits down to sleep
	[CHAR_SOUND_SNORING1] = 'Snore.ogg', -- Snore Inhale
	[CHAR_SOUND_SNORING2] = 'Silent.ogg', -- Exhale
	[CHAR_SOUND_SNORING3] = 'Silent.ogg', -- Sleep talking / mumbling
--COUGHING (USED IN THE GAS MAZE)
	[CHAR_SOUND_COUGHING1] = 'Cough.ogg', -- Cough take 1
	[CHAR_SOUND_COUGHING2] = 'Silent.ogg', -- Cough take 2
	[CHAR_SOUND_COUGHING3] = 'Silent.ogg', -- Cough take 3
--DEATH
	[CHAR_SOUND_DYING] = 'Dying.ogg', -- Dying from damage
	[CHAR_SOUND_DROWNING] = 'Drowning.ogg', -- Running out of air underwater
	[CHAR_SOUND_MAMA_MIA] = 'Falling.ogg' -- Booted out of level
}

local GEO_VARIANTS = {
    {
        name = "Walt",
        description = {
            "Walt from Animal Crossing: New Leaf.",
            "Uses the dedicated Walt geo variant."
        },
        credit = "Mimix23179",
        geoBin = "actors/walt_geo.bin",
    },
    {
        name = "Rooney",
        description = {
            "Rooney from Animal Crossing: New Leaf.",
            "Uses the dedicated Rooney geo variant."
        },
        credit = "Mimix23179",
        geoBin = "actors/rooney_geo.bin",
    },
    {
        name = "Astrid",
        description = {
            "Astrid from Animal Crossing: New Leaf.",
            "Uses the dedicated Astrid geo variant."
        },
        credit = "Mimix23179",
        geoBin = "actors/astrid_geo.bin",
    },
	{
        name = "Kitt",
        description = {
            "Kitt from Animal Crossing: New Leaf.",
            "Uses the dedicated Kitt geo variant."
        },
        credit = "Mimix23179",
        geoBin = "actors/kitt_geo.bin",
    },
	{
        name = "Mathilda",
        description = {
            "Mathilda from Animal Crossing: New Leaf.",
            "Uses the dedicated Mathilda geo variant."
        },
        credit = "Mimix23179",
        geoBin = "actors/mathilda_geo.bin",
    },
	{
        name = "Marcie",
        description = {
            "Marcie from Animal Crossing: New Leaf.",
            "Uses the dedicated Marcie geo variant."
        },
        credit = "Mimix23179",
        geoBin = "actors/marcie_geo.bin",
    },
	{
        name = "Sylvia",
        description = {
            "Sylvia from Animal Crossing: New Leaf.",
            "Uses the dedicated Sylvia geo variant."
        },
        credit = "Mimix23179",
        geoBin = "actors/sylvia_geo.bin",
    },
}

local HM_ACNL_KANGAROOS= {
    label = {
        left = get_texture_info("HealthLeft"),
        right = get_texture_info("HealthRight"),
    },
    pie = {
        [1] = get_texture_info("Pie1"),
        [2] = get_texture_info("Pie2"),
        [3] = get_texture_info("Pie3"),
        [4] = get_texture_info("Pie4"),
        [5] = get_texture_info("Pie5"),
        [6] = get_texture_info("Pie6"),
        [7] = get_texture_info("Pie7"),
        [8] = get_texture_info("Pie8"),
    }
}

local CSloaded = false
local function on_character_select_load()
    CT_ACNL_KANGAROOS = _G.charSelect.character_add(
        "ACNL Kangaroos",
        {
            "The kangaroos from Animal Crossing: New Leaf.",
            "Each resident is a selectable DX model variant."
        },
        "Mimix23179",
        {r = 255, g = 200, b = 200},
        E_MODEL_CUSTOM_MODEL,
        CT_MARIO,
        TEX_CUSTOM_LIFE_ICON
    )

    _G.charSelect.character_set_nickname(CT_ACNL_KANGAROOS, "Kangaroo", false)
    _G.charSelect.character_set_category(CT_ACNL_KANGAROOS, "All DX")
    _G.charSelect.character_add_model_variant(
        CT_ACNL_KANGAROOS,
        {
            name = "Base",
            description = {
                "The shared ACNL kangaroo base model.",
                "Use the variant dial to switch residents."
            },
            credit = "Mimix23179",
            model = E_MODEL_CUSTOM_MODEL,
        }
    )
    _G.charSelect.character_add_geo_variants(CT_ACNL_KANGAROOS, GEO_VARIANTS)

    _G.charSelect.character_add_voice(E_MODEL_CUSTOM_MODEL, VOICETABLE_ACNL_KANGAROOS)
    _G.charSelect.character_add_health_meter(CT_ACNL_KANGAROOS, HM_ACNL_KANGAROOS)

    CSloaded = true
end

local function on_character_sound(m, sound)
    if not CSloaded then return end
    if _G.charSelect.character_get_voice(m) == VOICETABLE_ACNL_KANGAROOS then return _G.charSelect.voice.sound(m, sound) end
end

local function on_character_snore(m)
    if not CSloaded then return end
    if _G.charSelect.character_get_voice(m) == VOICETABLE_ACNL_KANGAROOS then return _G.charSelect.voice.snore(m) end
end

hook_event(HOOK_ON_MODS_LOADED, on_character_select_load)
hook_event(HOOK_CHARACTER_SOUND, on_character_sound)
hook_event(HOOK_MARIO_UPDATE, on_character_snore)
