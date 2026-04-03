-- name: MoonOS
-- description: \\#6dc6ff\\MoonOS\n\n\\#dcdcdc\\Curated character pack browser for sm64dx.\nCharacter packs are staged in \\#a0d8ff\\moonos/packs\\#dcdcdc\\ during the build and run through Character Select's runtime.
-- pausable: true
-- category: ui

if incompatibleClient then return 0 end

moonOS = rawget(_G, "moonOS") or {}
MoonOS = moonOS
moonos = moonOS

moonOS.version = "0.2.0"
moonOS.packRoot = "moonos/packs"
moonOS.runtime = "character-select"

function moonOS.get_pack_root()
    return moonOS.packRoot
end

function moonOS.get_pack_path(id)
    if id == nil or id == "" then
        return moonOS.packRoot
    end
    return string.format("%s/%s", moonOS.packRoot, tostring(id))
end

local function bind_charselect_api()
    local api = rawget(_G, "charSelect")
    if type(api) ~= "table" then
        return
    end

    moonOS.api = api
    moonOS.voice = api.voice
    moonOS.optionTableRef = api.optionTableRef
    moonOS.controller = api.controller
    moonOS.gCSPlayers = api.gCSPlayers
    moonOS.CUTSCENE_CS_MENU = api.CUTSCENE_CS_MENU
    moonOS.CS_ANIM_MENU = api.CS_ANIM_MENU

    setmetatable(moonOS, {
        __index = function(_, key)
            return api[key]
        end,
    })
end

bind_charselect_api()
