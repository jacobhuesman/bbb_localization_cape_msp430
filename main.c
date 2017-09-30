#include <msp430g2553.h>


#define TXLED BIT0
#define RXLED BIT6


unsigned int i; //Counter

int main(void)
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
    UCA0BR0 = 0x08; // 1MHz 115200
    UCA0BR1 = 0x00; // 1MHz 115200
    UCA0MCTL = UCBRS2 + UCBRS0; // Modulation UCBRSx = 5
    UCA0CTL1 &= ~UCSWRST; // **Initialize USCI state machine**
    //UC0IE |= UCA0RXIE; // Enable USCI_A0 RX interrupt
    UC0IE |= UCA0TXIE; // Enable USCI_A0 TX interrupt
    __bis_SR_register(CPUOFF + GIE); // Enter LPM0 w/ int until Byte RXed
    while (1)
    { }
}

#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
   P1OUT |= TXLED;
     //UCA0TXBUF = string[i++]; // TX next character
     UCA0TXBUF = 'A';
    //if (i == sizeof string - 1) // TX over?
    //   i=0;
    P1OUT &= ~TXLED; }

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
   P1OUT |= RXLED;
    if (UCA0RXBUF == 'a') // 'a' received?
    {
       i = 0;
       UC0IE |= UCA0TXIE; // Enable USCI_A0 TX interrupt
      //UCA0TXBUF = string[i++];
    }
    P1OUT &= ~RXLED;
}
