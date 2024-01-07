#include "rfcommon.h"

#include "hardware/uart.h"
#include "hardware/irq.h"


#define UART_ID uart0
//#define BAUD_RATE 115200
#define BAUD_RATE 9600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define HW_BUF_SIZE             1024

static Byte buf[HW_BUF_SIZE];
static int readPos=0;
static int limit = HW_BUF_SIZE-1;    // (readPos-1) % HW_BUF_SIZE 
static volatile int writePos=0;

static volatile uint16_t lostChars=0;


// https://github.com/raspberrypi/pico-examples/blob/master/uart/uart_advanced/uart_advanced.c

// Serial input and output



// RX interrupt handler
static void on_uart_rx() {
    while (uart_is_readable(UART_ID)) {
        if (writePos != limit) {
            buf[writePos]=uart_getc(UART_ID);
            writePos=(writePos+1) % HW_BUF_SIZE;
        } else {
            lostChars++;
        }
    }
}




void initSerial() {
    // Set up our UART with a basic baud rate.
    uart_init(UART_ID, 2400);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as close as
    // possible to that requested
    int __unused actual = uart_set_baudrate(UART_ID, BAUD_RATE);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);

    // OK, all set up.
    // Lets send a basic string out, and then run a loop and wait for RX interrupts
    // The handler will count them, but also reflect the incoming data back with a slight change!
    uart_puts(UART_ID, WELCOME_STRING);
}

char serialNextChar() {
    // wait for input
    while(readPos == writePos) ;

    char c=buf[readPos];
    limit=readPos;
    readPos=(readPos+1) % HW_BUF_SIZE;
    return c;
}

void serialEmitChar (char c) {
    uart_putc(UART_ID, c);
}

void serialEmitStr (char *str) {
    uart_puts(UART_ID, str);
}

void serialEmitNewline () {
    uart_puts(UART_ID, "\r\n");
}


int serialLostChars() {
    return lostChars;
}

void DEBUG (char *msg) {
    uart_puts(UART_ID, msg);
    uart_puts(UART_ID,"\r\n");
}

void DEBUGint (char *name, int value) {
    uart_puts(UART_ID, "   ");
    uart_puts(UART_ID, name);
    sprintf(buf,"=%d",value);
    uart_puts(UART_ID, buf);
    uart_puts(UART_ID,"\r\n");
}

void DEBUGstr (char *name, char *value) {
    uart_puts(UART_ID, "   ");
    uart_puts(UART_ID, name);
    uart_putc(UART_ID,'=');
    uart_puts(UART_ID, value);
    uart_puts(UART_ID,"\r\n");
}


