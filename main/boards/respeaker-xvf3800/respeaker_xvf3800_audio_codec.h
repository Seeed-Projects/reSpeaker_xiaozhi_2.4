#pragma once

#include "codecs/no_audio_codec.h"

#include <driver/gpio.h>

// Full-duplex I2S codec for the reSpeaker XVF3800 (XIAO ESP32S3 host).
//
// The XVF3800 I2S firmware expects a 2-channel (stereo), 32-bit I2S link at
// 16 kHz, matching Seeed's reference test sketch. On TX each mono downlink
// sample is duplicated onto both the left and right slots (the XVF uses the
// far-end signal from I2S-in for its local speaker path and AEC reference).
// On RX the processed 2-channel uplink is read back as mono by taking the left
// slot (channel 0 / "Conference" output).
class RespeakerXvf3800AudioCodec : public NoAudioCodec {
public:
    RespeakerXvf3800AudioCodec(int sample_rate, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din);

protected:
    virtual int Write(const int16_t* data, int samples) override;
    virtual int Read(int16_t* dest, int samples) override;
};
