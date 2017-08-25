/*** USCI master library ************************************************************

In this file the usage of the USCI I2C master library without DMA support is 
shown. This library uses pointers to specify what data is to be sent. 


#define BCD_OUT P1OUT
// P1.0 BCD output - A0 - pin 2
// P1.1 BCD output - A1 - pin 3
// P1.2 BCD output - A2 - pin 4
// P1.3 BCD output - A3 - pin 5

// P1.6 I2C SCL - pin 14
// P1.7 I2C SDA - pin 15

#define ANODE_MUX_OUT P2OUT
// P2.0 MUX output - Tube1 - pin 8
// P2.1 MUX output - Tube2 - pin 9
// P2.2 MUX output - Tube3 - pin 10
// P2.3 MUX output - Tube4 - pin 11


*******************************************************************************/
#include "msp430g2553.h"
#include "isl1208.h"
#include "TI_USCI_I2C_master.h"

#define PRESCALER 0x14  // 0x12 is 500khz met een 1k pullup. 0x15 is 375 khz 0x14 = 400 khz
#define DELAY_I2C 400   //time after START condition
#define DELAY_MUX 50000
#define DEAD_TIME 100

//default time after power loss: 00:00:00
volatile unsigned char RTC_Hour    = 0x00;
volatile unsigned char RTC_Minutes = 0x00;
volatile unsigned char RTC_Seconds = 0x00;

unsigned char RTC_Sector_Address[] = { SC };
unsigned char RTC_Sector_Read[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
unsigned char RTC_Sector_Write[] = { SC, 0x00, 0x00, 0x00 };
unsigned char RTC_Sector_Write_Enable[] = { SR, 0x10 };

volatile unsigned int hour, min;
volatile unsigned int digit1, digit2, digit3, digit4;

void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT

  //MCLK = 8 MHz
  BCSCTL1 = CALBC1_8MHZ;
  DCOCTL = CALDCO_8MHZ;


  P1DIR     |= BIT0;                   //Diagnostic LED
  P1OUT     |= BIT0;

	P1DIR |= 0xF9;					// Set P1.0 - P1.3 to output direction
	P1OUT = 0x00;					// All cathodes off

	P2DIR |= 0x0F;					// Set P2.0 - P2.3 to output direction
	P2OUT = 0x00;					// All anodes off

  _EINT();

  RTC_Sector_Write[3] = RTC_Hour | MIL; //set MIL bit for 24h mode
  RTC_Sector_Write[2] = RTC_Minutes;
  RTC_Sector_Write[1] = RTC_Seconds;


  TI_USCI_I2C_transmitinit(SLAVE_ADDR,PRESCALER);
  while ( TI_USCI_I2C_notready() );
  if ( TI_USCI_I2C_slave_present(SLAVE_ADDR) )
  {

    TI_USCI_I2C_transmitinit(SLAVE_ADDR,PRESCALER);
    while ( TI_USCI_I2C_notready() );
    TI_USCI_I2C_transmit(2,RTC_Sector_Write_Enable);
    __delay_cycles(DELAY_I2C);

    TI_USCI_I2C_transmitinit(SLAVE_ADDR,PRESCALER);
    while ( TI_USCI_I2C_notready() );
    TI_USCI_I2C_transmit(4,RTC_Sector_Write);
    __delay_cycles(DELAY_I2C);

    while(1){
		//P1OUT    |= BIT0; //used for scope trigger

		TI_USCI_I2C_transmitinit(SLAVE_ADDR,PRESCALER);
		while ( TI_USCI_I2C_notready() );
		TI_USCI_I2C_transmit(1,RTC_Sector_Address);
		__delay_cycles(DELAY_I2C);

		TI_USCI_I2C_receiveinit(SLAVE_ADDR,PRESCALER);
		while ( TI_USCI_I2C_notready() );
		TI_USCI_I2C_receive(3,RTC_Sector_Read);
		__delay_cycles(DELAY_I2C);
		__delay_cycles(600);

		//P1OUT &= ~BIT0; //used for scope trigger

		RTC_Hour    = RTC_Sector_Read[2] & 0x7F; //throw away MIL bit
		RTC_Minutes = RTC_Sector_Read[1];
		RTC_Seconds = RTC_Sector_Read[0];

		hour = 12; //BCD to decimal
		min = 59; //BCD to decimal

		digit1 = hour / 10;
		digit2 = hour % 10;
		digit3 = min / 10;
		digit4 = min % 10;

		P1OUT = digit1;
		P2OUT |= BIT0;
		__delay_cycles(DELAY_MUX);
		P2OUT &= ~BIT0;

		__delay_cycles(DEAD_TIME);
		P1OUT = digit2;
		P2OUT |= BIT1;
		__delay_cycles(DELAY_MUX);
		P2OUT &= ~BIT1;

		__delay_cycles(DEAD_TIME);
		P1OUT = digit3;
		P2OUT |= BIT2;
		__delay_cycles(DELAY_MUX);
		P2OUT &= ~BIT2;

		__delay_cycles(DEAD_TIME);
		P1OUT = digit4;
		P2OUT |= BIT3;
		__delay_cycles(DELAY_MUX);
		P2OUT &= ~BIT3;

	}
  } else {
	 P1OUT    &= 0x00; //if LED is off then there was no response from SLAVE_ADDR
  }
}
