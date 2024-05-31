#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"

// https://github.com/ForsakenNGS/Pico_WS2812
#include "../shared/WS2812.hpp"

#define UART0_ID            uart0
#define UART0_TX_PIN        0
#define UART0_RX_PIN        1
#define UART0_BAUD_RATE     9600

#define UART1_ID            uart1
#define UART1_TX_PIN        4
#define UART1_RX_PIN        5
#define UART1_BAUD_RATE     9600

#define LED_PIN             16
#define LED_LENGTH          1
#define LED_BLINK_PERIOD_US 1000000

enum eState {
    WAIT_FIRST_0X40     = 0,
    WAIT_SECOND_0X40    = 1,
    MESSAGE_ID0         = 2,
    MESSAGE_ID1         = 3,
    MESSAGE_DATA        = 4,
    MESSAGE_CHECKSUM    = 5,
    MESSAGE_TERMINATOR0 = 6,
    MESSAGE_TERMINATOR1 = 7
};

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

int get_msg_len(char *msg_id, bool req)
{
    if (strcmp(msg_id, "Aw") == 0){
        return req ? 8 : 8;
    }
    if (strcmp(msg_id, "Bp") == 0){
        return req ? 8 : -1;
    }
    if (strcmp(msg_id, "Bj") == 0){
        return req ? 8 : 8;
    }
    if (strcmp(msg_id, "Cf") == 0){
        return req ? 7 : 7;
    }
    if (strcmp(msg_id, "Co") == 0){
        return req ? -1 : 29;
    }
    if (strcmp(msg_id, "Gd") == 0){
        return req ? 8 : 8;
    }
    if (strcmp(msg_id, "Ge") == 0){
        return req ? 8 : 8;
    }
    if (strcmp(msg_id, "Gf") == 0){
        return req ? 9 : 9;
    }
    if (strcmp(msg_id, "Gj") == 0){
        return req ? 7 : 21;
    }
    if (strcmp(msg_id, "Ha") == 0){
        return req ? 8 : 154;
    }
    if (strcmp(msg_id, "Hn") == 0){
        return req ? 8 : 78;
    }
    return -1;
}

void xmit_msg(uart_inst_t *uart_id, char msg[], int len)
{
    for(int i=0;i<len;++i){
        uart_putc(uart_id, msg[i]);
    }
}

char calc_checksum(char msg[], int start, int end)
{
    char checksum   = 0;

    for(int i=start;i<=end;++i){
        checksum ^= msg[i];
    }

    return checksum;
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

    #if 0
    test_alive();
    #endif

    enum eState state = WAIT_FIRST_0X40;

    char msg_id[10] = "\0\0\0";

    char tx_buf[1024];
    int offset  = 0;
    int exp_msg_len = 0;


    #if 0
    // <<< Co - utc/ionospheric data input: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], [44], [13, 10] - 29 - 6390504500.0
    char msg[]  = {64, 64, 67, 111, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 44, 13, 10 };
    xmit_msg(UART0_ID, msg, sizeof(msg));
    #endif

    char checksum = 0;

    uint64_t    next_led_flip_time_us   = time_us_64() + LED_BLINK_PERIOD_US;
    int         led_state = 1;

    while(true){

        char c;
        if (uart_is_readable(UART0_ID)){
            c = uart_getc(UART0_ID);
            next_led_flip_time_us  = time_us_64() + LED_BLINK_PERIOD_US;

            if (led_state != 2){
                ledStrip.fill( WS2812::RGB(0, 64, 0) );
                ledStrip.show();
            }
            led_state   = 2;
        }
        else{
            if (time_us_64() > next_led_flip_time_us){
                //printf("%d ", led_state);
                next_led_flip_time_us   += LED_BLINK_PERIOD_US;

                led_state   = led_state ? 0 : 1;
                ledStrip.fill( WS2812::RGB(64 * led_state, 0, 0) );
                ledStrip.show();
            }
            continue;
        }

        tx_buf[offset++] = c;

        if (false){
            printf("%02x ", c);
        }

        switch(state){
            case WAIT_FIRST_0X40:{
                offset = 0;

                if (c == 0x40){
                    tx_buf[offset++] = c;
                    state = WAIT_SECOND_0X40;
                }
                break;
            }

            case WAIT_SECOND_0X40:{
                if (c == 0x40){
                    checksum    = 0x00;
                    state = MESSAGE_ID0;
                }
                else{
                    state = WAIT_FIRST_0X40;
                }
                break;
            }

            case MESSAGE_ID0:{
                msg_id[0]           = c;
                checksum            ^= c;
                state               = MESSAGE_ID1;
                break;
            }

            case MESSAGE_ID1:{
                msg_id[1]           = c;
                checksum            ^= c;
                exp_msg_len         = get_msg_len(msg_id, false);

                if (exp_msg_len == -1){
                    state       = WAIT_FIRST_0X40;
                }
                else if (exp_msg_len == 7){
                    state       = MESSAGE_CHECKSUM;
                }
                else{
                    state       = MESSAGE_DATA;
                }
                break;
            }
            case MESSAGE_DATA:{
                checksum            ^= c;

                if (offset == exp_msg_len-3){
                    state       = MESSAGE_CHECKSUM;
                }
                break;
            }
            case MESSAGE_CHECKSUM:{
                if (c == checksum){
                    state               = MESSAGE_TERMINATOR0;
                }
                else{
                    state               = WAIT_FIRST_0X40;
                }
                break;
            }

            case MESSAGE_TERMINATOR0:{
                if (c == 0x0d){
                    state               = MESSAGE_TERMINATOR1;
                }
                else{
                    state               = WAIT_FIRST_0X40;
                }
                break;
            }

            case MESSAGE_TERMINATOR1:{
                if (c == 0x0a){
                    state               = WAIT_FIRST_0X40;

                    printf("msg: %s\n", msg_id);

                    if (strcmp(msg_id, "Ha") == 0){
                        struct tm date  = { 0 };

                        // Extract time from packet
                        date.tm_mon     = tx_buf[4] - 1;
                        date.tm_mday    = tx_buf[5];
                        date.tm_year    = (tx_buf[6] * 256) + tx_buf[7] - 1900; // Year since 1900

                        printf("Original date: %d/%d/%d\n", date.tm_year + 1900, date.tm_mon+1, date.tm_mday);

                        // Add 1024 weeks by adding the number of days. 
                        date.tm_mday    += 1024 * 7;

                        // Normalize...
                        time_t time     = mktime(&date);
                    
                        // Extract date
                        struct tm new_date; 
                        localtime_r(&time, &new_date);

                        tx_buf[4]       = new_date.tm_mon + 1;
                        tx_buf[5]       = new_date.tm_mday;

                        int new_year    = new_date.tm_year + 1900;
                        tx_buf[6]       = new_year >> 8;
                        tx_buf[7]       = new_year & 255;

                        printf("Adjusted date: %d/%d/%d\n", new_date.tm_year + 1900, new_date.tm_mon+1, new_date.tm_mday);

                        // Set cold start to 0
                        tx_buf[129]     = tx_buf[129] & 127;

                        // Set clock bias to some number
                        tx_buf[133]     = 29;
                        tx_buf[134]     = 0;

                        // Set oscillator offset to some number
                        tx_buf[135]     = 1;
                        tx_buf[136]     = 1;
                        tx_buf[137]     = 0;
                        tx_buf[138]     = 0;

                        char new_checksum = calc_checksum(tx_buf, 2, offset-4);
                        tx_buf[offset-3]    = new_checksum;
                    }
                    else if (strcmp(msg_id, "Hn") == 0){
                        // time solution accuracy
                        tx_buf[12]      = 0xff;
                        tx_buf[13]      = 0xff;

                        char new_checksum = calc_checksum(tx_buf, 2, offset-4);
                        tx_buf[offset-3]    = new_checksum;
                    }
                    else if (strcmp(msg_id, "Co") == 0){
                        char co_data[] = { 36, 1, 253, 0, 67, 0, 251, 4, 0, 0, 0, 1, 0, 0, 0, 1, 18, 36, 4, 137, 7, 18 };
                        for(int i=0;i<sizeof(co_data); ++i){
                            tx_buf[i+4] = co_data[i];
                        }

                        char new_checksum = calc_checksum(tx_buf, 2, offset-4);
                        tx_buf[offset-3]    = new_checksum;
                    }

                    xmit_msg(UART1_ID, tx_buf, offset);
                }
                else{
                    state               = WAIT_FIRST_0X40;
                }
                break;
            }
        }

    }
}
