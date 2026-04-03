#if defined(AAPI_SDL3) || defined(AAPI_SDL2)
#include <stdio.h>

#if defined(HAVE_SDL3)
#include <SDL3/SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "audio_api.h"

static SDL_AudioDeviceID dev;
static SDL_AudioStream *stream;

static bool audio_sdl_init(void) {
    #if defined(HAVE_SDL3)
    if (!SDL_Init(SDL_INIT_AUDIO)) {
    #else
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
    #endif
        fprintf(stderr, "SDL init error: %s\n", SDL_GetError());
        return false;
    }

    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = 32000;
    #if defined(HAVE_SDL3)
    want.format = SDL_AUDIO_S16;
    #else
    want.format = AUDIO_S16SYS;
    #endif
    want.channels = 2;
    #if !defined(HAVE_SDL3)
    want.samples = 512;
    #endif

#if defined(HAVE_SDL3)
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want, NULL, NULL);
    if (stream == NULL) {
        fprintf(stderr, "SDL_OpenAudioDeviceStream error: %s\n", SDL_GetError());
        return false;
    }
    dev = SDL_GetAudioStreamDevice(stream);
    if (!SDL_ResumeAudioDevice(dev)) {
        fprintf(stderr, "SDL_ResumeAudioDevice error: %s\n", SDL_GetError());
        SDL_DestroyAudioStream(stream);
        stream = NULL;
        dev = 0;
        return false;
    }
#else
    SDL_AudioSpec have;
    want.callback = NULL;
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev == 0) {
        fprintf(stderr, "SDL_OpenAudio error: %s\n", SDL_GetError());
        return false;
    }
    SDL_PauseAudioDevice(dev, 0);
#endif
    return true;
}

static int audio_sdl_buffered(void) {
#if defined(HAVE_SDL3)
    return (stream != NULL) ? (SDL_GetAudioStreamQueued(stream) / 4) : 0;
#else
    return SDL_GetQueuedAudioSize(dev) / 4;
#endif
}

static int audio_sdl_get_desired_buffered(void) {
    return 1100;
}

static void audio_sdl_play(const uint8_t *buf, size_t len) {
    if (audio_sdl_buffered() < 6000) {
        // Don't fill the audio buffer too much in case this happens
#if defined(HAVE_SDL3)
        SDL_PutAudioStreamData(stream, buf, (int) len);
#else
        SDL_QueueAudio(dev, buf, len);
#endif
    }
}

static void audio_sdl_shutdown(void) 
{
    if (SDL_WasInit(SDL_INIT_AUDIO)) {
#if defined(HAVE_SDL3)
        if (stream != NULL) {
            SDL_DestroyAudioStream(stream);
            stream = NULL;
            dev = 0;
        }
#else
        if (dev != 0) {
            SDL_CloseAudioDevice(dev);
            dev = 0;
        }
#endif
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
}

struct AudioAPI audio_sdl = {
    audio_sdl_init,
    audio_sdl_buffered,
    audio_sdl_get_desired_buffered,
    audio_sdl_play,
    audio_sdl_shutdown
};

#endif
