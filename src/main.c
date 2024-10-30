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

const struct gpio_dt_spec led_yellow_gpio = GPIO_DT_SPEC_GET_OR(LED_YELLOW_NODE, gpios, {0});
static const struct i2c_dt_spec lcd_screen = I2C_DT_SPEC_GET(LCD_SCREEN_NODE);
const struct device *const dht11 = DEVICE_DT_GET_ONE(aosong_dht);

int main(void)
{
	gpio_pin_configure_dt(&led_yellow_gpio, GPIO_OUTPUT_HIGH);
	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);
	init_lcd(&lcd_screen);

    while (1) {
        int ret = sensor_sample_fetch(dht11);
        if (ret == 0) {

            struct sensor_value temp, humidity;
            sensor_channel_get(dht11, SENSOR_CHAN_AMBIENT_TEMP, &temp);
            sensor_channel_get(dht11, SENSOR_CHAN_HUMIDITY, &humidity);


            printk("Temperature: %d.%06d C, Humidity: %d.%06d %%\n", temp.val1, temp.val2, humidity.val1, humidity.val2);
        } else {
           
            printk("Erreur récup des données du capteur(%d)\n", ret);
        }
        k_sleep(K_SECONDS(10));
    }
}