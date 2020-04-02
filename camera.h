
#ifndef CAMERA_H
#define CAMERA_H

#include "config.h"
#include "device_twin_state.h"
#include "driver/ledc.h"
#include "esp_camera.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "parson.h"
#include "sdkconfig.h"
#include "string.h"

#define CAM_BOARD "AI-THINKER"
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t camera_init_config(void);
esp_err_t camera_take_picture(void);
void camera_set_url(const char *url);
void camera_parse_from_json(const char *json, DEVICE_TWIN_STATE update_state);
#ifdef __cplusplus
}
#endif

#endif
