#include <stdio.h>
#include <string.h>

#include "network.h"

static s64 sOfflineIds[MAX_PLAYERS] = { 0 };

static bool ns_offline_initialize(enum NetworkType networkType, UNUSED bool reconnecting) {
    memset(sOfflineIds, 0, sizeof(sOfflineIds));
    return networkType == NT_SERVER;
}

static s64 ns_offline_get_id(u8 localIndex) {
    return (localIndex < MAX_PLAYERS) ? sOfflineIds[localIndex] : 0;
}

static char* ns_offline_get_id_str(u8 localIndex) {
    static char sIdString[32] = { 0 };
    snprintf(sIdString, sizeof(sIdString), "%lld", (long long)ns_offline_get_id(localIndex));
    return sIdString;
}

static void ns_offline_save_id(u8 localIndex, s64 networkId) {
    if (localIndex < MAX_PLAYERS) {
        sOfflineIds[localIndex] = networkId;
    }
}

static void ns_offline_clear_id(u8 localIndex) {
    if (localIndex < MAX_PLAYERS) {
        sOfflineIds[localIndex] = 0;
    }
}

static void* ns_offline_dup_addr(UNUSED u8 localIndex) {
    return NULL;
}

static bool ns_offline_match_addr(void* addr1, void* addr2) {
    return addr1 == addr2;
}

static void ns_offline_update(void) {
}

static int ns_offline_send(UNUSED u8 localIndex, UNUSED void* addr, UNUSED u8* data, UNUSED u16 dataLength) {
    return 0;
}

static void ns_offline_get_lobby_id(char* destination, u32 destLength) {
    snprintf(destination, destLength, "%s", "");
}

static void ns_offline_get_lobby_secret(char* destination, u32 destLength) {
    snprintf(destination, destLength, "%s", "");
}

static void ns_offline_shutdown(UNUSED bool reconnecting) {
    memset(sOfflineIds, 0, sizeof(sOfflineIds));
}

struct NetworkSystem gNetworkSystemOffline = {
    .initialize       = ns_offline_initialize,
    .get_id           = ns_offline_get_id,
    .get_id_str       = ns_offline_get_id_str,
    .save_id          = ns_offline_save_id,
    .clear_id         = ns_offline_clear_id,
    .dup_addr         = ns_offline_dup_addr,
    .match_addr       = ns_offline_match_addr,
    .update           = ns_offline_update,
    .send             = ns_offline_send,
    .get_lobby_id     = ns_offline_get_lobby_id,
    .get_lobby_secret = ns_offline_get_lobby_secret,
    .shutdown         = ns_offline_shutdown,
    .requireServerBroadcast = false,
    .name             = "Offline",
};
