

#int_timer2  /* HIGH used if we need high acuracy. Also need #device HIGH_INTS=TRUE */
/* this timer only runs once booted */
void isr_100us(void) {
	static int8 tick=0;

	/* every 100 cycles we tell main() loop to do 10 milisecond activities */
	tick++;
	if ( 100 == tick ) {
		tick=0;
		action.now_10millisecond=1;
	}
	

}


#int_ext
void isr_timepulse() {
	output_toggle(LED_GREEN);
	/* clear ubx buffers */

	/* start a countdown for our slot for transmit */
	current.live_countdown=LIVE_SLOT_DELAY;

	current.ubx_state=UBX_STATE_INIT;

}

#int_rda2
void serial_isr_wireless(void) {
	int8 c;

	c=fgetc(SERIAL_XTC);
}



#int_rda
void serial_isr_gnss(void) {
	static int8 buff[6];
	static int16 ubx_payload_length=0;
	static int8 pos;
	int8 c;

	c=fgetc(SERIAL_GNSS);

	if ( UBX_STATE_INIT == current.ubx_state ) {
		buff[0]=buff[1]=buff[2]=buff[3]=buff[4]=buff[5]=0;
		ubx_payload_length=0;
		pos=0;
		
		current.ubx_state=UBX_STATE_SCANNING;
	} 


	if ( UBX_STATE_SCANNING == current.ubx_state ) {
		output_low(LED_RED);

		/* FIFO */
		buff[0]=buff[1];            // 0xB5
		buff[1]=buff[2];            // 0x62
		buff[2]=buff[3];            // UBX class
		buff[3]=buff[4];            // UBX ID
		buff[4]=buff[5];            // UBX length LSB
		buff[5]=c;                  // UBX length MSB
	
		if ( 0xB5 == buff[0] && 0x62 == buff[1] ) {
			/* got UBX message */
			
			/* extract the length */
			ubx_payload_length=make16(buff[5],buff[4]);

			pos=0;

			if ( 0x01 == buff[2] && 0x07 == buff[3] ) {
				current.ubx_state=UBX_STATE_IN_PVT;
			} else if ( 0x01 == buff[2] && 0x39 == buff[3] ) {
				current.ubx_state=UBX_STATE_IN_GEOFENCE;
			} else {
				current.ubx_state=UBX_STATE_IN_DISCARD;
			}
		}

	} else if ( UBX_STATE_IN_PVT == current.ubx_state ) {
		output_high(LED_RED);

		if ( pos < ubx_payload_length && pos < (sizeof(current.ubx_buff_pvt)-1) ) {
			/* add to our buffer */
			current.ubx_buff_pvt[pos]=c;
			pos++;
		} else {
			current.ubx_state=UBX_STATE_SCANNING;
		}
	} else if ( UBX_STATE_IN_GEOFENCE == current.ubx_state ) {
		output_high(LED_RED);

		if ( pos < ubx_payload_length && pos < (sizeof(current.ubx_buff_geofence)-1) ) {
			/* add to our buffer */
			current.ubx_buff_geofence[pos]=c;
			pos++;
		} else {
			current.ubx_state=UBX_STATE_SCANNING;
		}
	} else if ( UBX_STATE_IN_DISCARD == current.ubx_state ) {
		output_high(LED_RED);

		if ( pos < ubx_payload_length ) {
			pos++;
		} else {
			current.ubx_state=UBX_STATE_SCANNING;
		}
	}
}
