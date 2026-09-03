#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// Seeed XIAO ESP32S3 + reSpeaker XVF3800 4-Mic Array
//
// Audio flows over a full-duplex I2S (STD/Philips) link between the ESP32-S3
// (I2S master) and the XMOS XVF3800 (I2S slave). The XVF3800 runs its own
// beamforming/AEC/AGC, so no external codec chip is used and the ESP32 does
// not do device-side AEC. Both directions share one BCLK/WS domain, therefore
// input and output sample rates must be equal.

#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 16000

// I2S pins (XIAO ESP32S3 breakout labels in parentheses)
// BCLK  -> D9
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_8
// LRCK/WS -> D8
#define AUDIO_I2S_GPIO_WS   GPIO_NUM_7
// DOUT (ESP32 -> XVF3800 playback / AEC reference) -> D7
// NOTE: GPIO44 is the default UART0 RX pin; console must run on USB instead.
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_44
// DIN (XVF3800 -> ESP32 processed mic) -> D6
// NOTE: GPIO43 is the default UART0 TX pin; console must run on USB instead.
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_43

// I2C control bus (used only to probe the XVF3800 at boot for logging).
// SDA -> D4, SCL -> D5
#define I2C_SDA_PIN GPIO_NUM_5
#define I2C_SCL_PIN GPIO_NUM_6

// XVF3800 control address
#define XVF3800_I2C_ADDR 0x2C

// Buttons / LEDs
#define BOOT_BUTTON_GPIO GPIO_NUM_0
#define BUILTIN_LED_GPIO GPIO_NUM_NC

#endif // _BOARD_CONFIG_H_
