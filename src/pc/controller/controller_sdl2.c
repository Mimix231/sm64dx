#if defined(CAPI_SDL3) || defined(CAPI_SDL2)

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(HAVE_SDL3)
#include <SDL3/SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include <ultra64.h>

#include "controller_api.h"
#include "controller_sdl.h"
#include "controller_mouse.h"
#include "pc/pc_main.h"
#include "pc/configfile.h"
#include "pc/platform.h"
#include "pc/fs/fs.h"

#include "game/level_update.h"
#include "game/first_person_cam.h"
#include "game/bettercamera.h"
#include "game/sm64dx_ui.h"
#include "pc/lua/utils/smlua_misc_utils.h"

#define MAX_JOYBINDS 32
#define MAX_MOUSEBUTTONS 8 // arbitrary
#define MAX_JOYBUTTONS 32  // arbitrary; includes virtual keys for triggers
#define AXIS_THRESHOLD (30 * 256)

static bool init_ok = false;
static bool haptics_enabled = false;
#if defined(HAVE_SDL3)
static SDL_Gamepad *sdl_cntrl = NULL;
#else
static SDL_GameController *sdl_cntrl = NULL;
#endif
static SDL_Joystick *sdl_joystick = NULL;
static SDL_Haptic *sdl_haptic = NULL;

static bool sBackgroundGamepad = false;

static u32 num_joy_binds = 0;
static u32 num_mouse_binds = 0;
static u32 joy_binds[MAX_JOYBINDS][2] = { 0 };
static u32 mouse_binds[MAX_JOYBINDS][2] = { 0 };

static bool joy_buttons[MAX_JOYBUTTONS] = { false };
static u32 last_mouse = VK_INVALID;
static u32 last_joybutton = VK_INVALID;
static u32 last_gamepad = 0;

#if defined(HAVE_SDL3)
static SDL_JoystickID controller_sdl_get_instance_id(const u32 index) {
    int count = 0;
    SDL_JoystickID *joysticks = SDL_GetJoysticks(&count);
    SDL_JoystickID instanceId = 0;
    if (joysticks != NULL && index < (u32) count) {
        instanceId = joysticks[index];
    }
    if (joysticks != NULL) {
        SDL_free(joysticks);
    }
    return instanceId;
}

static const char *controller_sdl_get_instance_name(SDL_JoystickID instanceId) {
    const char *name = SDL_GetJoystickNameForID(instanceId);
    return (name != NULL) ? name : "Unknown";
}
#endif

static s16 invert_s16(s16 val) {
    if (val == -0x8000) return 0x7FFF;
    return (s16)(-(s32)val);
}

static inline void controller_add_binds(const u32 mask, const u32 *btns) {
    for (u32 i = 0; i < MAX_BINDS; ++i) {
        if (btns[i] >= VK_BASE_SDL_GAMEPAD && btns[i] <= VK_BASE_SDL_GAMEPAD + VK_SIZE) {
            if (btns[i] >= VK_BASE_SDL_MOUSE && num_joy_binds < MAX_JOYBINDS) {
                mouse_binds[num_mouse_binds][0] = btns[i] - VK_BASE_SDL_MOUSE;
                mouse_binds[num_mouse_binds][1] = mask;
                ++num_mouse_binds;
            } else if (num_mouse_binds < MAX_JOYBINDS) {
                joy_binds[num_joy_binds][0] = btns[i] - VK_BASE_SDL_GAMEPAD;
                joy_binds[num_joy_binds][1] = mask;
                ++num_joy_binds;
            }
        }
    }
}

static void controller_sdl_bind(void) {
    bzero(joy_binds, sizeof(joy_binds));
    bzero(mouse_binds, sizeof(mouse_binds));
    num_joy_binds = 0;
    num_mouse_binds = 0;

    controller_add_binds(A_BUTTON,     configKeyA);
    controller_add_binds(B_BUTTON,     configKeyB);
    controller_add_binds(X_BUTTON,     configKeyX);
    controller_add_binds(Y_BUTTON,     configKeyY);
    controller_add_binds(Z_TRIG,       configKeyZ);
    controller_add_binds(STICK_UP,     configKeyStickUp);
    controller_add_binds(STICK_LEFT,   configKeyStickLeft);
    controller_add_binds(STICK_DOWN,   configKeyStickDown);
    controller_add_binds(STICK_RIGHT,  configKeyStickRight);
    controller_add_binds(U_CBUTTONS,   configKeyCUp);
    controller_add_binds(L_CBUTTONS,   configKeyCLeft);
    controller_add_binds(D_CBUTTONS,   configKeyCDown);
    controller_add_binds(R_CBUTTONS,   configKeyCRight);
    controller_add_binds(L_TRIG,       configKeyL);
    controller_add_binds(R_TRIG,       configKeyR);
    controller_add_binds(START_BUTTON, configKeyStart);
    controller_add_binds(U_JPAD,       configKeyDUp);
    controller_add_binds(D_JPAD,       configKeyDDown);
    controller_add_binds(L_JPAD,       configKeyDLeft);
    controller_add_binds(R_JPAD,       configKeyDRight);
}

static void controller_sdl_init(void) {
    // Allows game to be controlled by gamepad when not in focus
    if (configBackgroundGamepad) {
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    }
    sBackgroundGamepad = configBackgroundGamepad;

    #if defined(HAVE_SDL3)
    if (!SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_EVENTS)) {
    #else
    if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0) {
    #endif
        fprintf(stderr, "SDL init error: %s\n", SDL_GetError());
        return;
    }

#ifdef WAPI_DXGI
    extern void WIN_UpdateKeymap(void);
    WIN_UpdateKeymap();
#endif

    #if defined(HAVE_SDL3)
    haptics_enabled = SDL_InitSubSystem(SDL_INIT_HAPTIC);
    #else
    haptics_enabled = (SDL_InitSubSystem(SDL_INIT_HAPTIC) == 0);
    #endif

    // try loading an external gamecontroller mapping file
    uint64_t gcsize = 0;
    void *gcdata = fs_load_file("gamecontrollerdb.txt", &gcsize);
    if (gcdata && gcsize) {
        #if defined(HAVE_SDL3)
        SDL_IOStream *rw = SDL_IOFromConstMem(gcdata, gcsize);
        #else
        SDL_RWops *rw = SDL_RWFromConstMem(gcdata, gcsize);
        #endif
        if (rw) {
            #if defined(HAVE_SDL3)
            int nummaps = SDL_AddGamepadMappingsFromIO(rw, true);
            #else
            int nummaps = SDL_GameControllerAddMappingsFromRW(rw, SDL_TRUE);
            #endif
            if (nummaps >= 0)
                printf("loaded %d controller mappings from 'gamecontrollerdb.txt'\n", nummaps);
        }
        free(gcdata);
    }

    if (gNewCamera.isMouse) { controller_mouse_enter_relative(); }
    controller_mouse_read_relative();

    controller_sdl_bind();

    init_ok = true;
    mouse_init_ok = true;
}

static SDL_Haptic *controller_sdl_init_haptics(SDL_Joystick *joystick, const char *name) {
    if (!haptics_enabled) return NULL;

    if (joystick == NULL) return NULL;

    #if defined(HAVE_SDL3)
    SDL_Haptic *hap = SDL_OpenHapticFromJoystick(joystick);
    #else
    SDL_Haptic *hap = SDL_HapticOpenFromJoystick(joystick);
    #endif
    if (!hap) return NULL;

    #if defined(HAVE_SDL3)
    if (!SDL_HapticRumbleSupported(hap)) {
        SDL_CloseHaptic(hap);
        return NULL;
    }

    if (!SDL_InitHapticRumble(hap)) {
        SDL_CloseHaptic(hap);
        return NULL;
    }
    #else
    if (!SDL_HapticRumbleSupported(hap)) {
        SDL_HapticClose(hap);
        return NULL;
    }

    if (!SDL_HapticRumbleInit(hap)) {
        SDL_HapticClose(hap);
        return NULL;
    }
    #endif

    printf("Controller %s has haptics support, rumble enabled\n", name);
    return hap;
}

static inline void update_button(const int i, const bool new) {
    const bool pressed = !joy_buttons[i] && new;
    joy_buttons[i] = new;
    if (pressed) {
        last_joybutton = i;
    }
}

extern s16 gMenuMode;
static void controller_sdl_read(OSContPad *pad) {
    if (!init_ok) { return; }

    if ((gNewCamera.isMouse || get_first_person_enabled())
        && !is_game_paused()
        && !sm64dx_ui_pause_menu_is_created()
        && !sm64dx_ui_is_in_main_menu()
        && !sm64dx_ui_is_chat_box_focused()
        && !sm64dx_ui_is_console_focused()
        && WAPI.has_focus()) {
        controller_mouse_enter_relative();
    } else {
        controller_mouse_leave_relative();
    }

    u32 mouse_prev = mouse_buttons;
    controller_mouse_read_relative();
    u32 mouse = mouse_buttons;

    if (!sm64dx_ui_uses_interactable_pad()) {
        for (u32 i = 0; i < num_mouse_binds; ++i)
            if (mouse & SDL_BUTTON_MASK(mouse_binds[i][0]))
                pad->button |= mouse_binds[i][1];
    }
    // remember buttons that changed from 0 to 1
    last_mouse = (mouse_prev ^ mouse) & mouse;

    if (configBackgroundGamepad != sBackgroundGamepad) {
        sBackgroundGamepad = configBackgroundGamepad;
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, sBackgroundGamepad ? "1" : "0");
    }

    if (configDisableGamepads) { return; }

    #if defined(HAVE_SDL3)
    SDL_UpdateGamepads();
    #else
    SDL_GameControllerUpdate();
    #endif

    #if defined(HAVE_SDL3)
    if (sdl_cntrl != NULL && !SDL_GamepadConnected(sdl_cntrl)) {
    #else
    if (sdl_cntrl != NULL && !SDL_GameControllerGetAttached(sdl_cntrl)) {
    #endif
        if (sdl_haptic) {
            #if defined(HAVE_SDL3)
            SDL_CloseHaptic(sdl_haptic);
            #else
            SDL_HapticClose(sdl_haptic);
            #endif
        }
        #if defined(HAVE_SDL3)
        SDL_CloseGamepad(sdl_cntrl);
        #else
        SDL_GameControllerClose(sdl_cntrl);
        #endif
        sdl_cntrl = NULL;
        sdl_haptic = NULL;
    }

    if ((!sdl_cntrl && !sdl_joystick) || last_gamepad != configGamepadNumber) {
        if (sdl_haptic) {
            #if defined(HAVE_SDL3)
            SDL_CloseHaptic(sdl_haptic);
            #else
            SDL_HapticClose(sdl_haptic);
            #endif
            sdl_haptic = NULL;
        }
        if (sdl_cntrl) {
            #if defined(HAVE_SDL3)
            SDL_CloseGamepad(sdl_cntrl);
            #else
            SDL_GameControllerClose(sdl_cntrl);
            #endif
            sdl_cntrl = NULL;
        }
        if (sdl_joystick) {
            #if defined(HAVE_SDL3)
            SDL_CloseJoystick(sdl_joystick);
            #else
            SDL_JoystickClose(sdl_joystick);
            #endif
            sdl_joystick = NULL;
        }
        last_gamepad = configGamepadNumber;
        #if defined(HAVE_SDL3)
        SDL_JoystickID instanceId = controller_sdl_get_instance_id(configGamepadNumber);
        if (instanceId == 0) { return; }
        if (SDL_IsGamepad(instanceId)) {
            sdl_cntrl = SDL_OpenGamepad(instanceId);
            if (sdl_cntrl != NULL) {
                sdl_haptic = controller_sdl_init_haptics(SDL_GetGamepadJoystick(sdl_cntrl),
                    controller_sdl_get_instance_name(instanceId));
            }
        } else {
            sdl_joystick = SDL_OpenJoystick(instanceId);
            if (!sdl_joystick) { return; }
            sdl_haptic = controller_sdl_init_haptics(sdl_joystick, controller_sdl_get_instance_name(instanceId));
        }
        #else
        if (SDL_IsGameController(configGamepadNumber)) {
            sdl_cntrl = SDL_GameControllerOpen(configGamepadNumber);
            if (sdl_cntrl != NULL) {
                sdl_haptic = controller_sdl_init_haptics(SDL_GameControllerGetJoystick(sdl_cntrl),
                    SDL_JoystickNameForIndex(configGamepadNumber));
            }
        } else {
            sdl_joystick = SDL_JoystickOpen(configGamepadNumber);
            if (!sdl_joystick) { return; }
            sdl_haptic = controller_sdl_init_haptics(sdl_joystick, SDL_JoystickNameForIndex(configGamepadNumber));
        }
        #endif
    }

    int16_t leftx = 0, lefty = 0, rightx = 0, righty = 0;
    int16_t ltrig = 0, rtrig = 0;
    if (sdl_cntrl) {
        #if defined(HAVE_SDL3)
        leftx = SDL_GetGamepadAxis(sdl_cntrl, SDL_GAMEPAD_AXIS_LEFTX);
        lefty = SDL_GetGamepadAxis(sdl_cntrl, SDL_GAMEPAD_AXIS_LEFTY);
        rightx = SDL_GetGamepadAxis(sdl_cntrl, SDL_GAMEPAD_AXIS_RIGHTX);
        righty = SDL_GetGamepadAxis(sdl_cntrl, SDL_GAMEPAD_AXIS_RIGHTY);
        ltrig = SDL_GetGamepadAxis(sdl_cntrl, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
        rtrig = SDL_GetGamepadAxis(sdl_cntrl, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
        for (u32 i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; ++i) {
            const bool new = SDL_GetGamepadButton(sdl_cntrl, (SDL_GamepadButton) i);
        #else
        leftx = SDL_GameControllerGetAxis(sdl_cntrl, SDL_CONTROLLER_AXIS_LEFTX);
        lefty = SDL_GameControllerGetAxis(sdl_cntrl, SDL_CONTROLLER_AXIS_LEFTY);
        rightx = SDL_GameControllerGetAxis(sdl_cntrl, SDL_CONTROLLER_AXIS_RIGHTX);
        righty = SDL_GameControllerGetAxis(sdl_cntrl, SDL_CONTROLLER_AXIS_RIGHTY);
        ltrig = SDL_GameControllerGetAxis(sdl_cntrl, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        rtrig = SDL_GameControllerGetAxis(sdl_cntrl, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        for (u32 i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
            const bool new = SDL_GameControllerGetButton(sdl_cntrl, i);
        #endif
            update_button(i, new);
        }
    } else if (sdl_joystick) {
        #if defined(HAVE_SDL3)
        int axis_count = SDL_GetNumJoystickAxes(sdl_joystick);
        #else
        int axis_count = SDL_JoystickNumAxes(sdl_joystick);
        #endif
        if (axis_count >= 2) {
            #if defined(HAVE_SDL3)
            leftx = SDL_GetJoystickAxis(sdl_joystick, 0);
            lefty = SDL_GetJoystickAxis(sdl_joystick, 1);
            #else
            leftx = SDL_JoystickGetAxis(sdl_joystick, 0);
            lefty = SDL_JoystickGetAxis(sdl_joystick, 1);
            #endif
        }
        if (axis_count >= 4) {
            #if defined(HAVE_SDL3)
            rightx = SDL_GetJoystickAxis(sdl_joystick, 2);
            righty = SDL_GetJoystickAxis(sdl_joystick, 5);
            #else
            rightx = SDL_JoystickGetAxis(sdl_joystick, 2);
            righty = SDL_JoystickGetAxis(sdl_joystick, 5); // Specific to N64 controller
            #endif
        }
        if (axis_count >= 6) {
            #if defined(HAVE_SDL3)
            ltrig = SDL_GetJoystickAxis(sdl_joystick, 3);
            rtrig = SDL_GetJoystickAxis(sdl_joystick, 4);
            #else
            ltrig = SDL_JoystickGetAxis(sdl_joystick, 3);
            rtrig = SDL_JoystickGetAxis(sdl_joystick, 4);
            #endif
        }

        #if defined(HAVE_SDL3)
        int button_count = SDL_GetNumJoystickButtons(sdl_joystick);
        #else
        int button_count = SDL_JoystickNumButtons(sdl_joystick);
        #endif
        for (int i = 0; i < button_count && i < MAX_JOYBUTTONS; ++i) {
            #if defined(HAVE_SDL3)
            update_button(i, SDL_GetJoystickButton(sdl_joystick, i));
            #else
            update_button(i, SDL_JoystickGetButton(sdl_joystick, i));
            #endif
        }
    }

    if (configStick.rotateLeft) {
        s16 tmp = leftx;
        leftx = invert_s16(lefty);
        lefty = tmp;
    }
    if (configStick.rotateRight) {
        s16 tmp = rightx;
        rightx = invert_s16(righty);
        righty = tmp;
    }
    if (configStick.invertLeftX) { leftx = invert_s16(leftx); }
    if (configStick.invertLeftY) { lefty = invert_s16(lefty); }
    if (configStick.invertRightX) { rightx = invert_s16(rightx); }
    if (configStick.invertRightY) { righty = invert_s16(righty); }

    update_button(VK_LTRIGGER - VK_BASE_SDL_GAMEPAD, ltrig > AXIS_THRESHOLD);
    update_button(VK_RTRIGGER - VK_BASE_SDL_GAMEPAD, rtrig > AXIS_THRESHOLD);

    u32 buttons_down = 0;
    for (u32 i = 0; i < num_joy_binds; ++i)
        if (joy_buttons[joy_binds[i][0]])
            buttons_down |= joy_binds[i][1];

    pad->button |= buttons_down;

    const u32 xstick = buttons_down & STICK_XMASK;
    const u32 ystick = buttons_down & STICK_YMASK;
    if (xstick == STICK_LEFT)
        pad->stick_x = -128;
    else if (xstick == STICK_RIGHT)
        pad->stick_x = 127;
    if (ystick == STICK_DOWN)
        pad->stick_y = -128;
    else if (ystick == STICK_UP)
        pad->stick_y = 127;

    if (rightx < -0x4000) pad->button |= L_CBUTTONS;
    if (rightx > 0x4000) pad->button |= R_CBUTTONS;
    if (righty < -0x4000) pad->button |= U_CBUTTONS;
    if (righty > 0x4000) pad->button |= D_CBUTTONS;

    uint32_t magnitude_sq = (uint32_t)(leftx * leftx) + (uint32_t)(lefty * lefty);
    uint32_t stickDeadzoneActual = configStickDeadzone * DEADZONE_STEP;
    if (magnitude_sq > (uint32_t)(stickDeadzoneActual * stickDeadzoneActual)) {
        pad->stick_x = leftx / 0x100;
        int stick_y = -lefty / 0x100;
        pad->stick_y = stick_y == 128 ? 127 : stick_y;
    }

    magnitude_sq = (uint32_t)(rightx * rightx) + (uint32_t)(righty * righty);
    stickDeadzoneActual = configStickDeadzone * DEADZONE_STEP;
    if (magnitude_sq > (uint32_t)(stickDeadzoneActual * stickDeadzoneActual)) {
        pad->ext_stick_x = rightx / 0x100;
        int stick_y = -righty / 0x100;
        pad->ext_stick_y = stick_y == 128 ? 127 : stick_y;
    }
}

static void controller_sdl_rumble_play(f32 strength, f32 length) {
    if (sdl_haptic) {
        #if defined(HAVE_SDL3)
        SDL_PlayHapticRumble(sdl_haptic, strength, (u32)(length * 1000.0f));
        #else
        SDL_HapticRumblePlay(sdl_haptic, strength, (u32)(length * 1000.0f));
        #endif
    } else {
#if defined(HAVE_SDL3) || SDL_VERSION_ATLEAST(2,0,18)
        uint16_t scaled_strength = strength * pow(2, 16) - 1;
        if (sdl_cntrl != NULL) {
            #if defined(HAVE_SDL3)
            SDL_RumbleGamepad(sdl_cntrl, scaled_strength, scaled_strength, (u32)(length * 1000.0f));
            #else
            if (SDL_GameControllerHasRumble(sdl_cntrl) == SDL_TRUE) {
                SDL_GameControllerRumble(sdl_cntrl, scaled_strength, scaled_strength, (u32)(length * 1000.0f));
            }
            #endif
        }
#endif
    }
}

static void controller_sdl_rumble_stop(void) {
    if (sdl_haptic) {
        #if defined(HAVE_SDL3)
        SDL_StopHapticRumble(sdl_haptic);
        #else
        SDL_HapticRumbleStop(sdl_haptic);
        #endif
    } else {
#if defined(HAVE_SDL3) || SDL_VERSION_ATLEAST(2,0,18)
        if (sdl_cntrl != NULL) {
            #if defined(HAVE_SDL3)
            SDL_RumbleGamepad(sdl_cntrl, 0, 0, 0);
            #else
            if (SDL_GameControllerHasRumble(sdl_cntrl) == SDL_TRUE) {
                SDL_GameControllerRumble(sdl_cntrl, 0, 0, 0);
            }
            #endif
        }
#endif
    }
}

static u32 controller_sdl_rawkey(void) {
    if (last_joybutton != VK_INVALID) {
        const u32 ret = last_joybutton;
        last_joybutton = VK_INVALID;
        return ret;
    }

    for (u32 i = 1; i < MAX_MOUSEBUTTONS; ++i) {
        if (last_mouse & SDL_BUTTON_MASK(i)) {
            const u32 ret = VK_OFS_SDL_MOUSE + i;
            last_mouse = 0;
            return ret;
        }
    }
    return VK_INVALID;
}

static void controller_sdl_shutdown(void) {
    #if defined(HAVE_SDL3)
    if (SDL_WasInit(SDL_INIT_GAMEPAD)) {
    #else
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
    #endif
        if (sdl_cntrl) {
            #if defined(HAVE_SDL3)
            SDL_CloseGamepad(sdl_cntrl);
            #else
            SDL_GameControllerClose(sdl_cntrl);
            #endif
            sdl_cntrl = NULL;
        }
        #if defined(HAVE_SDL3)
        SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
        #else
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
        #endif
    }

    if (SDL_WasInit(SDL_INIT_HAPTIC)) {
        if (sdl_haptic) {
            #if defined(HAVE_SDL3)
            SDL_CloseHaptic(sdl_haptic);
            #else
            SDL_HapticClose(sdl_haptic);
            #endif
            sdl_haptic = NULL;
        }
        SDL_QuitSubSystem(SDL_INIT_HAPTIC);
    }

    if (sdl_joystick) {
        #if defined(HAVE_SDL3)
        SDL_CloseJoystick(sdl_joystick);
        #else
        SDL_JoystickClose(sdl_joystick);
        #endif
        sdl_joystick = NULL;
    }

    haptics_enabled = false;
    init_ok = false;
    mouse_init_ok = false;
}

struct ControllerAPI controller_sdl = {
    VK_BASE_SDL_GAMEPAD,
    controller_sdl_init,
    controller_sdl_read,
    controller_sdl_rawkey,
    controller_sdl_rumble_play,
    controller_sdl_rumble_stop,
    controller_sdl_bind,
    controller_sdl_shutdown
};

#endif
