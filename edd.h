#include <18F24J11.h>
#device ADC=10
#device *=16
//#device HIGH_INTS=TRUE
#fuses INTRC_IO, WDT8192, NOIESO, NOFCMEN, NODEBUG, NOSTVREN, T1DIG


#include <stdlib.h>
#use delay(clock=8000000, restart_wdt)

/* Vector GNSS */
#use rs232(UART1,stream=SERIAL_GNSS,baud=9600,xmit=PIN_C6,rcv=PIN_C7,ERRORS)	
// for PIC18F24J11
//#byte TXSTA=0xfad
//#bit  TRMT=TXSTA.1
#byte ANCON0=GETENV("SFR:ancon0")
#byte ANCON1=GETENV("SFR:ancon1")

/* UART2 - XTC */
#pin_select U2TX=PIN_C0
#pin_select U2RX=PIN_C1
#use rs232(UART2,stream=SERIAL_XTC, baud=57600,ERRORS)	

/* 
leave last 1K of code space alone for param storage 
*/
#define PARAM_CRC_ADDRESS  0x3800
#define PARAM_ADDRESS      PARAM_CRC_ADDRESS+2
#org PARAM_CRC_ADDRESS,PARAM_CRC_ADDRESS+1023 {}

#use standard_io(A)
#use standard_io(B)
#use standard_io(C)

#define LED_RED             PIN_C5
#define LED_GREEN           PIN_C4
#define ANEMOMETER_FILTERED PIN_B0

#define GNSS_TIMEPULSE      PIN_B0
#define GNSS_EXT_INT        PIN_B1

#define GNSS_PORT_A_TX      PIN_C7
#define GNSS_PORT_A_RX      PIN_C6
#define FROM_MODEM_DATA     PIN_C1
#define TO_MODEM_DATA       PIN_C0

#define CONTROL_A           PIN_C3
#define CONTROL_B           PIN_C2

/* analog inputs */
#define AN_IN_VOLTS        PIN_A0
#define ADC_AN_IN_VOLTS    0
