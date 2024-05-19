#include <stdio.h>
#include <string.h>
#include <time.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "../shared/m12_parser.h"

#define UART0_ID            uart0
#define UART0_TX_PIN        0
#define UART0_RX_PIN        1
#define UART0_BAUD_RATE     9600

#define UART1_ID            uart1
#define UART1_TX_PIN        4
#define UART1_RX_PIN        5
#define UART1_BAUD_RATE     9600

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

int main() {
    stdio_init_all();
    hw_init();

    M12Parser parser;


    // <<< Co - utc/ionospheric data input: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], [44], [13, 10] - 29 - 6390504500.0
    char msg[]  = {64, 64, 67, 111, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 44, 13, 10 };

    xmit_msg(UART0_ID, msg, sizeof(msg));

    char checksum = 0;

    while(true){
        char c = uart_getc(UART0_ID);

        if (true){
            printf("%02x ", c);
            //uart_putc(UART1_ID, c);
            //continue;
        }

        #if 0
        printf("Hello, world!\n");
        uart_puts(UART0_ID, "Hello world UART0\n");
        uart_puts(UART1_ID, "Hello world UART1\n");
        sleep_ms(1000);
        #endif
    }
}
