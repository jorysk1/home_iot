/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include "lcd_screen_i2c.h"

#define LED_YELLOW_NODE DT_ALIAS(led_yellow)
#define LCD_SCREEN_NODE DT_ALIAS(lcd_screen)
#define DHT11_NODE DT_ALIAS(dht11)
#define I2C_DEVICE
#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
    ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

const struct gpio_dt_spec led_yellow_gpio = GPIO_DT_SPEC_GET_OR(LED_YELLOW_NODE, gpios, {0});
static const struct i2c_dt_spec lcd_screen = I2C_DT_SPEC_GET(LCD_SCREEN_NODE);
const struct device *const dht11 = DEVICE_DT_GET_ONE(aosong_dht);
static const struct adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
                         DT_SPEC_AND_COMMA)};

int main(void)
{
    gpio_pin_configure_dt(&led_yellow_gpio, GPIO_OUTPUT_HIGH);
    printf("Hello World! %s\n", CONFIG_BOARD_TARGET);
    init_lcd(&lcd_screen);

    while (1)
    {
        int ret = sensor_sample_fetch(dht11);
        int err;
        uint32_t count = 0;
        uint16_t buf;
        struct adc_sequence sequence = {
            .buffer = &buf,
            .buffer_size = sizeof(buf),
        };

        for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++)
        {
            if (!adc_is_ready_dt(&adc_channels[i]))
            {
                printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
                return 0;
            }

            err = adc_channel_setup_dt(&adc_channels[i]);
            if (err < 0)
            {
                printk("Could not setup channel #%d (%d)\n", i, err);
                return 0;
            }
            if (ret == 0)
            {

                struct sensor_value temp, humidity;
                sensor_channel_get(dht11, SENSOR_CHAN_AMBIENT_TEMP, &temp);
                sensor_channel_get(dht11, SENSOR_CHAN_HUMIDITY, &humidity);

                printk("Temperature: %d.%06d C, Humidity: %d.%06d %%\n", temp.val1, temp.val2, humidity.val1, humidity.val2);
            }
            else
            {

                printk("Erreur récup des données du capteur(%d)\n", ret);
            }
            for (int k = 0; k < 10; k++)
            {

                printk("ADC reading[%u]:\n", count++);
                for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++)
                {
                    int32_t val_mv;

                    printk("- %s, channel %d: ",
                           adc_channels[i].dev->name,
                           adc_channels[i].channel_id);

                    (void)adc_sequence_init_dt(&adc_channels[i], &sequence);

                    err = adc_read_dt(&adc_channels[i], &sequence);
                    if (err < 0)
                    {
                        printk("Could not read (%d)\n", err);
                        continue;
                    }

                    if (adc_channels[i].channel_cfg.differential)
                    {
                        val_mv = (int32_t)((int16_t)buf);
                    }
                    else
                    {
                        val_mv = (int32_t)buf;
                    }
                    printk("%" PRId32, val_mv);
                    err = adc_raw_to_millivolts_dt(&adc_channels[i],
                                                   &val_mv);
                    if (err < 0)
                    {
                        printk(" (value in mV not available)\n");
                    }
                    else
                    {
                        printk(" = %" PRId32 " mV\n", val_mv);
                    }
                }
                k_sleep(K_SECONDS(10));
            }
        }
    }
}