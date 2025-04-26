#include "fetap_microphone.h"

#include "freertos/FreeRTOS.h"
#include "esphome/core/log.h"

namespace esphome {

namespace fetap {

static const char *const TAG = "fetap.microphone";

void FetapMicrophone::setup(void) {
    esp_err_t err;

    i2s_chan_config_t rx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    err = i2s_new_channel(&rx_chan_cfg, NULL, &i2s_rx_channel_);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error creating I2S channel: %s", esp_err_to_name(err));
        mark_failed();
        status_set_error();
        return;
    }

    i2s_std_config_t rx_std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = bclk_pin_,
            .ws   = lrclk_pin_,
            .dout = I2S_GPIO_UNUSED,
            .din  = din_pin_,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    rx_std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    err = i2s_channel_init_std_mode(i2s_rx_channel_, &rx_std_cfg);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error initializing I2S channel: %s", esp_err_to_name(err));
        mark_failed();
        status_set_error();
        return;
    }

    buffer_.reserve(kBufferSize);
    raw_i2s_buffer_.reserve(kBufferSize);

    ESP_LOGI(TAG, "Fetap Microphone initialized successfully.");
}

void FetapMicrophone::start(void) {
    if (state_ == microphone::STATE_RUNNING || is_failed()) {
        return;
    }

    state_ = microphone::STATE_STARTING;
}

void FetapMicrophone::start_(void) {
    const esp_err_t err = i2s_channel_enable(i2s_rx_channel_);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error enabling I2S channel: %s", esp_err_to_name(err));
        status_set_error();
        return;
    }

    state_ = microphone::STATE_RUNNING;
    high_freq_.start();
    status_clear_error();
    ESP_LOGI(TAG, "Fetap Microphone started successfully.");
}

void FetapMicrophone::stop(void) {
    if (state_ == microphone::STATE_STOPPED || is_failed()) {
        return;
    }

    if (state_ == microphone::STATE_STARTING) {
        state_ = microphone::STATE_STOPPED;
        return;
    }

    state_ = microphone::STATE_STOPPING;
}

void FetapMicrophone::stop_(void) {
    const esp_err_t err = i2s_channel_disable(i2s_rx_channel_);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error disabling I2S channel: %s", esp_err_to_name(err));
        status_set_error();
        return;
    }

    state_ = microphone::STATE_STOPPED;
    high_freq_.stop();
    status_clear_error();
    ESP_LOGI(TAG, "Fetap Microphone stopped successfully.");
}

size_t FetapMicrophone::read(int16_t *buf, size_t len) {
    size_t n_bytes_read{0};

    // The I2S RX channel config reads from the microphone with 32 bit sample width which is directly
    // written into the buffer (buf) and afterwards converted down to 16 bit sample width (int_16).
    const esp_err_t err = i2s_channel_read(i2s_rx_channel_, buf, len, &n_bytes_read, pdMS_TO_TICKS(kMaxI2SReadTimeoutMilliseconds));

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error reading from I2S channel: %s", esp_err_to_name(err));
        status_set_warning();
        return 0;
    }

    if (n_bytes_read == 0) {
        status_set_warning();
        return 0;
    }

    status_clear_warning();

    // Convert 32 bit samples to 16 bit samples
    const size_t samples_read = n_bytes_read / sizeof(int32_t);
    for (size_t i = 0; i < samples_read; i++) {
        int32_t temp = reinterpret_cast<int32_t *>(buf)[i] >> 13;
        buf[i] = clamp<int16_t>(temp, INT16_MIN, INT16_MAX);
    }
    return samples_read * sizeof(int16_t);
}

void FetapMicrophone::read_(void) {
    // Note: In order to adhere to the esphome i2s_audio_microphone implementation,
    //       we actually pass the amount of int32 samples that can be read into the
    //       buffer_ (allocated as kBufferSize * sizeof(int16_t)).
    const size_t bytes_read = read(buffer_.data(), kBufferSize / sizeof(int16_t));

    buffer_.resize(bytes_read / sizeof(int16_t));
    data_callbacks_.call(buffer_);
}

void FetapMicrophone::loop(void) {
    switch (state_) {
        case microphone::STATE_STOPPED:
            break;
        case microphone::STATE_STARTING:
            start_();
            break;
        case microphone::STATE_RUNNING:
            if (data_callbacks_.size() > 0) {
                read_();
            }
            break;
        case microphone::STATE_STOPPING:
            stop_();
            break;
    }
}

}

}