#include "wifi.h"
#include "filesystem.h"
#include <esp_wifi.h>
#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_log.h>

static const char *TAG = "WIFI";
static httpd_handle_t server = NULL;

typedef struct {
    user_post_cb callback;
    void* user_ctx;
} user_post_cb_data;

typedef struct {
    user_get_cb callback;
    void* user_ctx;
    bool binary;
} user_get_cb_data;

void wifi_init() {
    // wifi
    ESP_LOGI(TAG, "starting wifi...");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize Wi-Fi: %s", esp_err_to_name(ret));
        return;
    }

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_NAME,
            .password = WIFI_PASSWORD,
            .channel = 0,
            .max_connection = 1,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            }
        },
    };
    wifi_config.ap.ssid_len = strlen((char*)wifi_config.ap.ssid);

    if (strlen((char*)wifi_config.ap.password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // server
    ESP_LOGI(TAG, "starting server...");
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = MAX_ENDPOINTS;
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "error starting server!");
        return;
    }

    wifi_attachCallback_getConst("/", "/storage/index.htm");
    wifi_attachCallback_getConst("/index.html", "/storage/index.htm");
    wifi_attachCallback_getConst("/icon.png", "/storage/icon.png");
    wifi_attachCallback_getConst("/Victor_Mono/OFL.txt", "/storage/Victor_Mono/OFL.txt");
    wifi_attachCallback_getConst("/Victor_Mono/README.txt", "/storage/Victor_Mono/README.txt");
    wifi_attachCallback_getConst("/Victor_Mono/VictorMono.ttf", "/storage/Victor_Mono/VictorMono.ttf");

    ESP_LOGI(TAG, "wifi & server initialization done!");
}

int wifi_getUsersCount() {
    wifi_sta_list_t sta_list;
    esp_wifi_ap_get_sta_list(&sta_list);
    return sta_list.num;
}

void wifi_stop() {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
    
    ESP_LOGI(TAG, "stopping Wi-Fi...");
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_event_loop_delete_default());
}

static esp_err_t user_post_handler(httpd_req_t* req) {
    user_post_cb_data* cb_data = req->user_ctx;
    
    char* content = NULL;
    if (req->content_len > 0) {
        content = malloc(req->content_len);
        int ret = httpd_req_recv(req, content, req->content_len);
        if (ret <= 0) {
            free(content);
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }
    }

    cb_data->callback(cb_data->user_ctx, content, req->content_len);
    if (content) free(content);

    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t user_getConst_handler(httpd_req_t* req) {
    const char* path = req->user_ctx;
    ESP_LOGI(TAG, "file request: %s", path);
    size_t contentSize;
    void* content = filesystem_readFileToHeapAndGetSize(path, &contentSize);
    if (!content) {
        ESP_LOGE(TAG, "failed to read: %s", path);
        return ESP_FAIL;
    }
    httpd_resp_send(req, content, contentSize);
    free(content);
    return ESP_OK;
}

static esp_err_t user_get_handler(httpd_req_t* req) {
    user_get_cb_data* cb_data = req->user_ctx;
    
    char* content = NULL;
    if (req->content_len > 0) {
        content = malloc(req->content_len + (cb_data->binary ? 0 : 1));

        int ret = httpd_req_recv(req, content, req->content_len);
        if (ret <= 0) {
            free(content);
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }
    }

    size_t retsize;
    void* ret = cb_data->callback(
        cb_data->user_ctx,
        content,
        req->content_len,
        &retsize
    );
    if (content) free(content);

    if (retsize == 0) {
        httpd_resp_send(req, "", 0);
    } else {
        if (cb_data->binary) {
            httpd_resp_set_type(req, "application/octet-stream");
        }

        httpd_resp_send(req, ret, retsize);
        free(ret);
    }

    return ESP_OK;
}



void wifi_attachCallback_post(const char* path, user_post_cb callback, void* user_ctx) {
    user_post_cb_data* cb_data = malloc(sizeof(user_post_cb_data));
    cb_data->callback = callback;
    cb_data->user_ctx = user_ctx;

    httpd_uri_t uri = {
        .uri = path,
        .method = HTTP_POST,
        .handler = user_post_handler,
        .user_ctx = cb_data
    };
    
    httpd_register_uri_handler(server, &uri);
}

void wifi_attachCallback_get(const char* path, user_get_cb callback, void* user_ctx) {
    user_get_cb_data* cb_data = malloc(sizeof(user_get_cb_data));
    cb_data->callback = callback;
    cb_data->user_ctx = user_ctx;
    cb_data->binary = false;

    httpd_uri_t uri = {
        .uri = path,
        .method = HTTP_GET,
        .handler = user_get_handler,
        .user_ctx = cb_data
    };
    
    httpd_register_uri_handler(server, &uri);
}

void wifi_attachCallback_get_binary(const char* path, user_get_cb callback, void* user_ctx) {
    user_get_cb_data* cb_data = malloc(sizeof(user_get_cb_data));
    cb_data->callback = callback;
    cb_data->user_ctx = user_ctx;
    cb_data->binary = true;

    httpd_uri_t uri = {
        .uri = path,
        .method = HTTP_GET,
        .handler = user_get_handler,
        .user_ctx = cb_data
    };
    
    httpd_register_uri_handler(server, &uri);
}

void wifi_attachCallback_getConst(const char* path, const char* filepath) {
    httpd_uri_t uri = {
        .uri = path,
        .method = HTTP_GET,
        .handler = user_getConst_handler,
        .user_ctx = (void*)filepath
    };
    httpd_register_uri_handler(server, &uri);
}