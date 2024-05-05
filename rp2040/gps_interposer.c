#include <stdio.h>
#include <string.h>
#include <time.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define UART0_ID            uart0
#define UART0_TX_PIN        0
#define UART0_RX_PIN        1
#define UART0_BAUD_RATE     9600

#define UART1_ID            uart1
#define UART1_TX_PIN        4
#define UART1_RX_PIN        5
#define UART1_BAUD_RATE     9600

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

void xmit_msg(void *uart_id, char msg[], int len)
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

int main() {
    stdio_init_all();
    hw_init();

    enum eState state = WAIT_FIRST_0X40;

    char msg_id[10] = "\0\0\0";

    char tx_buf[1024];
    int offset  = 0;
    int exp_msg_len = 0;


    // <<< Co - utc/ionospheric data input: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], [44], [13, 10] - 29 - 6390504500.0
    char msg[]  = {64, 64, 67, 111, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 44, 13, 10 };

    //for(int i=0;i<sizeof(msg);++i)
    //    uart_putc(UART0_ID, msg[i]);
    xmit_msg(UART0_ID, msg, sizeof(msg));

    char checksum = 0;

    while(true){
        char c = uart_getc(UART0_ID);

        tx_buf[offset++] = c;

        if (true){
            printf("%02x ", c);
            //uart_putc(UART1_ID, c);
            //continue;
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

                    if (strcmp(msg_id, "Ha") == 0){
                        struct tm date  = { 0 };

                        date.tm_mday    = tx_buf[4];
                        date.tm_mon     = tx_buf[5] - 1;
                        date.tm_year    = (tx_buf[6] * 256) + tx_buf[7] - 1900; // Year since 1900

                        time_t time = mktime(&date);
                    
                        int weeks_to_add = 1024;
                        time_t seconds_to_add = weeks_to_add * 7 * 24 * 60 * 60;
                    
                        time += seconds_to_add;
                    
                        struct tm new_date; 
                        localtime_r(&time, &new_date);

                        tx_buf[4]       = new_date.tm_mday;
                        tx_buf[5]       = new_date.tm_mon + 1;

                        int new_year    = new_date.tm_year + 1900;
                        tx_buf[6]       = new_year >> 8;
                        tx_buf[7]       = new_year & 255;
                    }

                    char new_checksum = calc_checksum(tx_buf, 2, offset-4);

                    tx_buf[offset-3]    = new_checksum;

                    xmit_msg(UART1_ID, tx_buf, offset);
                    //uart_putc(UART1_ID, checksum);
                    //uart_putc(UART1_ID, new_checksum);
                }
                else{
                    state               = WAIT_FIRST_0X40;
                }
                break;
            }
        }

        #if 0
        printf("Hello, world!\n");
        uart_puts(UART0_ID, "Hello world UART0\n");
        uart_puts(UART1_ID, "Hello world UART1\n");
        sleep_ms(1000);
        #endif
    }
}
