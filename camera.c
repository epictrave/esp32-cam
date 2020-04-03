
#include "camera.h"

static const char *TAG = "camera";
camera_config_t config = {
    .ledc_channel = LEDC_CHANNEL_0,
    .ledc_timer = LEDC_TIMER_0,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .xclk_freq_hz = 20000000,
    .pixel_format = PIXFORMAT_JPEG,
    // init with high specs to pre-allocate larger buffers
    .frame_size = FRAMESIZE_QSXGA,
    .jpeg_quality = 12,
    .fb_count = 2,
};

ledc_timer_config_t ledc_timer = {
    .duty_resolution = LEDC_TIMER_8_BIT, // resolution of PWM duty
    .freq_hz = 1000,                     // frequency of PWM signal
    .speed_mode = LEDC_HIGH_SPEED_MODE,  // timer mode
    .timer_num = LEDC_TIMER_1            // timer index
};

ledc_channel_config_t ledc_channel = {.channel = LEDC_CHANNEL_1,
                                      .duty = 0,
                                      .gpio_num = 4,
                                      .speed_mode = LEDC_HIGH_SPEED_MODE,
                                      .hpoint = 0,
                                      .timer_sel = LEDC_TIMER_1};

char camera_url[256];

esp_err_t camera_http_event_handler(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
  case HTTP_EVENT_ERROR:
    ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key,
             evt->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    if (!esp_http_client_is_chunked_response(evt->client)) {
      // Write out data
      // strncat(buffer, (char *)evt->data, evt->data_len);
    }
    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
    break;
  }
  return ESP_OK;
}

bool is_set_url = false;

esp_err_t camera_init_config(void) {

  gpio_set_direction(4, GPIO_MODE_OUTPUT);
  switch (ledc_timer_config(&ledc_timer)) {
  case ESP_ERR_INVALID_ARG:
    ESP_LOGE(TAG, "ledc_timer_config() parameter error");
    break;
  case ESP_FAIL:
    ESP_LOGE(TAG,
             "ledc_timer_config() Can not find a proper pre-divider number "
             "base on the given frequency and the current duty_resolution");
    break;
  case ESP_OK:
    if (ledc_channel_config(&ledc_channel) == ESP_ERR_INVALID_ARG) {
      ESP_LOGE(TAG, "ledc_channel_config() parameter error");
    }
    break;
  default:
    break;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
    return err;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);       // flip it back
    s->set_brightness(s, 1);  // up the blightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_XGA);
  return ESP_OK;
}

void camera_set_url(const char *url) {
  if (!is_set_url) {
    is_set_url = true;
  }
  memset(camera_url, 0, sizeof(camera_url));
  memcpy(camera_url, url, strlen(url) + 1);
  ESP_LOGI(TAG, "url : %s", camera_url);
}

esp_err_t camera_take_picture(void) {
  ESP_LOGI(TAG, "Taking picture...");
  camera_fb_t *fb = NULL;
  int64_t fr_start = esp_timer_get_time();
  size_t fb_len = 0;

  fb = esp_camera_fb_get();
  if (!fb) {
    ESP_LOGE(TAG, "Camera capture failed");
    return ESP_FAIL;
  }
  fb_len = fb->len;

  if (!is_set_url) {
    ESP_LOGE(TAG, "url is not set.");
    esp_camera_fb_return(fb);
    return ESP_FAIL;
  }

  esp_http_client_handle_t http_client;
  esp_http_client_config_t camera_http_client_config = {0};
  camera_http_client_config.url = camera_url;
  camera_http_client_config.event_handler = camera_http_event_handler;
  camera_http_client_config.method = HTTP_METHOD_POST;

  http_client = esp_http_client_init(&camera_http_client_config);

  esp_http_client_set_post_field(http_client, (const char *)fb->buf, fb->len);
  esp_http_client_set_header(http_client, "Content-Type",
                             "multipart/form-data");

  esp_err_t err = esp_http_client_perform(http_client);
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "esp_http_client_get_status_code: %d",
             esp_http_client_get_status_code(http_client));
  }

  esp_http_client_cleanup(http_client);

  int64_t fr_end = esp_timer_get_time();
  ESP_LOGI(TAG, "JPG: %uKB %ums", (uint32_t)(fb_len / 1024),
           (uint32_t)((fr_end - fr_start) / 1000));
  esp_camera_fb_return(fb);

  return ESP_OK;
}

void camera_parse_from_json(const char *json, DEVICE_TWIN_STATE update_state) {
  JSON_Value *root_value = json_parse_string(json);
  JSON_Object *root_object = json_value_get_object(root_value);
  char property_name[40];
  if (update_state == UPDATE_PARTIAL) {
    sprintf(property_name, "cam");
  } else if (update_state == UPDATE_COMPLETE) {
    sprintf(property_name, "desired.cam");
  }

  JSON_Object *json_cam = json_object_dotget_object(root_object, property_name);

  if (json_object_get_value(json_cam, "url") != NULL) {
    const char *url = json_object_get_string(json_cam, "url");
    camera_set_url(url);
  }

  if (json_object_get_value(json_cam, "led") != NULL) {
    int duty = json_object_get_number(json_cam, "led");
    if (duty < 0) {
      duty = 0;
    } else if (duty > 255) {
      duty = 255;
    }
    ledc_channel.duty = duty;
    if (ledc_channel_config(&ledc_channel) == ESP_ERR_INVALID_ARG) {
      ESP_LOGE(TAG, "ledc_channel_config() parameter error");
    }
  }

  if (json_object_get_value(json_cam, "frame") != NULL) {
    framesize_t frame = (framesize_t)json_object_get_number(json_cam, "frame");
    if (frame < 0) {
      frame = 0;
    } else if (frame > 13) {
      frame = 13;
    }
    sensor_t *s = esp_camera_sensor_get();
    s->set_framesize(s, frame);
  }
}