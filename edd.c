#include "edd.h"

#define SERIAL_PREFIX   'A'
#define SERIAL_NUMBER   4798
#define LIVE_SLOT_DELAY 95    // milliseconds*10. Value 10=100 milliseconds
                             // 150 bytes of data with MT=3 takes ~75 ms
                             // so we will do 100 millisecond slots
                             // R1036=0 (APRS World's), R1037=10, R1038=20, R1039=30, R1040=40, R1041=50
                             // R1042=60, R1043=70, R1044=80, R1045=90, R1046=0



const int8 NMEA0183_TRIGGER[] = { 'G', 'N', 'G', 'G', 'A' };



typedef struct {
	int16 input_voltage_adc;
	int16 sequence;

	int8 live_age;
	int8 live_countdown;

	int8 gnss_age;
	int8 gnss_buff[254];
	int8 gnss_length;
} struct_current;


typedef struct {
	short now_nmea_raw_received;
	short now_gnss_trigger_start;
	short now_gnss_trigger_done;
	short now_10millisecond;

} struct_action;


typedef struct {
	int8 buff[254];
	int8 pos;
} struct_nmea_raw;

/* global structures */
struct_current current;
struct_action action;
struct_nmea_raw nmea_raw;


#include "edd_adc.c"
#include "edd_interrupts.c"
#include "edd_live.c"


void task_10millisecond(void) {
	if ( current.live_countdown > 0 && current.live_countdown != 0xff ) {
		current.live_countdown--;
	}

	if ( current.gnss_age < 255 ) {
		current.gnss_age++;
	}

	/* age live data timeout */
	if ( current.live_age < 255 ) {
		current.live_age++;
	}

#if 0
	if ( current.strobed_pulse_count > 0 ) {
		output_high(LED_ANEMOMETER);
	} else {
		output_low(LED_ANEMOMETER);
	}

	if ( current.vertical_anemometer_adc > 782 || current.vertical_anemometer_adc < 770 ) {
		output_high(LED_VERTICAL_ANEMOMETER);
	} else {
		output_low(LED_VERTICAL_ANEMOMETER);
	}
#endif
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


//	action.now_strobe_counters=0;
	action.now_gnss_trigger_start=0;
	action.now_gnss_trigger_done=0;
	action.now_10millisecond=0;


	nmea_raw.buff[0]='\0';
	nmea_raw.pos=0;

	current.gnss_age=255;;
	current.gnss_buff[0]='\0';
	current.gnss_length=0;
	
	current.sequence=0;

	current.live_countdown=0xff;
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


	fprintf(SERIAL_XTC,"# EDD (%s) on XTC\r\n",__DATE__);

	/* start 100uS timer */
//	enable_interrupts(INT_TIMER2);
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
		if ( current.live_age >= 120 && 0xff == current.live_countdown ) {
			/* didn't get a triger sentence from GNSS for last 1.2 seconds. Send data anyhow */
			action.now_strobe_counters=1;    /* triggers strobe of data */
			action.now_gnss_trigger_done=1;  /* triggers send of data */
//			fprintf(SERIAL_XTC,"# timeout waiting for trigger\r\n");
		}
#endif

		/* recieving a GNSS trigger on our serial port causes counter values to be strobe. Once that
		is complete, we sample ADCs */	
		if ( action.now_gnss_trigger_start ) {
			action.now_gnss_trigger_start=0;

			sample_adc();

//			fprintf(SERIAL_XTC,"# got trigger sentence\r\n");
		}	

		/* as soon as interrupt finishes a trigger sentence we are ready to send our data */
		if ( action.now_gnss_trigger_done) { 
			action.now_gnss_trigger_done=0;
	
			output_toggle(LED_RED);
			/* start a countdown for our slot for transmit */
			current.live_countdown=LIVE_SLOT_DELAY;

		}


		if ( 0==current.live_countdown ) {
		
			live_send();
			current.live_age=0;
			current.live_countdown=0xff;
		}

		/* periodic tasks */
		if  ( action.now_10millisecond ) {
			action.now_10millisecond=0;
			task_10millisecond();
		}
	}
}

#if 0
			fprintf(SERIAL_XTC,"# finished receiving trigger sentence or timeout\r\n");
			fprintf(SERIAL_XTC,"# current.live_age=%u\r\n",current.live_age);
			fprintf(SERIAL_XTC,"# {input=%lu}\r\n",
				current.input_voltage_adc
			);
			fprintf(SERIAL_XTC,"# current {gnss_age=%u gnss='%s'}\r\n",
				current.gnss_age,
				current.gnss_buff
			);
#endif