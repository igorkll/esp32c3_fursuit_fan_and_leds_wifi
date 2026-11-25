#include <stdio.h>
#include <esp_err.h>
#include <esp_vfs.h>
#include <esp_spiffs.h>
#include "filesystem.h"

bool filesystem_init() {
    esp_vfs_spiffs_conf_t fs_config = {
        .base_path = "/storage",
        .partition_label = "storage",
        .max_files = 2,
        .format_if_mount_failed = false
    };

    return esp_vfs_spiffs_register(&fs_config) == ESP_OK;
}

void* filesystem_readFileToHeap(const char* path) {
    size_t size;
    return filesystem_readFileToHeapAndGetSize(path, &size);
}

void* filesystem_readFileToHeapAndGetSize(const char* path, size_t* size) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(*size + 1);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, *size, file);
    if (read_size != *size) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[*size] = '\0';
    fclose(file);
    return buffer;
}

bool filesystem_writeFile(const char* path, void* data, size_t size) {
    FILE* file = fopen(path, "wb");
    if (file == NULL) return false;
    fwrite(data, size, 1, file);
    fclose(file);
    return true;
}

bool filesystem_exists(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) return false;
    fclose(file);
    return true;
}