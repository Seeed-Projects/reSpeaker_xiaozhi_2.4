#include "wifi_board.h"
#include "respeaker_xvf3800_audio_codec.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include <esp_log.h>
#include <driver/i2c_master.h>
#include <driver/gpio.h>

#define TAG "RespeakerXvf3800"

class RespeakerXvf3800Board : public WifiBoard {
private:
    Button boot_button_;
    i2c_master_bus_handle_t control_i2c_bus_;

    void InitializeControlI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = (i2c_port_t)I2C_NUM_0,
            .sda_io_num = I2C_SDA_PIN,
            .scl_io_num = I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        if (i2c_new_master_bus(&i2c_bus_cfg, &control_i2c_bus_) != ESP_OK) {
            ESP_LOGW(TAG, "Failed to create I2C bus for XVF3800 probe");
            return;
        }
        esp_err_t ret = i2c_master_probe(control_i2c_bus_, XVF3800_I2C_ADDR, pdMS_TO_TICKS(200));
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "reSpeaker XVF3800 detected at 0x%02X", XVF3800_I2C_ADDR);
        } else {
            ESP_LOGW(TAG, "reSpeaker XVF3800 not detected at 0x%02X: %s", XVF3800_I2C_ADDR, esp_err_to_name(ret));
        }
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
        boot_button_.OnLongPress([this]() {
            EnterWifiConfigMode();
        });
    }

public:
    RespeakerXvf3800Board() : boot_button_(BOOT_BUTTON_GPIO), control_i2c_bus_(nullptr) {
        InitializeControlI2c();
        InitializeButtons();
    }

    virtual AudioCodec* GetAudioCodec() override {
        static RespeakerXvf3800AudioCodec audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK,
            AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT,
            AUDIO_I2S_GPIO_DIN);
        return &audio_codec;
    }
};

DECLARE_BOARD(RespeakerXvf3800Board);
