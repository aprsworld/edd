#include "edd.h"

#define SERIAL_PREFIX   'A'
#define SERIAL_NUMBER   4799
#define LIVE_SLOT_DELAY 95    // milliseconds*10. Value 10=100 milliseconds
                             // 150 bytes of data with MT=3 takes ~75 ms
                             // so we will do 100 millisecond slots
                             // R1036=0 (APRS World's), R1037=10, R1038=20, R1039=30, R1040=40, R1041=50
                             // R1042=60, R1043=70, R1044=80, R1045=90, R1046=0



typedef struct {
	/* passed directly through to GNSS module */
	int8 geofence_latitude[4];
	int8 geofence_longitude[4];
	int8 geofence_radius[4];
	int8 geofence_confidence;

	/* evaluated by comparing to GNSS UBX-NAV-PVT */
	int32 geofence_altitude;

	/* channel a configuration */
	int16 delay_geofence_a;
	int16 delay_altitude_a;
	int16 on_time_a;

	/* channel b configuration */
	int16 delay_geofence_b;
	int16 delay_altitude_b;
	int16 on_time_b;
} struct_config;

	

typedef struct {
	int16 input_voltage_adc;
	int16 sequence;
	int16 last_command_seconds;

	int8 flags_a;
	int16 countdown_a;
	
	int8 flags_b;
	int16 countdown_b;
	
	int8 ubx_buff_pvt[95];
	int8 ubx_buff_geofence[20];
	
//	int8 live_age;
	int8 live_countdown;

	int8 ubx_state;
} struct_current;


typedef struct {
	short now_gnss_trigger_done;
	short now_10millisecond;

} struct_action;


/* global structures */
struct_current current;
struct_action action;
struct_config config;


#include "edd_adc.c"
#include "edd_interrupts.c"
#include "edd_live.c"





void task_10millisecond(void) {
	if ( current.live_countdown > 0 && current.live_countdown != 0xff ) {
		current.live_countdown--;
	}
}


void init() {
//	int8 i;
	setup_oscillator(OSC_8MHZ | OSC_INTRC);
	setup_adc(ADC_CLOCK_INTERNAL);
	setup_adc_ports(sAN0 | VSS_VDD );
	setup_wdt(WDT_ON);

	/* 
	Manually set ANCON0 to 0xff and ANCON1 to 0x1f for all digital
	Otherwise set high bit of ANCON1 for VbGen enable, then remaining bits are AN12 ... AN8
	ANCON1 AN7 ... AN0
	set bit to make input digital
	*/
	/* AN7 AN6 AN5 AN4 AN3 AN2 AN1 AN0 */
	ANCON0 = 0b11111110;
	/* VbGen x x 12 11 10 9 8 */
	ANCON1 = 0b01111111;

	/* one periodic interrupt @ 100uS. Generated from system 8 MHz clock */
	/* prescale=4, match=49, postscale=1. Match is 49 because when match occurs, one cycle is lost */
	setup_timer_2(T2_DIV_BY_4,49,1); 



	/* not sure why we need this */
	delay_ms(14);


	action.now_gnss_trigger_done=0;
	action.now_10millisecond=0;

	current.ubx_state=UBX_STATE_INIT;

	current.sequence=0;

	current.live_countdown=0xff;
}

void ubx_config_message_rate(int8 ubx_cls, int8 ubx_id, int8 ubx_rate) {
	int8 ck_a=0;
	int8 ck_b=0;
	int8 i;
	int8 buff[32];

	buff[0] =0xB5; /* sync char 1 */
	buff[1] =0x62; /* sync char 2 */

	/* checksum calculation starts here */
	buff[2] =0x06; /* ubx message class */
	buff[3] =0x01; /* ubx message id */
	buff[4] =0x08; /* message length LSB */
	buff[5] =0x00; /* message length MSB */

	/* payload */
	buff[6] =ubx_cls; 
	buff[7] =ubx_id; 
	buff[8] =ubx_rate; /* rate on port 1 to 6 (port 6 always 0) */
	buff[9] =ubx_rate;
	buff[10] =ubx_rate;
	buff[11] =ubx_rate;
	buff[12] =ubx_rate;
	buff[13] =0;
	/* checksum calculation end here */

	/* calculate checksum */
	for ( i=2 ; i<=13 ; i++ ) {
		ck_a = ck_a + buff[i];
		ck_b = ck_b + ck_a;
	}

	buff[14]=ck_a;
	buff[15]=ck_b;


	for ( i=0 ; i<=15 ; i++ ) {
		fputc(buff[i],SERIAL_GNSS);
//		delay_us(100);
	}

	delay_ms(100);

}

void ubx_config_geofence(void) {
	/*
??:??:??  0000  B5 62 06 69 14 00 00 01 05 00 01 01 00 00 00 DE  µb.i...........Þ
          0010  39 1A 00 BD F8 C7 10 27 00 00 6F 84              9..½øÇ.'..o..
	*/
	int8 ck_a=0;
	int8 ck_b=0;
	int8 i;
	int8 buff[32];

	buff[0]= 0xB5; /* sync char 1 */
	buff[1]= 0x62; /* sync char 2 */

	/* checksum calculation starts here */
	buff[2]= 0x06; /* UBX message class */
	buff[3]= 0x69; /* UBX message ID */
	buff[4]= 0x14; /* message length LSB 8 + 12*numFences */
	buff[5]= 0x00; /* message length MSB */
	buff[6]= 0x00; /* version */
	buff[7]= 0x01; /* number of geofences contained in this message */
	buff[8]= 0x05; /* confidence level. 0=no confidence required, 1=68%, 2=95%, 3=99.7%, 4=99.99%, 5=99.999% */
	buff[9]= 0x00; /* reserved */
	buff[10]=0x01; /* 1=enable PIO combined fence state output, 0=disable */
	buff[11]=0x01; /* PIO pin polarity. 0=Low means inside, 1=low means outside. Unknown is alway high */
	buff[12]=0x0d; /* PIO pin number */
	buff[13]=0x00; /* reserved */
#if 0
	/* 44, -94 */
	buff[14]=0x00; /* latitude * 1e-7 */
	buff[15]=0xDE;
	buff[16]=0x39;
	buff[17]=0x1A;
	buff[18]=0x00; /* longitude * 1e-7 */
	buff[19]=0xBD;
	buff[20]=0xF8;
	buff[21]=0xC7;
#else
	/* 43.9878275, -91.8727010 */
	/* ??:??:??  0000  B5 62 06 69 14 00 00 01 05 00 01 01 0D 00 83 02  µb.i............
          0010  38 1A 9E 56 3D C9 10 27 00 00 A0 A7              8..V=É.'.. §.
     */
	buff[14]=0x83; /* latitude * 1e-7 */
	buff[15]=0x02;
	buff[16]=0x38;
	buff[17]=0x1A;
	buff[18]=0x9E; /* longitude * 1e-7 */
	buff[19]=0x56;
	buff[20]=0x3D;
	buff[21]=0xC9;
#endif

#if 0
	/* 100.00 meter = 1000 value */
	buff[22]=0x10; /* radius * 1e-2 */
	buff[23]=0x27;
	buff[24]=0x00;
	buff[25]=0x00;
#else
	/* 1 meter = 100 value */
	buff[22]=0x64; /* radius * 1e-2 */
	buff[23]=0x00;
	buff[24]=0x00;
	buff[25]=0x00;
#endif
	/* checksum calculation end here */

	/* calculate checksum */
	for ( i=2 ; i<=25 ; i++ ) {
		ck_a = ck_a + buff[i];
		ck_b = ck_b + ck_a;
	}

	buff[26]=ck_a; 
	buff[27]=ck_b;

	for ( i=0 ; i<=27 ; i++ ) {
		fputc(buff[i],SERIAL_GNSS);
	}

	delay_ms(100);
          
}


void main(void) {
	int8 i;
//	int16 l,m;

	output_low(CONTROL_A);
	output_low(CONTROL_B);

	init();

	output_low(LED_RED);
	output_low(LED_GREEN);

#if 0
	for ( i=0 ; i<10 ; i++ ) {
		restart_wdt();

		output_high(LED_GREEN);
		output_low(LED_RED);

		delay_ms(250);

		output_low(LED_GREEN);
		output_high(LED_RED);

		delay_ms(250);
	}

	output_low(LED_RED);
	output_low(LED_GREEN);
#endif


	delay_ms(100);
//	fprintf(SERIAL_XTC,"# EDD (%s) on XTC woohoo\r\n",__DATE__);



	/* once per second */
	ubx_config_message_rate(0x01,0x07,1); /* UBX-NAV-PVT */
	ubx_config_message_rate(0x01,0x39,1); /* UBX-NAV-GEOFENCE */

	/* disable */
	ubx_config_message_rate(0x01,0x03,0); /* UBX-NAV-STATUS */
	ubx_config_message_rate(0xF0,0x00,0); /* GGA */ /* if we use NMEA sentences, this is the one we want */
	ubx_config_message_rate(0xF0,0x01,0); /* GLL */
	ubx_config_message_rate(0xF0,0x02,0); /* GSA */
	ubx_config_message_rate(0xF0,0x03,0); /* GSV */
	ubx_config_message_rate(0xF0,0x04,0); /* RMC */
	ubx_config_message_rate(0xF0,0x41,0); /* TXT */
	ubx_config_message_rate(0xF0,0x05,0); /* VTG */


	ubx_config_geofence();

	/* start 100uS timer */
	enable_interrupts(INT_TIMER2);
	/* enable serial ports */
	enable_interrupts(INT_RDA);

	/* 1 second time pulse from GNSS */
	enable_interrupts(INT_EXT);

	/* turn on interrupts */
	enable_interrupts(GLOBAL);

	i=0;
	for ( ; ; ) {
		restart_wdt();

#if 0
		/* low output fron GNSS PIO means outside fence */
		output_bit(LED_RED,! input(GNSS_EXT_INT));
#endif

		/* as soon as interrupt finishes a trigger sentence we are ready to send our data */
		if ( action.now_gnss_trigger_done) { 
			action.now_gnss_trigger_done=0;

		}


		/* live countdown is set to slot delay when time pulse is received */
		if ( 0==current.live_countdown ) {
			live_send_edd();
			current.live_countdown=0xff;
		}

		/* periodic tasks */
		if  ( action.now_10millisecond ) {
			action.now_10millisecond=0;
			task_10millisecond();
		}
	}
}

