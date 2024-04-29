#include <stdio.h>
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

int main() {
    stdio_init_all();

    uart_init(UART0_ID, UART0_BAUD_RATE);
    gpio_set_function(UART0_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART0_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART0_ID, false, false);

    uart_init(UART1_ID, UART1_BAUD_RATE);
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART1_ID, false, false);

    while(true){
        printf("Hello, world!\n");
        uart_puts(UART0_ID, "Hello world UART0\n");
        uart_puts(UART1_ID, "Hello world UART1\n");
        sleep_ms(1000);
    }
}
