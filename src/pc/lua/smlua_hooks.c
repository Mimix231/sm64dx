#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "smlua.h"
#include "sm64.h"
#include "behavior_commands.h"
#include "pc/mods/mod.h"
#include "game/object_list_processor.h"
#include "pc/mxui/mxui_exports.h"
#include "pc/crash_handler.h"
#include "game/hud.h"
#include "game/level_update.h"
#include "pc/debug_context.h"
#include "pc/network/network.h"
#include "pc/network/network_player.h"
#include "pc/network/socket/socket.h"
#include "pc/chat_commands.h"
#include "pc/pc_main.h"
#include "pc/configfile.h"
#include "pc/controller/controller_api.h"
#include "pc/mxui/mxui.h"
#include "pc/mxui/mxui_hud.h"
#include "pc/mxui/mxui_popup.h"
#include "pc/utils/misc.h"
#include "pc/lua/utils/smlua_model_utils.h"
#include "smlua_utils.h"

#include "../mods/mod_bindings.h"
#include "../mods/mods.h"
#include "game/print.h"
#include "gfx_dimensions.h"

#define MAX_HOOKED_REFERENCES 64
#define LUA_BEHAVIOR_FLAG (1 << 15)

u64* gBehaviorOffset = &gPcDebug.bhvOffset;

struct LuaHookedEvent {
    int reference[MAX_HOOKED_REFERENCES];
    struct Mod* mod[MAX_HOOKED_REFERENCES];
    struct ModFile* modFile[MAX_HOOKED_REFERENCES];
    int count;
};

static struct LuaHookedEvent sHookedEvents[HOOK_MAX] = { 0 };

static const char* sLuaHookedEventTypeName[] = {
#define SMLUA_EVENT_HOOK(hookEventType, ...) [hookEventType] = #hookEventType,
#include "smlua_hook_events.inl"
#undef SMLUA_EVENT_HOOK
    [HOOK_MAX] = "HOOK_MAX"
};

int smlua_call_hook(lua_State* L, int nargs, int nresults, int errfunc, struct Mod* activeMod, struct ModFile* activeModFile) {
    if (!gGameInited) { return 0; } // Don't call hooks while the game is booting

    struct Mod* prevActiveMod = gLuaActiveMod;
    struct ModFile* prevActiveModFile = gLuaActiveModFile;

    gLuaActiveMod = activeMod;
    gLuaActiveModFile = activeModFile;
    gLuaLastHookMod = activeMod;
    gPcDebug.lastModRun = activeMod;

    lua_profiler_start_counter(activeMod);

    CTX_BEGIN(CTX_HOOK);
    int rc = smlua_pcall(L, nargs, nresults, errfunc);
    CTX_END(CTX_HOOK);

    lua_profiler_stop_counter(activeMod);

    gLuaActiveMod = prevActiveMod;
    gLuaActiveModFile = prevActiveModFile;
    return rc;
}

int smlua_hook_event(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 2)) { return 0; }

    u16 hookType = smlua_to_integer(L, -2);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Invalid hook type given to hook_event(): %d", hookType);
        return 0;
    }

    if (hookType >= HOOK_MAX) {
        LOG_LUA_LINE("Hook Type: %d exceeds max!", hookType);
        return 0;
    }

    struct LuaHookedEvent* hook = &sHookedEvents[hookType];
    if (hook->count >= MAX_HOOKED_REFERENCES) {
        LOG_LUA_LINE("Hook Type: %s exceeded maximum references!", sLuaHookedEventTypeName[hookType]);
        return 0;
    }

    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (ref == -1) {
        LOG_LUA_LINE("Tried to hook undefined function to '%s'", sLuaHookedEventTypeName[hookType]);
        return 0;
    }

    hook->reference[hook->count] = ref;
    hook->mod[hook->count] = gLuaActiveMod;
    hook->modFile[hook->count] = gLuaActiveModFile;
    hook->count++;

    return 1;
}

  ///////////////////
 // hooked events //
///////////////////

#include "smlua_hook_events_autogen.inl"

static bool smlua_call_event_hooks_on_hud_render(void (*resetFunc)(void), bool renderBehind) {
    lua_State *L = gLuaState;
    if (L == NULL) { return false; }
    bool hookResult = false;

    if (resetFunc) { resetFunc(); }

    const enum LuaHookedEventType renderHudHookTypes[] = {
        HOOK_ON_HUD_RENDER_BEHIND,
        HOOK_ON_HUD_RENDER,
    };
    for (s32 k = renderBehind ? 0 : 1; k != 2; ++k) {
        enum LuaHookedEventType hookType = renderHudHookTypes[k];
        struct LuaHookedEvent *hook = &sHookedEvents[hookType];
        for (int i = 0; i < hook->count; i++) {

            // support deprecated render behind hud
            if (hookType == HOOK_ON_HUD_RENDER && hook->mod[i]->renderBehindHud != renderBehind) {
                continue;
            }

            // push the callback onto the stack
            lua_rawgeti(L, LUA_REGISTRYINDEX, hook->reference[i]);

            // call the callback
            if (0 != smlua_call_hook(L, 0, 0, 0, hook->mod[i], hook->modFile[i])) {
                LOG_LUA("Failed to call the callback for hook %s", sLuaHookedEventTypeName[hookType]);
            } else {
                hookResult = true;
            }

            if (resetFunc) { resetFunc(); }
        }
    }
    return hookResult;
}

bool smlua_call_event_hooks_HOOK_ON_HUD_RENDER(void (*resetFunc)(void)) {
    return smlua_call_event_hooks_on_hud_render(resetFunc, false);
}

bool smlua_call_event_hooks_HOOK_ON_HUD_RENDER_BEHIND(void (*resetFunc)(void)) {
    return smlua_call_event_hooks_on_hud_render(resetFunc, true);
}

bool smlua_call_event_hooks_HOOK_ON_NAMETAGS_RENDER(s32 playerIndex, Vec3f pos, const char **playerNameOverride) {
    lua_State *L = gLuaState;
    if (L == NULL) { return false; }

    struct LuaHookedEvent *hook = &sHookedEvents[HOOK_ON_NAMETAGS_RENDER];
    for (int i = 0; i < hook->count; i++) {
        s32 prevTop = lua_gettop(L);

        // push the callback onto the stack
        lua_rawgeti(L, LUA_REGISTRYINDEX, hook->reference[i]);

        // push playerIndex
        lua_pushinteger(L, playerIndex);

        // push pos
        extern void smlua_new_vec3f(Vec3f src);
        smlua_new_vec3f(pos);

        // call the callback
        if (0 != smlua_call_hook(L, 2, 1, 0, hook->mod[i], hook->modFile[i])) {
            LOG_LUA("Failed to call the callback for hook %s", sLuaHookedEventTypeName[HOOK_ON_NAMETAGS_RENDER]);
            continue;
        }

        // return playerNameOverride
        if (lua_type(L, -1) == LUA_TSTRING) {
            *playerNameOverride = smlua_to_string(L, -1);
            lua_settop(L, prevTop);
            return true;
        }

        // if it's a table, override name, pos or both
        if (lua_type(L, -1) == LUA_TTABLE) {
            bool override = false;

            // name
            lua_getfield(L, -1, "name");
            if (lua_type(L, -1) == LUA_TSTRING) {
                *playerNameOverride = smlua_to_string(L, -1);
                override = true;
            }
            lua_pop(L, 1);

            // pos
            lua_getfield(L, -1, "pos");
            if (lua_type(L, -1) == LUA_TTABLE) {
                extern void smlua_get_vec3f(Vec3f dest, int index);
                smlua_get_vec3f(pos, -1);
                override = true;
            }
            lua_pop(L, 1);

            lua_settop(L, prevTop);
            if (override) {
                return true;
            }
        }

        lua_settop(L, prevTop);
    }
    return false;
}

  ////////////////////
 // hooked actions //
////////////////////

struct LuaHookedMarioAction {
    u32 action;
    u32 interactionType;
    int actionHookRefs[ACTION_HOOK_MAX];
    struct Mod* mod;
    struct ModFile* modFile;
};

#define MAX_HOOKED_ACTIONS (ACT_NUM_GROUPS * ACT_NUM_ACTIONS_PER_GROUP)

static struct LuaHookedMarioAction sHookedMarioActions[MAX_HOOKED_ACTIONS] = { 0 };
static int sHookedMarioActionsCount = 0;
u32 gLuaMarioActionIndex[ACT_NUM_GROUPS] = { 0 };

int smlua_hook_mario_action(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_range(L, 2, 3)) { return 0; }

    if (gLuaLoadingMod == NULL) {
        LOG_LUA_LINE("hook_mario_action() can only be called on load.");
        return 0;
    }

    int paramCount = lua_gettop(L);

    if (sHookedMarioActionsCount >= MAX_HOOKED_ACTIONS) {
        LOG_LUA_LINE("Hooked mario actions exceeded maximum references!");
        return 0;
    }

    lua_Integer action = smlua_to_integer(L, 1);
    if (action == 0 || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook Action: tried to hook invalid action: %lld, %u", action, gSmLuaConvertSuccess);
        return 0;
    }

    int secondParamType = lua_type(L, 2);
    bool oldApi = secondParamType == LUA_TFUNCTION;

    if (!oldApi && secondParamType != LUA_TTABLE) {
        LOG_LUA_LINE("smlua_hook_mario_action received improper type '%s'", luaL_typename(L, 2));
        return 0;
    }

    lua_Integer interactionType = 0;
    if (paramCount >= 3) {
        interactionType = smlua_to_integer(L, 3);
        if (!gSmLuaConvertSuccess) {
            LOG_LUA_LINE("Hook Action: tried to hook invalid interactionType: %lld, %u", interactionType, gSmLuaConvertSuccess);
            return 0;
        }
    }

    struct LuaHookedMarioAction* hooked = &sHookedMarioActions[sHookedMarioActionsCount];

    // Support calling the function with just one function corresponding to the "every frame" hook instead of a full
    // table with all hooks
    if (oldApi) {
        for (int i = 0; i < ACTION_HOOK_MAX; i++) {
            hooked->actionHookRefs[i] = LUA_NOREF;
        }

        lua_pushvalue(L, 2);
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);

        if (ref == -1) {
            LOG_LUA_LINE("Hook Action: %lld tried to hook undefined function", action);
            return 0;
        }

        hooked->actionHookRefs[ACTION_HOOK_EVERY_FRAME] = ref;
    }
    else {
        for (int i = 0; i < ACTION_HOOK_MAX; i++) {
            lua_pushstring(L, LuaActionHookTypeArgName[i]);

            if (lua_gettable(L, 2) == LUA_TNIL) {
                hooked->actionHookRefs[i] = LUA_NOREF;
            } else {
                int ref = luaL_ref(L, LUA_REGISTRYINDEX);

                if (ref == -1) {
                    LOG_LUA_LINE("Hook Action: %lld tried to hook undefined function", action);
                    return 0;
                }

                hooked->actionHookRefs[i] = ref;
            }
        }
    }

    hooked->action = action;
    hooked->interactionType = interactionType;
    hooked->mod = gLuaActiveMod;
    hooked->modFile = gLuaActiveModFile;

    sHookedMarioActionsCount++;
    return 1;
}

bool smlua_call_action_hook(enum LuaActionHookType hookType, struct MarioState* m, s32* cancel) {
    lua_State* L = gLuaState;
    if (L == NULL) { return false; }

    //TODO GAG: Set up things such that O(n) check isn't performed on every action hook? Maybe in MarioState?
    for (int i = 0; i < sHookedMarioActionsCount; i++) {
        struct LuaHookedMarioAction* hook = &sHookedMarioActions[i];
        if (hook->action == m->action && hook->actionHookRefs[hookType] != LUA_NOREF) {
            // push the callback onto the stack
            lua_rawgeti(L, LUA_REGISTRYINDEX, hook->actionHookRefs[hookType]);

            // push mario state
            lua_getglobal(L, "gMarioStates");
            lua_pushinteger(L, m->playerIndex);
            lua_gettable(L, -2);
            lua_remove(L, -2);

            // call the callback
            if (0 != smlua_call_hook(L, 1, 1, 0, hook->mod, hook->modFile)) {
                LOG_LUA("Failed to call the action callback: '%08X'", m->action);
                continue;
            }

            // output the return value
            // special return values:
            // - returning -1 allows to continue the execution, useful when overriding vanilla actions
            bool stopActionHook = true;
            *cancel = FALSE;

            switch (lua_type(L, -1)) {
                case LUA_TBOOLEAN: {
                    *cancel = smlua_to_boolean(L, -1) ? TRUE : FALSE;
                } break;

                case LUA_TNUMBER: {
                    s32 returnValue = (s32) smlua_to_integer(L, -1);
                    if (returnValue > 0) {
                        *cancel = TRUE;
                    } else if (returnValue == 0) {
                        *cancel = FALSE;
                    } else if (returnValue == ACTION_HOOK_CONTINUE_EXECUTION) {
                        stopActionHook = false;
                    } else {
                        LOG_LUA("Invalid return value when calling the action callback: '%08X' returned %d", m->action, returnValue);
                    }
                } break;
            }
            lua_pop(L, 1);

            if (stopActionHook) {
                return true;
            }
        }
    }

    return false;
}

u32 smlua_get_action_interaction_type(struct MarioState* m) {
    u32 interactionType = 0;
    lua_State* L = gLuaState;
    if (L == NULL) { return false; }
    for (int i = 0; i < sHookedMarioActionsCount; i++) {
        if (sHookedMarioActions[i].action == m->action) {
            interactionType |= sHookedMarioActions[i].interactionType;
        }
    }
    return interactionType;
}

  //////////////////////
 // hooked behaviors //
//////////////////////

struct LuaHookedBehavior {
    u32 behaviorId;
    u32 overrideId;
    u32 originalId;
    BehaviorScript *behavior;
    const BehaviorScript* originalBehavior;
    const char* bhvName;
    int initReference;
    int loopReference;
    bool replace;
    bool luaBehavior;
    struct Mod* mod;
    struct ModFile* modFile;
};

#define MAX_HOOKED_BEHAVIORS 1024

static struct LuaHookedBehavior sHookedBehaviors[MAX_HOOKED_BEHAVIORS] = { 0 };
static int sHookedBehaviorsCount = 0;

enum BehaviorId smlua_get_original_behavior_id(const BehaviorScript* behavior) {
    enum BehaviorId id = get_id_from_behavior(behavior);
    for (int i = 0; i < sHookedBehaviorsCount; i++) {
        struct LuaHookedBehavior* hooked = &sHookedBehaviors[i];
        if (hooked->behavior == behavior) {
            id = hooked->overrideId;
        }
    }
    return id;
}

const BehaviorScript* smlua_override_behavior(const BehaviorScript *behavior) {
    lua_State *L = gLuaState;
    if (L == NULL) { return behavior; }

    enum BehaviorId id = get_id_from_behavior(behavior);
    const BehaviorScript *hookedBehavior = smlua_get_hooked_behavior_from_id(id, false);
    if (hookedBehavior != NULL) { return hookedBehavior; }
    return behavior + *gBehaviorOffset;
}

const BehaviorScript* smlua_get_hooked_behavior_from_id(enum BehaviorId id, bool returnOriginal) {
    lua_State *L = gLuaState;
    if (L == NULL) { return NULL; }

    for (int i = 0; i < sHookedBehaviorsCount; i++) {
        struct LuaHookedBehavior* hooked = &sHookedBehaviors[i];
        if (hooked->behaviorId != id && hooked->overrideId != id) { continue; }
        if (returnOriginal && !hooked->replace) { return hooked->originalBehavior; }
        return hooked->behavior;
    }
    return NULL;
}

bool smlua_is_behavior_hooked(const BehaviorScript *behavior) {
    lua_State *L = gLuaState;
    if (L == NULL) { return false; }

    enum BehaviorId id = get_id_from_behavior(behavior);
    for (int i = 0; i < sHookedBehaviorsCount; i++) {
        struct LuaHookedBehavior *hooked = &sHookedBehaviors[i];
        if (hooked->behaviorId != id && hooked->overrideId != id) { continue; }
        return hooked->luaBehavior;
    }

    return false;
}

const char* smlua_get_name_from_hooked_behavior_id(enum BehaviorId id) {
    for (int i = 0; i < sHookedBehaviorsCount; i++) {
        struct LuaHookedBehavior *hooked = &sHookedBehaviors[i];
        if (hooked->behaviorId != id && hooked->overrideId != id) { continue; }
        return hooked->bhvName;
    }
    return NULL;
}

int smlua_hook_custom_bhv(BehaviorScript *bhvScript, const char *bhvName) {
    if (sHookedBehaviorsCount >= MAX_HOOKED_BEHAVIORS) {
        LOG_ERROR("Hooked behaviors exceeded maximum references!");
        return 0;
    }

    u32 originalBehaviorId = get_id_from_behavior(bhvScript);

    if (originalBehaviorId == id_bhvMario) {
        LOG_LUA_LINE("Cannot hook Mario's behavior. Use HOOK_MARIO_UPDATE and HOOK_BEFORE_MARIO_UPDATE.");
        return 0;
    }

    u8 newBehavior = originalBehaviorId >= id_bhv_max_count;

    struct LuaHookedBehavior *hooked = &sHookedBehaviors[sHookedBehaviorsCount];
    u16 customBehaviorId = (sHookedBehaviorsCount & 0xFFFF) | LUA_BEHAVIOR_FLAG;
    hooked->behavior = bhvScript;
    hooked->behavior[1] = (BehaviorScript)BC_B0H(0x39, customBehaviorId); // This is ID(customBehaviorId)
    hooked->behaviorId = customBehaviorId;
    hooked->overrideId = newBehavior ? customBehaviorId : originalBehaviorId;
    hooked->originalId = originalBehaviorId;
    hooked->originalBehavior = newBehavior ? bhvScript : get_behavior_from_id(originalBehaviorId);
    hooked->bhvName = bhvName;
    hooked->initReference = 0;
    hooked->loopReference = 0;
    hooked->replace = true;
    hooked->luaBehavior = false;
    hooked->mod = gLuaActiveMod;
    hooked->modFile = gLuaActiveModFile;

    sHookedBehaviorsCount++;

    // We want to push the behavior into the global LUA state. So mods can access it.
    // It's also used for some things that would normally access a LUA behavior instead.
    lua_State* L = gLuaState;
    if (L != NULL) {
        lua_pushinteger(L, customBehaviorId);
        lua_setglobal(L, bhvName);
        LOG_INFO("Registered custom behavior: 0x%04hX - %s", customBehaviorId, bhvName);
    }

    return 1;
}

int smlua_hook_behavior(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_range(L, 5, 6)) { return 0; }

    if (gLuaLoadingMod == NULL) {
        LOG_LUA_LINE("hook_behavior() can only be called on load.");
        return 0;
    }

    int paramCount = lua_gettop(L);

    if (sHookedBehaviorsCount >= MAX_HOOKED_BEHAVIORS) {
        LOG_LUA_LINE("Hooked behaviors exceeded maximum references!");
        return 0;
    }

    bool noOverrideId = (lua_type(L, 1) == LUA_TNIL);
    gSmLuaConvertSuccess = true;
    lua_Integer overrideBehaviorId = noOverrideId ? 0xFFFFFF : smlua_to_integer(L, 1);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook behavior: tried to override invalid behavior: %lld, %u", overrideBehaviorId, gSmLuaConvertSuccess);
        return 0;
    }

    if (overrideBehaviorId == id_bhvMario) {
        LOG_LUA_LINE("Hook behavior: cannot hook Mario's behavior. Use HOOK_MARIO_UPDATE and HOOK_BEFORE_MARIO_UPDATE.");
        return 0;
    }

    lua_Integer objectList = smlua_to_integer(L, 2);
    if (objectList <= 0 || objectList >= NUM_OBJ_LISTS || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook behavior: tried use invalid object list: %lld, %u", objectList, gSmLuaConvertSuccess);
        return 0;
    }

    bool replaceBehavior = smlua_to_boolean(L, 3);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook behavior: could not parse replaceBehavior");
        return 0;
    }
    const BehaviorScript* originalBehavior = noOverrideId ? NULL : get_behavior_from_id(overrideBehaviorId);
    if (originalBehavior == NULL) {
        replaceBehavior = true;
    }

    int initReference = 0;
    int initReferenceType = lua_type(L, 4);
    if (initReferenceType == LUA_TNIL) {
        // nothing
    } else if (initReferenceType == LUA_TFUNCTION) {
        // get reference
        lua_pushvalue(L, 4);
        initReference = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        LOG_LUA_LINE("Hook behavior: tried to reference non-function for init");
        return 0;
    }

    int loopReference = 0;
    int loopReferenceType = lua_type(L, 5);
    if (loopReferenceType == LUA_TNIL) {
        // nothing
    } else if (loopReferenceType == LUA_TFUNCTION) {
        // get reference
        lua_pushvalue(L, 5);
        loopReference = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        LOG_LUA_LINE("Hook behavior: tried to reference non-function for loop");
        return 0;
    }

    const char *bhvName = NULL;
    if (paramCount >= 6) {
        int bhvNameType = lua_type(L, 6);
        if (bhvNameType == LUA_TNIL) {
            // nothing
        } else if (bhvNameType == LUA_TSTRING) {
            bhvName = smlua_to_string(L, 6);
            if (!bhvName || !gSmLuaConvertSuccess) {
                LOG_LUA_LINE("Hook behavior: could not parse bhvName");
                return 0;
            }
        } else {
            LOG_LUA_LINE("Hook behavior: invalid type passed for argument bhvName: %u", bhvNameType);
            return 0;
        }
    }

    // If not provided, generate generic behavior name: bhv<ModName>Custom<Index>
    // - <ModName> is the mod name in CamelCase format, alphanumeric chars only
    // - <Index> is in 3-digit numeric format, ranged from 001 to 256
    // For example, the 4th unnamed behavior of the mod "my-great_MOD" will be named "bhvMyGreatMODCustom004"
    if (!bhvName) {
        static char sGenericBhvName[MOD_NAME_MAX_LENGTH + 16];
        s32 i = 3;
        snprintf(sGenericBhvName, 4, "bhv");
        for (char caps = TRUE, *c = gLuaLoadingMod->name; *c && i < MOD_NAME_MAX_LENGTH + 3; ++c) {
            if ('0' <= *c && *c <= '9') {
                sGenericBhvName[i++] = *c;
                caps = TRUE;
            } else if ('A' <= *c && *c <= 'Z') {
                sGenericBhvName[i++] = *c;
                caps = FALSE;
            } else if ('a' <= *c && *c <= 'z') {
                sGenericBhvName[i++] = *c + (caps ? 'A' - 'a' : 0);
                caps = FALSE;
            } else {
                caps = TRUE;
            }
        }
        snprintf(sGenericBhvName + i, 12, "Custom%03u", (u32) (gLuaLoadingMod->customBehaviorIndex++) + 1);
        bhvName = sGenericBhvName;
    }

    struct LuaHookedBehavior* hooked = &sHookedBehaviors[sHookedBehaviorsCount];
    u16 customBehaviorId = (sHookedBehaviorsCount & 0xFFFF) | LUA_BEHAVIOR_FLAG;
    hooked->behavior = calloc(4, sizeof(BehaviorScript));
    hooked->behavior[0] = (BehaviorScript)BC_BB(0x00, objectList); // This is BEGIN(objectList)
    hooked->behavior[1] = (BehaviorScript)BC_B0H(0x39, customBehaviorId); // This is ID(customBehaviorId)
    hooked->behavior[2] = (BehaviorScript)BC_B(0x0A); // This is BREAK()
    hooked->behavior[3] = (BehaviorScript)BC_B(0x0A); // This is BREAK()
    hooked->behaviorId = customBehaviorId;
    hooked->overrideId = noOverrideId ? customBehaviorId : overrideBehaviorId;
    hooked->originalId = customBehaviorId; // For LUA behaviors. The only behavior id they have IS their custom one.
    hooked->originalBehavior = originalBehavior ? originalBehavior : hooked->behavior;
    hooked->bhvName = bhvName;
    hooked->initReference = initReference;
    hooked->loopReference = loopReference;
    hooked->replace = replaceBehavior;
    hooked->luaBehavior = true;
    hooked->mod = gLuaActiveMod;
    hooked->modFile = gLuaActiveModFile;

    sHookedBehaviorsCount++;

    // We want to push the behavior into the global LUA state. So mods can access it.
    // It's also used for some things that would normally access a LUA behavior instead.
    lua_pushinteger(L, customBehaviorId);
    lua_setglobal(L, bhvName);
    LOG_INFO("Registered custom behavior: 0x%04hX - %s", customBehaviorId, bhvName);

    // return behavior ID
    lua_pushinteger(L, customBehaviorId);

    return 1;
}

bool smlua_call_behavior_hook(const BehaviorScript** behavior, struct Object* object, bool before) {
    lua_State* L = gLuaState;
    if (L == NULL) { return false; }
    for (int i = 0; i < sHookedBehaviorsCount; i++) {
        struct LuaHookedBehavior* hooked = &sHookedBehaviors[i];

        // find behavior
        if (object->behavior != hooked->behavior) {
            continue;
        }

        // Figure out whether to run before or after
        if (before && !hooked->replace) {
            return false;
        }
        if (!before && hooked->replace) {
            return false;
        }

        // This behavior doesn't call it's LUA functions in this manner. It actually uses the normal behavior
        // system.
        if (!hooked->luaBehavior) {
            return false;
        }

        // retrieve and remember first run
        bool firstRun = (object->curBhvCommand == hooked->originalBehavior) || (object->curBhvCommand == hooked->behavior);
        if (firstRun && hooked->replace) { *behavior = &hooked->behavior[1]; }

        // get function and null check it
        int reference = firstRun ? hooked->initReference : hooked->loopReference;
        if (reference == 0) {
            return true;
        }

        // push the callback onto the stack
        lua_rawgeti(L, LUA_REGISTRYINDEX, reference);

        // push object
        smlua_push_object(L, LOT_OBJECT, object, NULL);

        // call the callback
        if (0 != smlua_call_hook(L, 1, 0, 0, hooked->mod, hooked->modFile)) {
            LOG_LUA("Failed to call the behavior callback: %u", hooked->behaviorId);
            return true;
        }

        return hooked->replace;
    }

    return false;
}


  /////////////////////////
 // hooked chat command //
/////////////////////////

struct LuaHookedChatCommand {
    char* command;
    char* description;
    int reference;
    struct Mod* mod;
    struct ModFile* modFile;
};

#define MAX_HOOKED_CHAT_COMMANDS 512

static struct LuaHookedChatCommand sHookedChatCommands[MAX_HOOKED_CHAT_COMMANDS] = { 0 };
static int sHookedChatCommandsCount = 0;

int smlua_hook_chat_command(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 3)) { return 0; }

    if (gLuaLoadingMod == NULL) {
        LOG_LUA_LINE("hook_chat_command() can only be called on load.");
        return 0;
    }

    if (sHookedChatCommandsCount >= MAX_HOOKED_CHAT_COMMANDS) {
        LOG_LUA_LINE("Hooked chat command exceeded maximum references!");
        return 0;
    }

    const char* command = smlua_to_string(L, 1);
    if (command == NULL || strlen(command) == 0 || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook chat command: tried to hook invalid command");
        return 0;
    }

    const char* description = smlua_to_string(L, 2);
    if (description == NULL || strlen(description) == 0 || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook chat command: tried to hook invalid description");
        return 0;
    }

    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (ref == -1) {
        LOG_LUA_LINE("Hook chat command: tried to hook undefined function '%s'", command);
        return 0;
    }

    struct LuaHookedChatCommand* hooked = &sHookedChatCommands[sHookedChatCommandsCount];
    hooked->command = strdup(command);
    hooked->description = strdup(description);
    hooked->reference = ref;
    hooked->mod = gLuaActiveMod;
    hooked->modFile = gLuaActiveModFile;

    sHookedChatCommandsCount++;
    return 1;
}

int smlua_update_chat_command_description(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 2)) { return 0; }

    const char* command = smlua_to_string(L, 1);
    if (command == NULL || strlen(command) == 0 || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Update chat command: tried to update invalid command");
        return 0;
    }

    const char* description = smlua_to_string(L, 2);
    if (description == NULL || strlen(description) == 0 || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Update chat command: tried to update invalid description");
        return 0;
    }

    for (int i = 0; i < sHookedChatCommandsCount; i++) {
        struct LuaHookedChatCommand* hook = &sHookedChatCommands[i];
        if (!strcmp(hook->command, command)) {
            if (hook->description) {
                free(hook->description);
            }
            hook->description = strdup(description);
            return 1;
        }
    }

    LOG_LUA_LINE("Update chat command: could not find command to update");
    return 0;
}

bool smlua_call_chat_command_hook(char* command) {
    lua_State* L = gLuaState;
    if (L == NULL) { return false; }
    for (int i = 0; i < sHookedChatCommandsCount; i++) {
        struct LuaHookedChatCommand* hook = &sHookedChatCommands[i];
        size_t commandLength = strlen(hook->command);
        for (size_t j = 0; j < commandLength; j++) {
            if (hook->command[j] != command[j + 1]) {
                goto NEXT_HOOK;
            }
        }

        char* params = &command[commandLength + 1];
        if (*params != '\0' && *params != ' ') {
            goto NEXT_HOOK;
        }
        if (*params == ' ') {
            params++;
        }

        // push the callback onto the stack
        lua_rawgeti(L, LUA_REGISTRYINDEX, hook->reference);

        // push parameter
        lua_pushstring(L, params);

        // call the callback
        if (0 != smlua_call_hook(L, 1, 1, 0, hook->mod, hook->modFile)) {
            LOG_LUA("Failed to call the chat command callback: %s", command);
            continue;
        }

        // output the return value
        bool returnValue = false;
        if (lua_type(L, -1) == LUA_TBOOLEAN) {
            returnValue = smlua_to_boolean(L, -1);
        }
        lua_pop(L, 1);

        if (!gSmLuaConvertSuccess) { return false; }

        return returnValue;

NEXT_HOOK:;
    }

    return false;
}

void smlua_display_chat_commands(void) {
    for (int i = 0; i < sHookedChatCommandsCount; i++) {
        struct LuaHookedChatCommand* hook = &sHookedChatCommands[i];
        char msg[256] = { 0 };
        snprintf(msg, 256, "/%s %s", hook->command, hook->description);
        djui_chat_message_create(msg);
    }
}

bool is_valid_subcommand(const char* start, const char* end) {
    for (const char* ptr = start; ptr < end; ptr++) {
        if (isspace(*ptr) || *ptr == '\0') {
            return false;
        }
    }
    return true;
}

s32 sort_alphabetically(const void *a, const void *b) {
    const char* strA = *(const char**)a;
    const char* strB = *(const char**)b;

    s32 cmpResult = strcasecmp(strA, strB);

    if (cmpResult == 0) {
        return strcmp(strA, strB);
    }

    return cmpResult;
}

char** smlua_get_chat_player_list(void) {
    char* playerNames[MAX_PLAYERS] = { NULL };
    s32 playerCount = 0;

    for (s32 i = 0; i < MAX_PLAYERS; i++) {
        struct NetworkPlayer* np = &gNetworkPlayers[i];
        if (!np->connected) continue;

        bool isDuplicate = false;
        for (s32 j = 0; j < playerCount; j++) {
            if (strcmp(playerNames[j], np->name) == 0) {
                isDuplicate = true;
                break;
            }
        }

        if (!isDuplicate) {
            playerNames[playerCount++] = np->name;
        }
    }

    qsort(playerNames, playerCount, sizeof(char*), sort_alphabetically);

    char** sortedPlayers = (char**) malloc((playerCount + 1) * sizeof(char*));
    for (s32 i = 0; i < playerCount; i++) {
        sortedPlayers[i] = strdup(playerNames[i]);
    }
    sortedPlayers[playerCount] = NULL;
    return sortedPlayers;
}

char** smlua_get_chat_maincommands_list(void) {
#if defined(DEVELOPMENT)
    s32 defaultCmdsCount = 11;
    static char* defaultCmds[] = {"players", "kick", "ban", "permban", "moderator", "help", "?", "warp", "lua", "luaf", NULL};
#else
    s32 defaultCmdsCount = 8;
    static char* defaultCmds[] = {"players", "kick", "ban", "permban", "moderator", "help", "?", NULL};
#endif
    s32 defaultCmdsCountNew = 0;
    for (s32 i = 0; i < defaultCmdsCount; i++) {
        if (defaultCmds[i] != NULL) {
            defaultCmdsCountNew++;
        } else if (gServerSettings.nametags && defaultCmds[i] == NULL) {
            defaultCmds[i] = "nametags";
            defaultCmdsCountNew++;
            break;
        }
    }
    char** commands = (char**) malloc((sHookedChatCommandsCount + defaultCmdsCountNew + 1) * sizeof(char*));
    for (s32 i = 0; i < sHookedChatCommandsCount; i++) {
        struct LuaHookedChatCommand* hook = &sHookedChatCommands[i];
        commands[i] = strdup(hook->command);
    }
    for (s32 i = 0; i < defaultCmdsCount; i++) {
        if (defaultCmds[i] != NULL) {
            commands[sHookedChatCommandsCount + i] = strdup(defaultCmds[i]);
        }
    }
    commands[sHookedChatCommandsCount + defaultCmdsCountNew] = NULL;
    qsort(commands, sHookedChatCommandsCount + defaultCmdsCountNew, sizeof(char*), sort_alphabetically);
    return commands;
}

char** smlua_get_chat_subcommands_list(const char* maincommand) {
    if (gServerSettings.nametags && strcmp(maincommand, "nametags") == 0) {
        s32 count = 2;
        char** subcommands = (char**) malloc((count + 1) * sizeof(char*));
        subcommands[0] = strdup("show-tag");
        subcommands[1] = strdup("show-health");
        subcommands[2] = NULL;
        return subcommands;
    }

    for (s32 i = 0; i < sHookedChatCommandsCount; i++) {
        struct LuaHookedChatCommand* hook = &sHookedChatCommands[i];
        if (strcmp(hook->command, maincommand) == 0) {
            char* noColorsDesc = str_remove_color_codes(hook->description);
            char* startSubcommands = strstr(noColorsDesc, "[");
            char* endSubcommands = strstr(noColorsDesc, "]");

            if (startSubcommands && endSubcommands && is_valid_subcommand(startSubcommands + 1, endSubcommands)) {
                *endSubcommands = '\0';
                char* subcommandsStr = strdup(startSubcommands + 1);

                s32 count = 1;
                for (s32 j = 0; subcommandsStr[j]; j++) {
                    if (subcommandsStr[j] == '|') count++;
                }

                char** subcommands = (char**) malloc((count + 1) * sizeof(char*));
                char* token = strtok(subcommandsStr, "|");
                s32 index = 0;
                while (token) {
                    subcommands[index++] = strdup(token);
                    token = strtok(NULL, "|");
                }
                subcommands[index] = NULL;

                qsort(subcommands, count, sizeof(char*), sort_alphabetically);

                free(noColorsDesc);
                free(subcommandsStr);
                return subcommands;
            }
            free(noColorsDesc);
        }
    }
    return NULL;
}

bool smlua_maincommand_exists(const char* maincommand) {
    char** commands = smlua_get_chat_maincommands_list();
    bool result = false;

    s32 i = 0;
    while (commands[i] != NULL) {
        if (strcmp(commands[i], maincommand) == 0) {
            result = true;
            break;
        }
        i++;
    }

    for (s32 j = 0; commands[j] != NULL; j++) {
        free(commands[j]);
    }
    free(commands);

    return result;
}

bool smlua_subcommand_exists(const char* maincommand, const char* subcommand) {
    char** subcommands = smlua_get_chat_subcommands_list(maincommand);

    if (subcommands == NULL) {
        return false;
    }

    bool result = false;
    s32 i = 0;
    while (subcommands[i] != NULL) {
        if (strcmp(subcommands[i], subcommand) == 0) {
            result = true;
            break;
        }
        i++;
    }

    for (s32 j = 0; subcommands[j] != NULL; j++) {
        free(subcommands[j]);
    }
    free(subcommands);

    return result;
}

  //////////////////////////////
 // hooked sync table change //
//////////////////////////////

int smlua_hook_on_sync_table_change(lua_State* L) {
    LUA_STACK_CHECK_BEGIN(L);
    if (L == NULL) { return 0; }
    if(!smlua_functions_valid_param_count(L, 4)) { return 0; }

    int syncTableIndex = 1;
    int keyIndex = 2;
    int tagIndex = 3;
    int funcIndex = 4;

    if (gLuaLoadingMod == NULL) {
        LOG_LUA_LINE("hook_on_sync_table_change() can only be called on load.");
        return 0;
    }

    if (lua_type(L, syncTableIndex) != LUA_TTABLE) {
        LOG_LUA_LINE("Tried to attach a non-table to hook_on_sync_table_change: %s", luaL_typename(L, syncTableIndex));
        return 0;
    }

    if (lua_type(L, funcIndex) != LUA_TFUNCTION) {
        LOG_LUA_LINE("Tried to attach a non-function to hook_on_sync_table_change: %s", luaL_typename(L, funcIndex));
        return 0;
    }

    // set hook's table
    lua_newtable(L);
    int valTableIndex = lua_gettop(L);

    lua_pushstring(L, "_func");
    lua_pushvalue(L, funcIndex);
    lua_settable(L, valTableIndex);

    lua_pushstring(L, "_tag");
    lua_pushvalue(L, tagIndex);
    lua_settable(L, valTableIndex);

    // get _hook_on_changed
    lua_pushstring(L, "_hook_on_changed");
    lua_rawget(L, syncTableIndex);
    int hookOnChangedIndex = lua_gettop(L);

    // attach
    lua_pushvalue(L, keyIndex);
    lua_pushvalue(L, valTableIndex);
    lua_settable(L, hookOnChangedIndex);

    // clean up
    lua_remove(L, hookOnChangedIndex);
    lua_remove(L, valTableIndex);

    LUA_STACK_CHECK_END(L);
    return 1;
}


  ////////////////////////////
 // hooked mod menu button //
////////////////////////////

struct LuaHookedModMenuElement gHookedModMenuElements[MAX_HOOKED_MOD_MENU_ELEMENTS] = { 0 };
int gHookedModMenuElementsCount = 0;

void smlua_call_mod_menu_element_hook(struct LuaHookedModMenuElement* hooked, int index);

static void smlua_mod_menu_init_element(struct LuaHookedModMenuElement* hooked, enum LuaModMenuElementType element, const char* name) {
    memset(hooked, 0, sizeof(*hooked));
    hooked->element = element;
    snprintf(hooked->name, sizeof(hooked->name), "%s", name);
    hooked->stringValue[0] = '\0';
    hooked->configKey[0] = '\0';
    for (int i = 0; i < MAX_BINDS; i++) {
        hooked->bindValue[i] = VK_INVALID;
        hooked->defaultBindValue[i] = VK_INVALID;
    }
    hooked->mod = gLuaActiveMod;
    hooked->modFile = gLuaActiveModFile;
}

static void smlua_mod_menu_generate_config_key(struct LuaHookedModMenuElement* hooked, const char* name) {
    char baseKey[sizeof(hooked->configKey)] = { 0 };
    size_t out = 0;

    for (const unsigned char* c = (const unsigned char*)name; *c != '\0' && out + 1 < sizeof(baseKey); c++) {
        if (isalnum(*c)) {
            baseKey[out++] = (char)tolower(*c);
        } else if (out > 0 && baseKey[out - 1] != '-') {
            baseKey[out++] = '-';
        }
    }

    while (out > 0 && baseKey[out - 1] == '-') {
        out--;
    }
    baseKey[out] = '\0';

    if (baseKey[0] == '\0') {
        snprintf(baseKey, sizeof(baseKey), "control");
    }

    snprintf(hooked->configKey, sizeof(hooked->configKey), "%s", baseKey);

    int duplicateCount = 1;
    for (int i = 0; i < gHookedModMenuElementsCount; i++) {
        struct LuaHookedModMenuElement* other = &gHookedModMenuElements[i];
        if (other->mod != hooked->mod) { continue; }
        if (strcmp(other->configKey, hooked->configKey) == 0) {
            duplicateCount++;
        }
    }

    if (duplicateCount > 1) {
        snprintf(hooked->configKey, sizeof(hooked->configKey), "%s-%d", baseKey, duplicateCount);
    }
}

static bool smlua_mod_menu_read_bind_table(lua_State* L, int index, unsigned int bindValue[MAX_BINDS]) {
    gSmLuaConvertSuccess = true;
    if (lua_type(L, index) != LUA_TTABLE) {
        LOG_LUA_LINE("Hook mod menu bind: expected a table of bind values");
        gSmLuaConvertSuccess = false;
        return false;
    }

    for (int i = 0; i < MAX_BINDS; i++) {
        lua_rawgeti(L, index, i + 1);
        if (lua_type(L, -1) == LUA_TNIL) {
            bindValue[i] = VK_INVALID;
        } else {
            bindValue[i] = (unsigned int)smlua_to_integer(L, -1);
            if (!gSmLuaConvertSuccess) {
                lua_pop(L, 1);
                return false;
            }
        }
        lua_pop(L, 1);
    }

    return true;
}

static void smlua_mod_menu_push_bind_table(lua_State* L, const unsigned int bindValue[MAX_BINDS]) {
    lua_newtable(L);
    for (int i = 0; i < MAX_BINDS; i++) {
        lua_pushinteger(L, bindValue[i]);
        lua_rawseti(L, -2, i + 1);
    }
}

int smlua_hook_mod_menu_text(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 1)) { return 0; }

    if (gHookedModMenuElementsCount >= MAX_HOOKED_MOD_MENU_ELEMENTS) {
        LOG_LUA_LINE("Hooked mod menu element exceeded maximum references!");
        return 0;
    }

    const char* name = smlua_to_string(L, 1);
    if (name == NULL || strlen(name) == 0 || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook mod menu element: tried to hook invalid element");
        return 0;
    }

    struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[gHookedModMenuElementsCount];
    smlua_mod_menu_init_element(hooked, MOD_MENU_ELEMENT_TEXT, name);

    lua_pushinteger(L, gHookedModMenuElementsCount);
    gHookedModMenuElementsCount++;
    return 1;
}

int smlua_hook_mod_menu_button(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 2)) { return 0; }

    if (gHookedModMenuElementsCount >= MAX_HOOKED_MOD_MENU_ELEMENTS) {
        LOG_LUA_LINE("Hooked mod menu element exceeded maximum references!");
        return 0;
    }

    const char* name = smlua_to_string(L, 1);
    if (name == NULL || strlen(name) == 0 || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook mod menu element: tried to hook invalid element");
        return 0;
    }

    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (ref == -1) {
        LOG_LUA_LINE("Hook mod menu element: tried to hook undefined function '%s'", gLuaActiveMod->name);
        return 0;
    }

    struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[gHookedModMenuElementsCount];
    smlua_mod_menu_init_element(hooked, MOD_MENU_ELEMENT_BUTTON, name);
    hooked->reference = ref;

    lua_pushinteger(L, gHookedModMenuElementsCount);
    gHookedModMenuElementsCount++;
    return 1;
}

int smlua_hook_mod_menu_checkbox(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 3)) { return 0; }

    if (gHookedModMenuElementsCount >= MAX_HOOKED_MOD_MENU_ELEMENTS) {
        LOG_LUA_LINE("Hooked mod menu element exceeded maximum references!");
        return 0;
    }

    const char* name = smlua_to_string(L, 1);
    if (name == NULL || strlen(name) == 0 || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook mod menu element: tried to hook invalid element");
        return 0;
    }

    bool defaultValue = smlua_to_boolean(L, 2);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook mod menu element: tried to hook invalid element");
        return 0;
    }

    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (ref == -1) {
        LOG_LUA_LINE("Hook mod menu element: tried to hook undefined function '%s'", gLuaActiveMod->name);
        return 0;
    }

    struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[gHookedModMenuElementsCount];
    smlua_mod_menu_init_element(hooked, MOD_MENU_ELEMENT_CHECKBOX, name);
    hooked->boolValue = defaultValue;
    hooked->reference = ref;

    lua_pushinteger(L, gHookedModMenuElementsCount);
    gHookedModMenuElementsCount++;
    return 1;
}

int smlua_hook_mod_menu_slider(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 5)) { return 0; }

    if (gHookedModMenuElementsCount >= MAX_HOOKED_MOD_MENU_ELEMENTS) {
        LOG_LUA_LINE("Hooked mod menu element exceeded maximum references!");
        return 0;
    }

    const char* name = smlua_to_string(L, 1);
    if (name == NULL || strlen(name) == 0 || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook mod menu element: tried to hook invalid element");
        return 0;
    }

    u32 defaultValue = smlua_to_integer(L, 2);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook mod menu element: tried to hook invalid element");
        return 0;
    }

    u32 sliderMin = smlua_to_integer(L, 3);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook mod menu element: tried to hook invalid element");
        return 0;
    }

    u32 sliderMax = smlua_to_integer(L, 4);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook mod menu element: tried to hook invalid element");
        return 0;
    }

    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (ref == -1) {
        LOG_LUA_LINE("Hook mod menu element: tried to hook undefined function '%s'", gLuaActiveMod->name);
        return 0;
    }

    struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[gHookedModMenuElementsCount];
    smlua_mod_menu_init_element(hooked, MOD_MENU_ELEMENT_SLIDER, name);
    hooked->uintValue = defaultValue;
    hooked->sliderMin = sliderMin;
    hooked->sliderMax = sliderMax;
    hooked->reference = ref;

    lua_pushinteger(L, gHookedModMenuElementsCount);
    gHookedModMenuElementsCount++;
    return 1;
}

int smlua_hook_mod_menu_inputbox(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 4)) { return 0; }

    if (gHookedModMenuElementsCount >= MAX_HOOKED_MOD_MENU_ELEMENTS) {
        LOG_LUA_LINE("Hooked mod menu element exceeded maximum references!");
        return 0;
    }

    const char* name = smlua_to_string(L, 1);
    if (name == NULL || strlen(name) == 0 || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook mod menu element: tried to hook invalid element");
        return 0;
    }

    const char* defaultValue = smlua_to_string(L, 2);
    if (defaultValue == NULL || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook mod menu element: tried to hook invalid element");
        return 0;
    }

    u32 length = smlua_to_integer(L, 3);
    length = MIN(length, 256);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook mod menu element: tried to hook invalid element");
        return 0;
    }

    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (ref == -1) {
        LOG_LUA_LINE("Hook mod menu element: tried to hook undefined function '%s'", gLuaActiveMod->name);
        return 0;
    }

    struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[gHookedModMenuElementsCount];
    smlua_mod_menu_init_element(hooked, MOD_MENU_ELEMENT_INPUTBOX, name);
    snprintf(hooked->stringValue, 256, "%s", defaultValue);
    hooked->length = length;
    hooked->reference = ref;

    lua_pushinteger(L, gHookedModMenuElementsCount);
    gHookedModMenuElementsCount++;
    return 1;
}

int smlua_hook_mod_menu_bind(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 3)) { return 0; }

    if (gHookedModMenuElementsCount >= MAX_HOOKED_MOD_MENU_ELEMENTS) {
        LOG_LUA_LINE("Hooked mod menu element exceeded maximum references!");
        return 0;
    }

    const char* name = smlua_to_string(L, 1);
    if (name == NULL || strlen(name) == 0 || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Hook mod menu bind: tried to hook invalid element");
        return 0;
    }

    unsigned int bindValue[MAX_BINDS] = { VK_INVALID, VK_INVALID, VK_INVALID };
    if (!smlua_mod_menu_read_bind_table(L, 2, bindValue)) {
        LOG_LUA_LINE("Hook mod menu bind: tried to use invalid default bind table");
        return 0;
    }

    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    if (ref == -1) {
        LOG_LUA_LINE("Hook mod menu bind: tried to hook undefined function '%s'", gLuaActiveMod->name);
        return 0;
    }

    int index = gHookedModMenuElementsCount;
    struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[index];
    smlua_mod_menu_init_element(hooked, MOD_MENU_ELEMENT_BIND, name);
    hooked->reference = ref;
    memcpy(hooked->bindValue, bindValue, sizeof(hooked->bindValue));
    memcpy(hooked->defaultBindValue, bindValue, sizeof(hooked->defaultBindValue));
    smlua_mod_menu_generate_config_key(hooked, name);

    if (hooked->mod != NULL && hooked->configKey[0] != '\0') {
        mod_bindings_get(hooked->mod->relativePath, hooked->configKey, hooked->bindValue);
    }

    gHookedModMenuElementsCount++;
    smlua_call_mod_menu_element_hook(hooked, index);
    lua_pushinteger(L, index);
    return 1;
}

int smlua_update_mod_menu_element_name(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 2)) { return 0; }

    int index = smlua_to_integer(L, 1);
    if (index >= gHookedModMenuElementsCount || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Update mod menu element: tried to update invalid element");
        return 0;
    }

    const char* name = smlua_to_string(L, 2);
    if (name == NULL || strlen(name) == 0 || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Update mod menu element: tried to update invalid name");
        return 0;
    }

    snprintf(gHookedModMenuElements[index].name, 64, "%s", name);
    return 1;
}

int smlua_update_mod_menu_element_checkbox(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 2)) { return 0; }

    int index = smlua_to_integer(L, 1);
    if (index >= gHookedModMenuElementsCount || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Update mod menu element: tried to update invalid element");
        return 0;
    }

    if (gHookedModMenuElements[index].element != MOD_MENU_ELEMENT_CHECKBOX) {
        LOG_LUA_LINE("Update mod menu element: element is not a checkbox.");
        return 0;
    }

    bool boolValue = smlua_to_boolean(L, 2);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Update mod menu element: tried to update invalid element");
        return 0;
    }

    gHookedModMenuElements[index].boolValue = boolValue;
    return 1;
}

int smlua_update_mod_menu_element_slider(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 2)) { return 0; }

    int index = smlua_to_integer(L, 1);
    if (index >= gHookedModMenuElementsCount || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Update mod menu element: tried to update invalid element");
        return 0;
    }

    if (gHookedModMenuElements[index].element != MOD_MENU_ELEMENT_SLIDER) {
        LOG_LUA_LINE("Update mod menu element: element is not a slider.");
        return 0;
    }

    u32 uintValue = smlua_to_integer(L, 2);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Update mod menu element: tried to update invalid element");
        return 0;
    }

    gHookedModMenuElements[index].uintValue = uintValue;
    return 1;
}

int smlua_update_mod_menu_element_bind(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 2)) { return 0; }

    int index = smlua_to_integer(L, 1);
    if (index >= gHookedModMenuElementsCount || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Update mod menu element: tried to update invalid element");
        return 0;
    }

    if (gHookedModMenuElements[index].element != MOD_MENU_ELEMENT_BIND) {
        LOG_LUA_LINE("Update mod menu element: element is not a bind.");
        return 0;
    }

    if (!smlua_mod_menu_read_bind_table(L, 2, gHookedModMenuElements[index].bindValue)) {
        LOG_LUA_LINE("Update mod menu element: tried to update invalid bind table");
        return 0;
    }

    struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[index];
    if (hooked->mod != NULL && hooked->configKey[0] != '\0') {
        mod_bindings_set(hooked->mod->relativePath, hooked->configKey, hooked->bindValue);
    }
    return 1;
}

int smlua_update_mod_menu_element_inputbox(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 2)) { return 0; }

    int index = smlua_to_integer(L, 1);
    if (index >= gHookedModMenuElementsCount || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Update mod menu element: tried to update invalid element");
        return 0;
    }

    if (gHookedModMenuElements[index].element != MOD_MENU_ELEMENT_INPUTBOX) {
        LOG_LUA_LINE("Update mod menu element: element is not an inputbox.");
        return 0;
    }

    const char* stringValue = smlua_to_string(L, 2);
    if (stringValue == NULL || strlen(stringValue) == 0 || !gSmLuaConvertSuccess) {
        LOG_LUA_LINE("Update mod menu element: tried to update invalid element string");
        return 0;
    }

    snprintf(gHookedModMenuElements[index].stringValue, gHookedModMenuElements[index].length, "%s", stringValue);
    return 1;
}

static int smlua_mod_bind_query(lua_State* L, bool (*query)(const unsigned int bindValue[MAX_BINDS]), const char* name) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 1)) { return 0; }

    unsigned int bindValue[MAX_BINDS] = { VK_INVALID, VK_INVALID, VK_INVALID };
    if (!smlua_mod_menu_read_bind_table(L, 1, bindValue)) {
        LOG_LUA_LINE("%s: tried to use an invalid bind table", name);
        return 0;
    }

    lua_pushboolean(L, query(bindValue));
    return 1;
}

int smlua_mod_bind_pressed(lua_State* L) {
    return smlua_mod_bind_query(L, controller_bind_pressed, "mod_bind_pressed");
}

int smlua_mod_bind_down(lua_State* L) {
    return smlua_mod_bind_query(L, controller_bind_down, "mod_bind_down");
}

int smlua_mod_bind_released(lua_State* L) {
    return smlua_mod_bind_query(L, controller_bind_released, "mod_bind_released");
}

int smlua_is_game_menu_open(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }

    lua_pushboolean(L, mxui_is_pause_active());
    return 1;
}

int smlua_is_character_select_menu_open(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }

    lua_pushboolean(L, mxui_is_character_select_active());
    return 1;
}

int smlua_is_frontend_menu_open(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }

    lua_pushboolean(L, mxui_is_main_menu_active());
    return 1;
}

int smlua_game_menu_open(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }

    lua_pushboolean(L, mxui_try_open_pause_menu());
    return 1;
}

int smlua_game_menu_close(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }

    bool closed = false;
    if (mxui_is_pause_active()) {
        mxui_close_pause_menu_with_mode(1);
        closed = true;
    }
    lua_pushboolean(L, closed);
    return 1;
}

int smlua_game_menu_open_character_select(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }

    lua_pushboolean(L, mxui_open_character_select_menu());
    return 1;
}

int smlua_mxui_popup_create(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_range(L, 1, 2)) { return 0; }

    const char* message = smlua_to_string(L, 1);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("mxui_popup_create: invalid message");
        return 0;
    }

    int lines = 1;
    if (lua_gettop(L) >= 2) {
        lines = smlua_to_integer(L, 2);
        if (!gSmLuaConvertSuccess) {
            LOG_LUA_LINE("mxui_popup_create: invalid line count");
            return 0;
        }
    }

    mxui_popup_create(message, lines);
    return 1;
}

int smlua_mxui_popup_create_global(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_range(L, 1, 2)) { return 0; }

    const char* message = smlua_to_string(L, 1);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("mxui_popup_create_global: invalid message");
        return 0;
    }

    int lines = 1;
    if (lua_gettop(L) >= 2) {
        lines = smlua_to_integer(L, 2);
        if (!gSmLuaConvertSuccess) {
            LOG_LUA_LINE("mxui_popup_create_global: invalid line count");
            return 0;
        }
    }

    mxui_popup_create_global(message, lines);
    return 1;
}

static void smlua_mxui_hud_push_color(lua_State* L, const struct MxuiHudColor* color) {
    lua_createtable(L, 0, 4);
    lua_pushinteger(L, color != NULL ? color->r : 255);
    lua_setfield(L, -2, "r");
    lua_pushinteger(L, color != NULL ? color->g : 255);
    lua_setfield(L, -2, "g");
    lua_pushinteger(L, color != NULL ? color->b : 255);
    lua_setfield(L, -2, "b");
    lua_pushinteger(L, color != NULL ? color->a : 255);
    lua_setfield(L, -2, "a");
}

int smlua_mxui_hud_set_resolution(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 1)) { return 0; }

    u8 resolutionType = (u8)smlua_to_integer(L, 1);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("mxui_hud_set_resolution: invalid resolution");
        return 0;
    }

    mxui_hud_set_resolution(resolutionType);
    return 0;
}

int smlua_mxui_hud_get_filter(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    lua_pushinteger(L, mxui_hud_get_filter());
    return 1;
}

int smlua_mxui_hud_set_filter(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 1)) { return 0; }

    u8 filterType = (u8)smlua_to_integer(L, 1);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("mxui_hud_set_filter: invalid filter");
        return 0;
    }

    mxui_hud_set_filter(filterType);
    return 0;
}

int smlua_mxui_hud_get_screen_width(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    lua_pushinteger(L, mxui_hud_get_screen_width());
    return 1;
}

int smlua_mxui_hud_get_screen_height(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    lua_pushinteger(L, mxui_hud_get_screen_height());
    return 1;
}

int smlua_mxui_hud_get_mouse_x(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    lua_pushnumber(L, mxui_hud_get_mouse_x());
    return 1;
}

int smlua_mxui_hud_get_mouse_y(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    lua_pushnumber(L, mxui_hud_get_mouse_y());
    return 1;
}

int smlua_mxui_hud_get_raw_mouse_x(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    lua_pushnumber(L, mxui_hud_get_raw_mouse_x());
    return 1;
}

int smlua_mxui_hud_get_raw_mouse_y(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    lua_pushnumber(L, mxui_hud_get_raw_mouse_y());
    return 1;
}

int smlua_mxui_hud_is_mouse_locked(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    lua_pushboolean(L, mxui_hud_is_mouse_locked());
    return 1;
}

int smlua_mxui_hud_set_mouse_locked(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 1)) { return 0; }

    bool locked = smlua_to_boolean(L, 1);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("mxui_hud_set_mouse_locked: invalid locked flag");
        return 0;
    }

    mxui_hud_set_mouse_locked(locked);
    return 0;
}

int smlua_mxui_hud_get_mouse_buttons_down(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    lua_pushinteger(L, mxui_hud_get_mouse_buttons_down());
    return 1;
}

int smlua_mxui_hud_get_mouse_buttons_pressed(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    lua_pushinteger(L, mxui_hud_get_mouse_buttons_pressed());
    return 1;
}

int smlua_mxui_hud_get_mouse_buttons_released(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    lua_pushinteger(L, mxui_hud_get_mouse_buttons_released());
    return 1;
}

int smlua_mxui_hud_get_mouse_scroll_x(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    lua_pushnumber(L, mxui_hud_get_mouse_scroll_x());
    return 1;
}

int smlua_mxui_hud_set_font(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 1)) { return 0; }

    s8 fontType = (s8)smlua_to_integer(L, 1);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("mxui_hud_set_font: invalid font");
        return 0;
    }

    mxui_hud_set_font(fontType);
    return 0;
}

int smlua_mxui_hud_get_font(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    lua_pushinteger(L, mxui_hud_get_font());
    return 1;
}

int smlua_mxui_hud_set_color(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 4)) { return 0; }

    u8 r = (u8)smlua_to_integer(L, 1);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_set_color: invalid red"); return 0; }
    u8 g = (u8)smlua_to_integer(L, 2);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_set_color: invalid green"); return 0; }
    u8 b = (u8)smlua_to_integer(L, 3);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_set_color: invalid blue"); return 0; }
    u8 a = (u8)smlua_to_integer(L, 4);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_set_color: invalid alpha"); return 0; }

    mxui_hud_set_color(r, g, b, a);
    return 0;
}

int smlua_mxui_hud_get_color(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    smlua_mxui_hud_push_color(L, mxui_hud_get_color());
    return 1;
}

int smlua_mxui_hud_reset_color(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    mxui_hud_reset_color();
    return 0;
}

int smlua_mxui_hud_set_rotation(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 3)) { return 0; }

    s16 rotation = (s16)smlua_to_integer(L, 1);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_set_rotation: invalid rotation"); return 0; }
    f32 pivotX = smlua_to_number(L, 2);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_set_rotation: invalid pivotX"); return 0; }
    f32 pivotY = smlua_to_number(L, 3);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_set_rotation: invalid pivotY"); return 0; }

    mxui_hud_set_rotation(rotation, pivotX, pivotY);
    return 0;
}

int smlua_mxui_hud_measure_text(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 1)) { return 0; }

    const char* message = smlua_to_string(L, 1);
    if (!gSmLuaConvertSuccess) {
        LOG_LUA_LINE("mxui_hud_measure_text: invalid message");
        return 0;
    }

    lua_pushnumber(L, mxui_hud_measure_text(message));
    return 1;
}

int smlua_mxui_hud_print_text(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 4)) { return 0; }

    const char* message = smlua_to_string(L, 1);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_print_text: invalid message"); return 0; }
    f32 x = smlua_to_number(L, 2);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_print_text: invalid x"); return 0; }
    f32 y = smlua_to_number(L, 3);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_print_text: invalid y"); return 0; }
    f32 scale = smlua_to_number(L, 4);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_print_text: invalid scale"); return 0; }

    mxui_hud_print_text(message, x, y, scale);
    return 0;
}

int smlua_mxui_hud_print_text_interpolated(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 7)) { return 0; }

    const char* message = smlua_to_string(L, 1);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_print_text_interpolated: invalid message"); return 0; }
    f32 prevX = smlua_to_number(L, 2);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_print_text_interpolated: invalid prevX"); return 0; }
    f32 prevY = smlua_to_number(L, 3);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_print_text_interpolated: invalid prevY"); return 0; }
    f32 prevScale = smlua_to_number(L, 4);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_print_text_interpolated: invalid prevScale"); return 0; }
    f32 x = smlua_to_number(L, 5);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_print_text_interpolated: invalid x"); return 0; }
    f32 y = smlua_to_number(L, 6);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_print_text_interpolated: invalid y"); return 0; }
    f32 scale = smlua_to_number(L, 7);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_print_text_interpolated: invalid scale"); return 0; }

    mxui_hud_print_text_interpolated(message, prevX, prevY, prevScale, x, y, scale);
    return 0;
}

int smlua_mxui_hud_render_texture(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 5)) { return 0; }

    struct TextureInfo* texInfo = smlua_to_texture_info(L, 1);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture: invalid texture"); return 0; }
    f32 x = smlua_to_number(L, 2);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture: invalid x"); return 0; }
    f32 y = smlua_to_number(L, 3);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture: invalid y"); return 0; }
    f32 scaleW = smlua_to_number(L, 4);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture: invalid scaleW"); return 0; }
    f32 scaleH = smlua_to_number(L, 5);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture: invalid scaleH"); return 0; }

    mxui_hud_render_texture(texInfo, x, y, scaleW, scaleH);
    return 0;
}

int smlua_mxui_hud_render_texture_tile(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 9)) { return 0; }

    struct TextureInfo* texInfo = smlua_to_texture_info(L, 1);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile: invalid texture"); return 0; }
    f32 x = smlua_to_number(L, 2);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile: invalid x"); return 0; }
    f32 y = smlua_to_number(L, 3);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile: invalid y"); return 0; }
    f32 scaleW = smlua_to_number(L, 4);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile: invalid scaleW"); return 0; }
    f32 scaleH = smlua_to_number(L, 5);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile: invalid scaleH"); return 0; }
    u32 tileX = (u32)smlua_to_integer(L, 6);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile: invalid tileX"); return 0; }
    u32 tileY = (u32)smlua_to_integer(L, 7);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile: invalid tileY"); return 0; }
    u32 tileW = (u32)smlua_to_integer(L, 8);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile: invalid tileW"); return 0; }
    u32 tileH = (u32)smlua_to_integer(L, 9);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile: invalid tileH"); return 0; }

    mxui_hud_render_texture_tile(texInfo, x, y, scaleW, scaleH, tileX, tileY, tileW, tileH);
    return 0;
}

int smlua_mxui_hud_render_texture_interpolated(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 9)) { return 0; }

    struct TextureInfo* texInfo = smlua_to_texture_info(L, 1);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_interpolated: invalid texture"); return 0; }
    f32 prevX = smlua_to_number(L, 2);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_interpolated: invalid prevX"); return 0; }
    f32 prevY = smlua_to_number(L, 3);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_interpolated: invalid prevY"); return 0; }
    f32 prevScaleW = smlua_to_number(L, 4);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_interpolated: invalid prevScaleW"); return 0; }
    f32 prevScaleH = smlua_to_number(L, 5);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_interpolated: invalid prevScaleH"); return 0; }
    f32 x = smlua_to_number(L, 6);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_interpolated: invalid x"); return 0; }
    f32 y = smlua_to_number(L, 7);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_interpolated: invalid y"); return 0; }
    f32 scaleW = smlua_to_number(L, 8);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_interpolated: invalid scaleW"); return 0; }
    f32 scaleH = smlua_to_number(L, 9);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_interpolated: invalid scaleH"); return 0; }

    mxui_hud_render_texture_interpolated(texInfo, prevX, prevY, prevScaleW, prevScaleH, x, y, scaleW, scaleH);
    return 0;
}

int smlua_mxui_hud_render_texture_tile_interpolated(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 13)) { return 0; }

    struct TextureInfo* texInfo = smlua_to_texture_info(L, 1);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile_interpolated: invalid texture"); return 0; }
    f32 prevX = smlua_to_number(L, 2);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile_interpolated: invalid prevX"); return 0; }
    f32 prevY = smlua_to_number(L, 3);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile_interpolated: invalid prevY"); return 0; }
    f32 prevScaleW = smlua_to_number(L, 4);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile_interpolated: invalid prevScaleW"); return 0; }
    f32 prevScaleH = smlua_to_number(L, 5);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile_interpolated: invalid prevScaleH"); return 0; }
    f32 x = smlua_to_number(L, 6);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile_interpolated: invalid x"); return 0; }
    f32 y = smlua_to_number(L, 7);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile_interpolated: invalid y"); return 0; }
    f32 scaleW = smlua_to_number(L, 8);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile_interpolated: invalid scaleW"); return 0; }
    f32 scaleH = smlua_to_number(L, 9);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile_interpolated: invalid scaleH"); return 0; }
    u32 tileX = (u32)smlua_to_integer(L, 10);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile_interpolated: invalid tileX"); return 0; }
    u32 tileY = (u32)smlua_to_integer(L, 11);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile_interpolated: invalid tileY"); return 0; }
    u32 tileW = (u32)smlua_to_integer(L, 12);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile_interpolated: invalid tileW"); return 0; }
    u32 tileH = (u32)smlua_to_integer(L, 13);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_texture_tile_interpolated: invalid tileH"); return 0; }

    mxui_hud_render_texture_tile_interpolated(texInfo, prevX, prevY, prevScaleW, prevScaleH, x, y, scaleW, scaleH, tileX, tileY, tileW, tileH);
    return 0;
}

int smlua_mxui_hud_render_rect(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 4)) { return 0; }

    f32 x = smlua_to_number(L, 1);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_rect: invalid x"); return 0; }
    f32 y = smlua_to_number(L, 2);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_rect: invalid y"); return 0; }
    f32 width = smlua_to_number(L, 3);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_rect: invalid width"); return 0; }
    f32 height = smlua_to_number(L, 4);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_rect: invalid height"); return 0; }

    mxui_hud_render_rect(x, y, width, height);
    return 0;
}

int smlua_mxui_hud_render_rect_interpolated(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 8)) { return 0; }

    f32 prevX = smlua_to_number(L, 1);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_rect_interpolated: invalid prevX"); return 0; }
    f32 prevY = smlua_to_number(L, 2);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_rect_interpolated: invalid prevY"); return 0; }
    f32 prevWidth = smlua_to_number(L, 3);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_rect_interpolated: invalid prevWidth"); return 0; }
    f32 prevHeight = smlua_to_number(L, 4);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_rect_interpolated: invalid prevHeight"); return 0; }
    f32 x = smlua_to_number(L, 5);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_rect_interpolated: invalid x"); return 0; }
    f32 y = smlua_to_number(L, 6);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_rect_interpolated: invalid y"); return 0; }
    f32 width = smlua_to_number(L, 7);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_rect_interpolated: invalid width"); return 0; }
    f32 height = smlua_to_number(L, 8);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_rect_interpolated: invalid height"); return 0; }

    mxui_hud_render_rect_interpolated(prevX, prevY, prevWidth, prevHeight, x, y, width, height);
    return 0;
}

int smlua_mxui_hud_set_scissor(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 4)) { return 0; }

    f32 x = smlua_to_number(L, 1);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_set_scissor: invalid x"); return 0; }
    f32 y = smlua_to_number(L, 2);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_set_scissor: invalid y"); return 0; }
    f32 width = smlua_to_number(L, 3);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_set_scissor: invalid width"); return 0; }
    f32 height = smlua_to_number(L, 4);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_set_scissor: invalid height"); return 0; }

    mxui_hud_set_scissor(x, y, width, height);
    return 0;
}

int smlua_mxui_hud_set_viewport(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 4)) { return 0; }

    f32 x = smlua_to_number(L, 1);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_set_viewport: invalid x"); return 0; }
    f32 y = smlua_to_number(L, 2);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_set_viewport: invalid y"); return 0; }
    f32 width = smlua_to_number(L, 3);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_set_viewport: invalid width"); return 0; }
    f32 height = smlua_to_number(L, 4);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_set_viewport: invalid height"); return 0; }

    mxui_hud_set_viewport(x, y, width, height);
    return 0;
}

int smlua_mxui_hud_reset_viewport(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    mxui_hud_reset_viewport();
    return 0;
}

int smlua_mxui_hud_reset_scissor(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    mxui_hud_reset_scissor();
    return 0;
}

int smlua_mxui_hud_render_line(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 5)) { return 0; }

    f32 p1X = smlua_to_number(L, 1);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_line: invalid p1X"); return 0; }
    f32 p1Y = smlua_to_number(L, 2);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_line: invalid p1Y"); return 0; }
    f32 p2X = smlua_to_number(L, 3);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_line: invalid p2X"); return 0; }
    f32 p2Y = smlua_to_number(L, 4);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_line: invalid p2Y"); return 0; }
    f32 size = smlua_to_number(L, 5);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_render_line: invalid size"); return 0; }

    mxui_hud_render_line(p1X, p1Y, p2X, p2Y, size);
    return 0;
}

int smlua_mxui_hud_get_mouse_scroll_y(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    lua_pushnumber(L, mxui_hud_get_mouse_scroll_y());
    return 1;
}

int smlua_mxui_hud_get_fov_coeff(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    lua_pushnumber(L, mxui_hud_get_fov_coeff());
    return 1;
}

int smlua_mxui_hud_world_pos_to_screen_pos(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 2)) { return 0; }

    extern void smlua_get_vec3f(Vec3f dest, int index);
    extern void smlua_push_vec3f(Vec3f src, int index);

    Vec3f pos;
    smlua_get_vec3f(pos, 1);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_world_pos_to_screen_pos: invalid position"); return 0; }

    Vec3f out;
    smlua_get_vec3f(out, 2);
    if (!gSmLuaConvertSuccess) { LOG_LUA_LINE("mxui_hud_world_pos_to_screen_pos: invalid output"); return 0; }

    lua_pushboolean(L, mxui_hud_world_pos_to_screen_pos(pos, out));
    smlua_push_vec3f(out, 2);
    return 1;
}

int smlua_mxui_hud_is_pause_menu_created(lua_State* L) {
    if (L == NULL) { return 0; }
    if (!smlua_functions_valid_param_count(L, 0)) { return 0; }
    lua_pushboolean(L, mxui_hud_is_pause_menu_created());
    return 1;
}

void smlua_call_mod_menu_element_hook(struct LuaHookedModMenuElement* hooked, int index) {
    lua_State* L = gLuaState;
    if (L == NULL) { return; }
    if (hooked == NULL || hooked->reference == 0) { return; }

    // push the callback onto the stack
    lua_rawgeti(L, LUA_REGISTRYINDEX, hooked->reference);

    // push parameter
    u8 params = 2;
    lua_pushinteger(L, index);
    switch (hooked->element) {
        case MOD_MENU_ELEMENT_TEXT:
            params = 1;
        case MOD_MENU_ELEMENT_BUTTON:
            params = 1;
            break;
        case MOD_MENU_ELEMENT_CHECKBOX:
            lua_pushboolean(L, hooked->boolValue);
            break;
        case MOD_MENU_ELEMENT_SLIDER:
            lua_pushinteger(L, hooked->uintValue);
            break;
        case MOD_MENU_ELEMENT_INPUTBOX:
            lua_pushstring(L, hooked->stringValue);
            break;
        case MOD_MENU_ELEMENT_BIND:
            smlua_mod_menu_push_bind_table(L, hooked->bindValue);
            break;
        case MOD_MENU_ELEMENT_MAX:
            break;
    }

    // call the callback
    if (0 != smlua_call_hook(L, params, 1, 0, hooked->mod, hooked->modFile)) {
        LOG_LUA("Failed to call the mod menu element callback: %s", hooked->name);
        return;
    }
    lua_pop(L, 1);
}


  //////////
 // misc //
//////////

static void smlua_hook_replace_function_reference(lua_State* L, int* hookedReference, int oldReference, int newReference) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, *hookedReference);   // stack: ..., hookedFunc
    int hookedIdx = lua_gettop(L);

    lua_rawgeti(L, LUA_REGISTRYINDEX, oldReference);    // stack: ..., hookedFunc, oldFunc
    int oldIdx = lua_gettop(L);

    if (lua_rawequal(L, hookedIdx, oldIdx)) {
        luaL_unref(L, LUA_REGISTRYINDEX, *hookedReference);
        *hookedReference = newReference;
    }

    lua_pop(L, 2);
}

void smlua_hook_replace_function_references(lua_State* L, int oldReference, int newReference) {
    for (int i = 0; i < HOOK_MAX; i++) {
        struct LuaHookedEvent* hooked = &sHookedEvents[i];
        for (int j = 0; j < hooked->count; j++) {
            smlua_hook_replace_function_reference(L, &hooked->reference[j], oldReference, newReference);
        }
    }

    for (int i = 0; i < sHookedMarioActionsCount; i++) {
        struct LuaHookedMarioAction* hooked = &sHookedMarioActions[i];
        for (int j = 0; j < ACTION_HOOK_MAX; j++) {
            smlua_hook_replace_function_reference(L, &hooked->actionHookRefs[j], oldReference, newReference);
        }
    }

    for (int i = 0; i < sHookedChatCommandsCount; i++) {
        struct LuaHookedChatCommand* hooked = &sHookedChatCommands[i];
        smlua_hook_replace_function_reference(L, &hooked->reference, oldReference, newReference);
    }

    for (int i = 0; i < gHookedModMenuElementsCount; i++) {
        struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[i];
        smlua_hook_replace_function_reference(L, &hooked->reference, oldReference, newReference);
    }

    for (int i = 0; i < sHookedBehaviorsCount; i++) {
        struct LuaHookedBehavior* hooked = &sHookedBehaviors[i];
        smlua_hook_replace_function_reference(L, &hooked->initReference, oldReference, newReference);
        smlua_hook_replace_function_reference(L, &hooked->loopReference, oldReference, newReference);
    }
}

void smlua_clear_hooks(void) {
    for (int i = 0; i < HOOK_MAX; i++) {
        struct LuaHookedEvent* hooked = &sHookedEvents[i];
        for (int j = 0; j < hooked->count; j++) {
            hooked->reference[j] = 0;
            hooked->mod[j] = NULL;
        }
        hooked->count = 0;
    }

    for (int i = 0; i < sHookedMarioActionsCount; i++) {
        struct LuaHookedMarioAction* hooked = &sHookedMarioActions[i];
        hooked->action = 0;
        hooked->mod = NULL;
        hooked->modFile = NULL;
        memset(hooked->actionHookRefs, 0, sizeof(hooked->actionHookRefs));
    }
    sHookedMarioActionsCount = 0;

    for (int i = 0; i < sHookedChatCommandsCount; i++) {
        struct LuaHookedChatCommand* hooked = &sHookedChatCommands[i];
        if (hooked->command != NULL) { free(hooked->command); }
        hooked->command = NULL;

        if (hooked->description != NULL) { free(sHookedChatCommands[i].description); }
        hooked->description = NULL;

        hooked->reference = 0;
        hooked->mod = NULL;
        hooked->modFile = NULL;
    }
    sHookedChatCommandsCount = 0;

    for (int i = 0; i < gHookedModMenuElementsCount; i++) {
        struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[i];
        hooked->element = MOD_MENU_ELEMENT_TEXT;
        hooked->name[0] = '\0';
        hooked->configKey[0] = '\0';
        hooked->boolValue = false;
        hooked->uintValue = 0;
        hooked->stringValue[0] = '\0';
        hooked->length = 0;
        hooked->sliderMin = 0;
        hooked->sliderMax = 0;
        for (int j = 0; j < MAX_BINDS; j++) {
            hooked->bindValue[j] = VK_INVALID;
        }
        hooked->reference = 0;
        hooked->mod = NULL;
        hooked->modFile = NULL;
    }
    gHookedModMenuElementsCount = 0;

    for (int i = 0; i < sHookedBehaviorsCount; i++) {
        struct LuaHookedBehavior* hooked = &sHookedBehaviors[i];

        // If this is NULL. We can't do anything with it.
        if (hooked->behavior != NULL) {
            // If it's a LUA made behavior, The behavior is allocated so reset and free it.
            // Otherwise it's a DynOS behavior and it needs to have it's original id put back where it belongs.
            if (hooked->luaBehavior) {
                // Just free the allocated behavior.
                free(hooked->behavior);
            } else {
                hooked->behavior[1] = (BehaviorScript)BC_B0H(0x39, hooked->originalId); // This is ID(hooked->originalId)
            }
        }
        // Reset the variables.
        hooked->behaviorId = 0;
        hooked->overrideId = 0;
        hooked->originalId = 0;
        hooked->behavior = NULL;
        hooked->originalBehavior = NULL;
        hooked->initReference = 0;
        hooked->loopReference = 0;
        hooked->replace = false;
        hooked->luaBehavior = false;
        hooked->mod = NULL;
        hooked->modFile = NULL;
    }
    sHookedBehaviorsCount = 0;
    memset(gLuaMarioActionIndex, 0, sizeof(gLuaMarioActionIndex));
}

void smlua_bind_hooks(void) {
    lua_State* L = gLuaState;
    smlua_clear_hooks();

    smlua_bind_function(L, "hook_event", smlua_hook_event);
    smlua_bind_function(L, "hook_mario_action", smlua_hook_mario_action);
    smlua_bind_function(L, "hook_chat_command", smlua_hook_chat_command);
    smlua_bind_function(L, "hook_on_sync_table_change", smlua_hook_on_sync_table_change);
    smlua_bind_function(L, "hook_behavior", smlua_hook_behavior);
    smlua_bind_function(L, "hook_mod_menu_text", smlua_hook_mod_menu_text);
    smlua_bind_function(L, "hook_mod_menu_button", smlua_hook_mod_menu_button);
    smlua_bind_function(L, "hook_mod_menu_checkbox", smlua_hook_mod_menu_checkbox);
    smlua_bind_function(L, "hook_mod_menu_slider", smlua_hook_mod_menu_slider);
    smlua_bind_function(L, "hook_mod_menu_inputbox", smlua_hook_mod_menu_inputbox);
    smlua_bind_function(L, "hook_mod_menu_bind", smlua_hook_mod_menu_bind);
    smlua_bind_function(L, "mod_bind_pressed", smlua_mod_bind_pressed);
    smlua_bind_function(L, "mod_bind_down", smlua_mod_bind_down);
    smlua_bind_function(L, "mod_bind_released", smlua_mod_bind_released);
    smlua_bind_function(L, "is_game_menu_open", smlua_is_game_menu_open);
    smlua_bind_function(L, "is_character_select_menu_open", smlua_is_character_select_menu_open);
    smlua_bind_function(L, "is_frontend_menu_open", smlua_is_frontend_menu_open);
    smlua_bind_function(L, "game_menu_open", smlua_game_menu_open);
    smlua_bind_function(L, "game_menu_close", smlua_game_menu_close);
    smlua_bind_function(L, "game_menu_open_character_select", smlua_game_menu_open_character_select);
    smlua_bind_function(L, "mxui_popup_create", smlua_mxui_popup_create);
    smlua_bind_function(L, "mxui_popup_create_global", smlua_mxui_popup_create_global);
    smlua_bind_function(L, "mxui_hud_set_resolution", smlua_mxui_hud_set_resolution);
    smlua_bind_function(L, "mxui_hud_get_filter", smlua_mxui_hud_get_filter);
    smlua_bind_function(L, "mxui_hud_set_filter", smlua_mxui_hud_set_filter);
    smlua_bind_function(L, "mxui_hud_get_screen_width", smlua_mxui_hud_get_screen_width);
    smlua_bind_function(L, "mxui_hud_get_screen_height", smlua_mxui_hud_get_screen_height);
    smlua_bind_function(L, "mxui_hud_get_mouse_x", smlua_mxui_hud_get_mouse_x);
    smlua_bind_function(L, "mxui_hud_get_mouse_y", smlua_mxui_hud_get_mouse_y);
    smlua_bind_function(L, "mxui_hud_get_raw_mouse_x", smlua_mxui_hud_get_raw_mouse_x);
    smlua_bind_function(L, "mxui_hud_get_raw_mouse_y", smlua_mxui_hud_get_raw_mouse_y);
    smlua_bind_function(L, "mxui_hud_is_mouse_locked", smlua_mxui_hud_is_mouse_locked);
    smlua_bind_function(L, "mxui_hud_set_mouse_locked", smlua_mxui_hud_set_mouse_locked);
    smlua_bind_function(L, "mxui_hud_get_mouse_buttons_down", smlua_mxui_hud_get_mouse_buttons_down);
    smlua_bind_function(L, "mxui_hud_get_mouse_buttons_pressed", smlua_mxui_hud_get_mouse_buttons_pressed);
    smlua_bind_function(L, "mxui_hud_get_mouse_buttons_released", smlua_mxui_hud_get_mouse_buttons_released);
    smlua_bind_function(L, "mxui_hud_get_mouse_scroll_x", smlua_mxui_hud_get_mouse_scroll_x);
    smlua_bind_function(L, "mxui_hud_set_font", smlua_mxui_hud_set_font);
    smlua_bind_function(L, "mxui_hud_get_font", smlua_mxui_hud_get_font);
    smlua_bind_function(L, "mxui_hud_set_color", smlua_mxui_hud_set_color);
    smlua_bind_function(L, "mxui_hud_get_color", smlua_mxui_hud_get_color);
    smlua_bind_function(L, "mxui_hud_reset_color", smlua_mxui_hud_reset_color);
    smlua_bind_function(L, "mxui_hud_set_rotation", smlua_mxui_hud_set_rotation);
    smlua_bind_function(L, "mxui_hud_measure_text", smlua_mxui_hud_measure_text);
    smlua_bind_function(L, "mxui_hud_print_text", smlua_mxui_hud_print_text);
    smlua_bind_function(L, "mxui_hud_print_text_interpolated", smlua_mxui_hud_print_text_interpolated);
    smlua_bind_function(L, "mxui_hud_render_texture", smlua_mxui_hud_render_texture);
    smlua_bind_function(L, "mxui_hud_render_texture_tile", smlua_mxui_hud_render_texture_tile);
    smlua_bind_function(L, "mxui_hud_render_texture_interpolated", smlua_mxui_hud_render_texture_interpolated);
    smlua_bind_function(L, "mxui_hud_render_texture_tile_interpolated", smlua_mxui_hud_render_texture_tile_interpolated);
    smlua_bind_function(L, "mxui_hud_render_rect", smlua_mxui_hud_render_rect);
    smlua_bind_function(L, "mxui_hud_render_rect_interpolated", smlua_mxui_hud_render_rect_interpolated);
    smlua_bind_function(L, "mxui_hud_render_line", smlua_mxui_hud_render_line);
    smlua_bind_function(L, "mxui_hud_set_viewport", smlua_mxui_hud_set_viewport);
    smlua_bind_function(L, "mxui_hud_reset_viewport", smlua_mxui_hud_reset_viewport);
    smlua_bind_function(L, "mxui_hud_set_scissor", smlua_mxui_hud_set_scissor);
    smlua_bind_function(L, "mxui_hud_reset_scissor", smlua_mxui_hud_reset_scissor);
    smlua_bind_function(L, "mxui_hud_get_mouse_scroll_y", smlua_mxui_hud_get_mouse_scroll_y);
    smlua_bind_function(L, "mxui_hud_get_fov_coeff", smlua_mxui_hud_get_fov_coeff);
    smlua_bind_function(L, "mxui_hud_world_pos_to_screen_pos", smlua_mxui_hud_world_pos_to_screen_pos);
    smlua_bind_function(L, "mxui_hud_is_pause_menu_created", smlua_mxui_hud_is_pause_menu_created);
    smlua_bind_function(L, "update_chat_command_description", smlua_update_chat_command_description);
    smlua_bind_function(L, "update_mod_menu_element_name", smlua_update_mod_menu_element_name);
    smlua_bind_function(L, "update_mod_menu_element_checkbox", smlua_update_mod_menu_element_checkbox);
    smlua_bind_function(L, "update_mod_menu_element_slider", smlua_update_mod_menu_element_slider);
    smlua_bind_function(L, "update_mod_menu_element_inputbox", smlua_update_mod_menu_element_inputbox);
    smlua_bind_function(L, "update_mod_menu_element_bind", smlua_update_mod_menu_element_bind);
}
