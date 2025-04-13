#include "Arduino.h"
#include "iot_button.h"
#include "button_gpio.h"

#include "state.h"
// #include "log.h"
#include "support.h"
#include "settings.h"
#include "global.h"

static void button_long_event_cb(void *arg, void *data)
{
  log_i("Button","longHode_click");
}

static void button_double_click_event_cb(void *arg, void *data)
{
    changeState(STATE_WEB_CONFIG_NOT_STARTED);
    // if(logConfig.logEnable == 0)
    // {
    //     logConfig.logEnable = 1;
    // }
    // else
    // {
    //     logConfig.logEnable = 0;
    // }
}

void beginButton()
{
    button_config_t btn_cfg = {
        .long_press_time = 3000,
        .short_press_time = 180,
    };
    button_gpio_config_t gpio_cfg = {
        .gpio_num = (gpio_num_t)0,
        .active_level = 0,
        .enable_power_save = true,
        .disable_pull = false,
    };
    button_handle_t btn;
    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn);
    assert(ret == ESP_OK);

    ret |= iot_button_register_cb(btn, BUTTON_LONG_PRESS_HOLD, NULL, button_long_event_cb, nullptr);
    ret |= iot_button_register_cb(btn, BUTTON_DOUBLE_CLICK, NULL, button_double_click_event_cb, nullptr);
}