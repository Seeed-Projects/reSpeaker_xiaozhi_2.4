#include "respeaker_xvf3800_audio_codec.h"

#include <esp_log.h>
#include <cmath>
#include <cstdint>

#define TAG "RespeakerCodec"

RespeakerXvf3800AudioCodec::RespeakerXvf3800AudioCodec(int sample_rate, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din) {
    duplex_ = true;
    input_sample_rate_ = sample_rate;
    output_sample_rate_ = sample_rate;
    input_channels_ = 1;
    output_channels_ = 1;

    i2s_chan_config_t chan_cfg = {
        .id = XIAOZHI_I2S_PORT(0),
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM,
        .dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM,
        .auto_clear_after_cb = true,
        .auto_clear_before_cb = false,
        .intr_priority = 0,
    };
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle_, &rx_handle_));

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = (uint32_t)output_sample_rate_,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
#ifdef I2S_HW_VERSION_2
            .ext_clk_freq_hz = 0,
#endif
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_STEREO,
            .slot_mask = I2S_STD_SLOT_BOTH,
            .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
            .ws_pol = false,
            .bit_shift = true,
#ifdef I2S_HW_VERSION_2
            .left_align = true,
            .big_endian = false,
            .bit_order_lsb = false,
#endif
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = bclk,
            .ws = ws,
            .dout = dout,
            .din = din,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle_, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_, &std_cfg));
    ESP_LOGI(TAG, "Stereo 32-bit duplex channels created (%d Hz)", sample_rate);
}

int RespeakerXvf3800AudioCodec::Write(const int16_t* data, int samples) {
    std::lock_guard<std::mutex> lock(data_if_mutex_);
    // Expand N mono samples into N stereo 32-bit frames, duplicating each
    // sample onto both the left and right slots.
    std::vector<int32_t> buffer(samples * 2);
    int32_t volume_factor = static_cast<int32_t>(pow(double(output_volume_) / 100.0, 2) * 65536);
    for (int i = 0; i < samples; i++) {
        int64_t temp = int64_t(data[i]) * volume_factor;
        int32_t value = (temp > INT32_MAX) ? INT32_MAX : (temp < INT32_MIN) ? INT32_MIN : static_cast<int32_t>(temp);
        buffer[i * 2] = value;
        buffer[i * 2 + 1] = value;
    }

    size_t bytes_written = 0;
    ESP_ERROR_CHECK(i2s_channel_write(tx_handle_, buffer.data(), buffer.size() * sizeof(int32_t), &bytes_written, portMAX_DELAY));
    return bytes_written / (2 * sizeof(int32_t));
}

int RespeakerXvf3800AudioCodec::Read(int16_t* dest, int samples) {
    size_t bytes_read = 0;
    constexpr uint32_t kReadTimeoutMs = 200;

    std::vector<int32_t> bit32_buffer(samples * 2);
    if (i2s_channel_read(rx_handle_, bit32_buffer.data(), bit32_buffer.size() * sizeof(int32_t), &bytes_read, kReadTimeoutMs) != ESP_OK) {
        return 0;
    }

    // Collapse the stereo uplink to mono using the left slot (channel 0).
    int frames = bytes_read / (2 * sizeof(int32_t));
    for (int i = 0; i < frames; i++) {
        int32_t value = bit32_buffer[i * 2] >> 12;
        dest[i] = (value > INT16_MAX) ? INT16_MAX : (value < -INT16_MAX) ? -INT16_MAX : static_cast<int16_t>(value);
    }
    return frames;
}
