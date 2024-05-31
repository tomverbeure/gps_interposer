#include <stdio.h>
#include <string.h>
#include <time.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

// https://github.com/ForsakenNGS/Pico_WS2812
#include "../shared/WS2812.hpp"

#include "../shared/m12_parser.h"

// UART0: module
#define UART0_ID            uart0
#define UART0_TX_PIN        0
#define UART0_RX_PIN        1
#define UART0_BAUD_RATE     9600

// UART1: carrier/motherboard
#define UART1_ID            uart1
#define UART1_TX_PIN        4
#define UART1_RX_PIN        5
#define UART1_BAUD_RATE     9600

#define LED_PIN             16
#define LED_LENGTH          1
#define LED_BLINK_PERIOD_US 1000000

void hw_init()
{
    uart_init(UART0_ID, UART0_BAUD_RATE);
    gpio_set_function(UART0_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART0_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART0_ID, false, false);

    uart_init(UART1_ID, UART1_BAUD_RATE);
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART1_ID, false, false);
}

void xmit_msg(uart_inst *uart_id, char msg[], int len)
{
    for(int i=0;i<len;++i){
        uart_putc(uart_id, msg[i]);
    }
}

void test_alive()
{
    while(1){
        printf("Hello, world!\n");
        uart_puts(UART0_ID, "Hello world UART0\n");
        uart_puts(UART1_ID, "Hello world UART1\n");
        sleep_ms(1000);
    }
}

int main() {
    stdio_init_all();
    hw_init();

    WS2812 ledStrip(LED_PIN, LED_LENGTH, pio0, 0, WS2812::FORMAT_RGB);

    ledStrip.fill( WS2812::RGB(64, 0, 0) );
    ledStrip.show();

    M12Parser rx_parser;
    M12Parser tx_parser;

    uint64_t    next_led_flip_time_us   = time_us_64() + LED_BLINK_PERIOD_US;
    int         led_state = 1;

    while(true){
        char c_from_module;
        char c_from_carrier;

        if (uart_is_readable(UART0_ID)){
            c_from_module = uart_getc(UART0_ID);
            next_led_flip_time_us  = time_us_64() + LED_BLINK_PERIOD_US;

            if (led_state != 2){
                ledStrip.fill( WS2812::RGB(0, 64, 0) );
                ledStrip.show();
            }
            led_state   = 2;

            uart_putc(UART1_ID, c_from_module);
        }

        if (uart_is_readable(UART1_ID)){
            c_from_carrier = uart_getc(UART1_ID);
            next_led_flip_time_us  = time_us_64() + LED_BLINK_PERIOD_US;

            if (led_state != 2){
                ledStrip.fill( WS2812::RGB(0, 64, 0) );
                ledStrip.show();
            }
            led_state   = 2;

            uart_putc(UART0_ID, c_from_carrier);
        }

        if (time_us_64() > next_led_flip_time_us){
            //printf("%d ", led_state);
            next_led_flip_time_us   += LED_BLINK_PERIOD_US;

            led_state   = led_state ? 0 : 1;
            ledStrip.fill( WS2812::RGB(64 * led_state, 0, 0) );
            ledStrip.show();
            continue;
        }
    }
}
