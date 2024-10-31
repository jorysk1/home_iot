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
#define BUTTON_1_NODE DT_ALIAS(button_1)
#define BUTTON_2_NODE DT_ALIAS(button_2)
#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
    ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

#define DEBOUNCE_DELAY_MS 1000

static int64_t last_press_time_1 = 0;
static int64_t last_press_time_2 = 0;

const struct gpio_dt_spec led_yellow_gpio = GPIO_DT_SPEC_GET_OR(LED_YELLOW_NODE, gpios, {0});
static const struct i2c_dt_spec lcd_screen = I2C_DT_SPEC_GET(LCD_SCREEN_NODE);
const struct device *const dht11 = DEVICE_DT_GET_ONE(aosong_dht);
static const struct adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};

const struct gpio_dt_spec button_1 = GPIO_DT_SPEC_GET(BUTTON_1_NODE, gpios);
const struct gpio_dt_spec button_2 = GPIO_DT_SPEC_GET(BUTTON_2_NODE, gpios);

static struct gpio_callback button_cb;

void lcd_display_handler_button_1(struct k_work *work);
void lcd_display_handler_button_2(struct k_work *work);

K_WORK_DEFINE(lcd_display_work_1, lcd_display_handler_button_1);
K_WORK_DEFINE(lcd_display_work_2, lcd_display_handler_button_2);

void lcd_display_handler_button_1(struct k_work *work) {
    write_lcd(&lcd_screen, "Bouton 1 appuyé!", LCD_LINE_1);
    write_lcd(&lcd_screen, "Affichage LCD", LCD_LINE_2);
}

void lcd_display_handler_button_2(struct k_work *work) {
    write_lcd(&lcd_screen, "Bouton 2 appuyé!", LCD_LINE_1);
    write_lcd(&lcd_screen, "Affichage LCD", LCD_LINE_2);
}

void button_pressed_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    int64_t now = k_uptime_get(); 

    if (pins & BIT(button_1.pin)) {
        if (now - last_press_time_1 > DEBOUNCE_DELAY_MS) {
            last_press_time_1 = now;
            printk("Appui détecté sur le Bouton 1\n");
            k_work_submit(&lcd_display_work_1);  
        }
    }

    if (pins & BIT(button_2.pin)) {
        if (now - last_press_time_2 > DEBOUNCE_DELAY_MS) {
            last_press_time_2 = now;
            printk("Appui détecté sur le Bouton 2\n");
            k_work_submit(&lcd_display_work_2);  
        }
    }
}

int main(void) {
    gpio_pin_configure_dt(&led_yellow_gpio, GPIO_OUTPUT_HIGH);
    printf("Hello World! %s\n", CONFIG_BOARD_TARGET);
    init_lcd(&lcd_screen);

    gpio_pin_configure_dt(&button_1, GPIO_INPUT);
    gpio_pin_configure_dt(&button_2, GPIO_INPUT);

    gpio_pin_interrupt_configure_dt(&button_1, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_pin_interrupt_configure_dt(&button_2, GPIO_INT_EDGE_TO_ACTIVE);

    gpio_init_callback(&button_cb, button_pressed_handler, BIT(button_1.pin) | BIT(button_2.pin));
    gpio_add_callback(button_1.port, &button_cb);
    gpio_add_callback(button_2.port, &button_cb);

    while (1) {
        int ret = sensor_sample_fetch(dht11);
        if (ret == 0) {
            struct sensor_value temp, humidity;
            sensor_channel_get(dht11, SENSOR_CHAN_AMBIENT_TEMP, &temp);
            sensor_channel_get(dht11, SENSOR_CHAN_HUMIDITY, &humidity);
            printk("Temperature: %d.%06d C, Humidity: %d.%06d %%\n", temp.val1, temp.val2, humidity.val1, humidity.val2);
        } else {
            printk("Erreur récup des données du capteur (%d)\n", ret);
        }

        uint32_t count = 0;
        uint16_t buf;
        struct adc_sequence sequence = {
            .buffer = &buf,
            .buffer_size = sizeof(buf),
        };

        for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
            if (!adc_is_ready_dt(&adc_channels[i])) {
                printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
                continue; 
            }

            int err = adc_channel_setup_dt(&adc_channels[i]);
            if (err < 0) {
                printk("Could not setup channel #%d (%d)\n", i, err);
                continue; 
            }

            for (int k = 0; k < 10; k++) {
                int32_t val_mv;
                printk("ADC reading[%u]:\n", count++);
                (void)adc_sequence_init_dt(&adc_channels[i], &sequence);
                err = adc_read_dt(&adc_channels[i], &sequence);
                if (err < 0) {
                    printk("Could not read (%d)\n", err);
                    continue;
                }

                val_mv = (adc_channels[i].channel_cfg.differential) ? (int32_t)((int16_t)buf) : (int32_t)buf;
                printk("- %s, channel %d: ", adc_channels[i].dev->name, adc_channels[i].channel_id);
                err = adc_raw_to_millivolts_dt(&adc_channels[i], &val_mv);
                if (err < 0) {
                    printk(" (value in mV not available)\n");
                } else {
                    printk(" = %" PRId32 " mV\n", val_mv);
                }
                k_sleep(K_SECONDS(1)); 
            }
        }
        k_sleep(K_SECONDS(10)); 
    }
}