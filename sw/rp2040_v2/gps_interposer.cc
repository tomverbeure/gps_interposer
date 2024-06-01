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

void xmit_msg(uart_inst *uart_id, const char msg[], int len)
{
    printf("xmit_msg: ");
    for(int i=0;i<len;++i){
        printf("%02x ", msg[i]);
        uart_putc(uart_id, msg[i]);
        sleep_us(2000);
    }
    printf("\n");
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

struct tx_element
{
    int     wait_us;
    char    data;
};

int main() {
    stdio_init_all();
    hw_init();

    WS2812 ledStrip(LED_PIN, LED_LENGTH, pio0, 0, WS2812::FORMAT_RGB);

    ledStrip.fill( WS2812::RGB(64, 0, 0) );
    ledStrip.show();

    M12Parser req_parser(true);
    M12Parser resp_parser(false);

    uint64_t    next_led_flip_time_us   = time_us_64() + LED_BLINK_PERIOD_US;
    int         led_state = 1;

    char tx_buf[1024];
    int tx_buf_wr_ptr   = 0;
    int tx_buf_rd_ptr   = 0;

    uint64_t    next_uart_tx_time_us    = time_us_64();

    while(true){

        //============================================================
        // Request by carrier
        //===========================================================
        if (uart_is_readable(UART1_ID)){
            char c_from_carrier = uart_getc(UART1_ID);

            next_led_flip_time_us  = time_us_64() + LED_BLINK_PERIOD_US;
            if (led_state != 2){
                ledStrip.fill( WS2812::RGB(0, 64, 0) );
                ledStrip.show();
            }
            led_state   = 2;

            tx_buf[tx_buf_wr_ptr] = c_from_carrier;
            tx_buf_wr_ptr   = (tx_buf_wr_ptr + 1) % 1024;


            bool req_msg_complete = req_parser.parse(c_from_carrier);
            if (req_msg_complete){
                printf("req : %s - %d\n", req_parser.get_msg_id(), req_parser.get_msg_len(req_parser.get_msg_id()) );

                if (strcmp(req_parser.get_msg_id(), "Cf") == 0){
                    printf("Insert Gb...\n");
                    // Insert time correction command
                    char gb_set_msg[]  = { '@',  '@', 'G', 'b', 
                                            5,                                  // month
                                            4,                                  // day
                                            2024>>8, 2024&255,                  // year
                                            0,                                  // hours
                                            0,                                  // minutes
                                            0,                                  // seconds
                                            0,                                  // GMT offset sign
                                            0,                                  // GMT hour offset
                                            0,                                  // GMT minute offset
                                            0,                                  // checksum
                                            0x0d,                               // terminator
                                            0x0a };                             // terminator

                    gb_set_msg[sizeof(gb_set_msg)-3]  = req_parser.calc_checksum(gb_set_msg, 2, 13);

                    for(int i=0;i<sizeof(gb_set_msg);++i){
                        tx_buf[tx_buf_wr_ptr]   = gb_set_msg[i];
                        tx_buf_wr_ptr   = (tx_buf_wr_ptr + 1) % 1024;
                    }
                    //xmit_msg(UART0_ID, gb_set_msg, sizeof(gb_set_msg));

                    next_uart_tx_time_us    = time_us_64() + 50000;

                    #if 0
                    char gb_query_msg[]  = { '@',  '@', 'G', 'b', 
                                            0xff,                               // month
                                            0xff,                               // day
                                            0xff, 0xff,                         // year
                                            0xff,                               // hours
                                            0xff,                               // minutes
                                            0xff,                               // seconds
                                            0xff,                               // GMT offset sign
                                            0xff,                               // GMT hour offset
                                            0xff,                               // GMT minute offset
                                            0x25,                               // checksum
                                            0x0d,                               // terminator
                                            0x0a };                             // terminator
                    xmit_msg(UART0_ID, gb_query_msg, sizeof(gb_query_msg));
                    #endif
                }
            }
        }

        if (tx_buf_rd_ptr != tx_buf_wr_ptr){
            if (time_us_64() > next_uart_tx_time_us){
                uart_putc(UART0_ID, tx_buf[tx_buf_rd_ptr]);
                tx_buf_rd_ptr   = (tx_buf_rd_ptr + 1) % 1024;
                next_uart_tx_time_us    = time_us_64() + 2000;
            }
        }

        //============================================================
        // Response by module
        //============================================================
        if (uart_is_readable(UART0_ID)){
            char c_from_module = uart_getc(UART0_ID);
            next_led_flip_time_us  = time_us_64() + LED_BLINK_PERIOD_US;

            if (led_state != 2){
                ledStrip.fill( WS2812::RGB(0, 64, 0) );
                ledStrip.show();
            }
            led_state   = 2;

            uart_putc(UART1_ID, c_from_module);

            bool resp_msg_complete = resp_parser.parse(c_from_module);
            if (resp_msg_complete){
                printf("resp: %s - %d\n", resp_parser.get_msg_id(), resp_parser.get_msg_len(resp_parser.get_msg_id()) );

                if (strcmp(resp_parser.get_msg_id(), "Ha") == 0){
                    struct tm date  = { 0 };

                    // Extract time from packet
                    date.tm_mon     = resp_parser.buf[4] - 1;
                    date.tm_mday    = resp_parser.buf[5];
                    date.tm_year    = (resp_parser.buf[6] * 256) + resp_parser.buf[7] - 1900; // Year since 1900

                    printf("Date: %d/%d/%d\n", date.tm_year + 1900, date.tm_mon+1, date.tm_mday);
                }

            }

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
