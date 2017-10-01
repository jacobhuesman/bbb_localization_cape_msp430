#include <msp430g2553.h>
#include <intrinsics.h>
#include <stdint.h>

#define TXLED BIT0
#define RXLED BIT6
#define BUTTON BIT3


unsigned int blink = 0;


uint8_t button_pressed;


uint8_t tx_packet[20]; //TODO figure out ideal packet size
uint8_t rx_packet[20];

uint8_t tx_cb; // Current TX Byte
uint8_t rx_cb;

uint8_t packet_length = 8;

void initialize(void)
{
    // Stop WatchDog Timer (WDT)
    WDTCTL = WDTPW + WDTHOLD;

    // Set DCO
    BCSCTL1 = CALBC1_8MHZ;
    DCOCTL  = CALDCO_8MHZ;

    // Set all of port 2 as output and reset
    P2DIR |= 0xFF;
    P2OUT &= 0x00;

    // Put pin 1.1 and 1.2 in UART mode (msp430g2553.pdf page 43)
    P1SEL  |= BIT1 + BIT2;
    P1SEL2 |= BIT1 + BIT2;

    // Set the two LED pins as outputs
    P1DIR |= RXLED + TXLED;
    P1OUT &= 0x00;

    // Configure baud rate
    UCA0CTL1 |= UCSSEL_2; // SMCLK (slau144j.pdf page 430)
    UCA0BR0 = 0x08;       // 1MHz 115200
    UCA0BR1 = 0x00;       // 1MHz 115200
    UCA0CTL1 &= ~UCSWRST; // Release USCI for operation (slau144j.pdf page 445)
    UC0IE |= UCA0RXIE;    // Enable USCI_A0 RX interrupt

    // Configure Button
    P1REN |= BUTTON;
    P1OUT |= BUTTON;
    P1IES |= BUTTON;
    P1IE  |= BUTTON; //Enables the selector-mask for generating interrupts on the relevant pin
    button_pressed = 0;

    // Initialize packet
    tx_packet[0] = 0xFF;
    tx_packet[1] = 0xFF;
    tx_cb = 0;

    // Enable Interrupts
    __enable_interrupt();
}

uint8_t calculate_checksum(void)
{
    uint8_t sum = 0;
    int i;
    for (i = 2; i < (packet_length - 1); i++)
    {
        sum += tx_packet[i];
    }
    return ~sum;
}

int read_data(uint8_t id, uint8_t start_address, uint8_t data_length)
{
    packet_length = 8;

    tx_packet[2] = id;            // ID
    tx_packet[3] = 0x04;          // LENGTH
    tx_packet[4] = 0x02;          // INSTRUCTION
    tx_packet[5] = start_address; // PARAMETER1 (Start address)
    tx_packet[6] = data_length;   // PARAMETER2 (Length of data to be read)
    tx_packet[7] = calculate_checksum();

    return 1;
}

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
    button_pressed = 1;

    blink ^= 0x01;
    P1IFG &= ~BUTTON; // P1.3 IFG cleared
    P1OUT &= ~RXLED; // Clear the LEDs so they start in OFF state
}

// UART TX
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
    P1OUT |= TXLED;

    UCA0TXBUF = tx_packet[tx_cb++];
    if (tx_cb >= packet_length)
    {
        tx_cb = 0;
        UC0IE &= ~UCA0TXIE;
    }

    P1OUT &= ~TXLED;
}

// UART RX
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
    P1OUT |= RXLED;
    rx_packet[rx_cb++] = UCA0RXBUF;
    P1OUT &= ~RXLED;
}


int main(void)
{
    initialize();
    const uint8_t id = 1;

    while(1)
    {
        if(button_pressed)
        {
            // Send read request
            read_data(id, 0x03, 0x01);
            P2OUT |= BIT0;
            UC0IE |= UCA0TXIE;

            while(UC0IE & UCA0TXIE);
            __delay_cycles(88);

            P2OUT &= ~BIT0;

            __delay_cycles(1E7);
            button_pressed = 0;
            rx_cb = 0;
        }
    }

}
