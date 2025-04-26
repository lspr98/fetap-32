#include "fetap_speaker.h"

#include "freertos/FreeRTOS.h"
#include "esphome/core/log.h"

namespace esphome {

namespace fetap {

static const char *const TAG = "fetap.speaker";

void FetapSpeaker::setup(void) {
    esp_err_t err;

    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    err = i2s_new_channel(&tx_chan_cfg, &i2s_tx_channel_, NULL);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error creating I2S channel: %s", esp_err_to_name(err));
        mark_failed();
        status_set_error();
        return;
    }

    i2s_std_config_t tx_std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = bclk_pin_,
            .ws   = lrclk_pin_,
            .dout = dout_pin_,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };
    tx_std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    err = i2s_channel_init_std_mode(i2s_tx_channel_, &tx_std_cfg);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error initializing I2S channel: %s", esp_err_to_name(err));
        mark_failed();
        status_set_error();
        return;
    }

    ESP_LOGI(TAG, "Fetap Speaker initialized successfully.");
}

void FetapSpeaker::start(void) {
    if (is_failed() || state_ != State::STOPPED) {
        return;
    }

    state_ = State::STARTING;
}

void FetapSpeaker::start_(void) {
    const esp_err_t err = i2s_channel_enable(i2s_tx_channel_);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error enabling I2S channel: %s", esp_err_to_name(err));
        status_set_error();
        return;
    }

    state_ = State::RUNNING;
    status_clear_error();
    ESP_LOGI(TAG, "Fetap Speaker started successfully.");
}

void FetapSpeaker::stop(void) {
    if (is_failed() || state_ != State::RUNNING) {
        return;
    }

    if (state_ == State::STARTING) {
        state_ = State::STOPPED;
        return;
    }

    state_ = State::STOPPING;
}

void FetapSpeaker::stop_(void) {
    const esp_err_t err = i2s_channel_disable(i2s_tx_channel_);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error disabling I2S channel: %s", esp_err_to_name(err));
        status_set_error();
        return;
    }

    state_ = State::STOPPED;
    status_clear_error();
    ESP_LOGI(TAG, "Fetap Speaker stopped successfully.");
}

size_t FetapSpeaker::write_(const uint8_t* const data, const size_t length, const TickType_t ticks_to_wait) {
    const size_t n_samples = length / sizeof(int16_t);
    buffer_.resize(n_samples);
    memcpy(buffer_.data(), data, length);
    for (size_t i = 0; i < n_samples; i++) {
        buffer_.at(i) = buffer_.at(i) >> kAudioGainShift;
    }
    size_t n_bytes_written{0};
    const esp_err_t err = i2s_channel_write(i2s_tx_channel_, buffer_.data(), length, &n_bytes_written, ticks_to_wait);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error writing to I2S channel: %s", esp_err_to_name(err));
        status_set_warning();
    }

    return n_bytes_written;
}

void FetapSpeaker::loop(void) {
    switch (state_) {
        case State::STOPPED:
            break;
        case State::STARTING:
            start_();
            break;
        case State::RUNNING:
            break;
        case State::STOPPING:
            stop_();
            break;
        default:
            ESP_LOGE(TAG, "Encountered unknown state");
            break;
    }
}

}
}