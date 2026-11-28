#pragma once
#include "main.h"

#define MAX_ENDPOINTS 64
#define WIFI_CHUNK_SIZE 1024

typedef void (*user_post_cb) (void* user_ctx, char* content, size_t len);
typedef void* (*user_get_cb) (void* user_ctx, char* content, size_t len, size_t* retsize);

void wifi_init();
void wifi_reloadSettings();
int wifi_getUsersCount();
void wifi_attachCallback_post(const char* path, user_post_cb callback, void* user_ctx);
void wifi_attachCallback_get(const char* path, user_get_cb callback, void* user_ctx);
void wifi_attachCallback_get_binary(const char* path, user_get_cb callback, void* user_ctx);
void wifi_attachCallback_getConst(const char* path, const char* filepath);
void wifi_stop();