

#int_timer2  HIGH
/* this timer only runs once booted */
void isr_100us(void) {
}




#int_rda2
void serial_isr_wireless(void) {
	int8 c;

	c=fgetc(SERIAL_XTC);
}

#define GNSS_STATE_WAITING 0
#define GNSS_STATE_IN      1

#int_rda
void serial_isr_gnss(void) {
	static int1 state=GNSS_STATE_WAITING; 
	static int8 buff[6];
	static int8 pos;
	int8 c;

	c=fgetc(SERIAL_GNSS);


	/* skip adding if ( carriage returns OR line feeds OR buffer full ) */
	if ( '\r' != c && '\n' != c && pos < (sizeof(nmea_raw.buff)-2) ) {
		/* add to our buffer */
		nmea_raw.buff[pos]=c;
		pos++;
		nmea_raw.buff[pos]='\0';
	}

	/* capturing data but looking for our trigger so we can set flag */
	if ( GNSS_STATE_WAITING == state ) {
		/* FIFO */
		buff[0]=buff[1];            // '$'
		buff[1]=buff[2];            // 'G'
		buff[2]=buff[3];            // 'P'
		buff[3]=buff[4];            // 'R' or 'H'
		buff[4]=buff[5];            // 'M' or 'D'
		buff[5]=c; 					// 'C' or 'T'
	
		if ( '$' == buff[0] && NMEA0183_TRIGGER[0] == buff[1] && 
	 	                       NMEA0183_TRIGGER[1] == buff[2] &&
		                       NMEA0183_TRIGGER[2] == buff[3] &&
		                       NMEA0183_TRIGGER[3] == buff[4] &&
	 	                       NMEA0183_TRIGGER[4] == buff[5] ) {
			/* matched our 5 character trigger sequence */
//			action.now_strobe_counters=1;
			current.gnss_age=0;
			state=GNSS_STATE_IN;

//			output_toggle(LED_RED);
		}
	} else {
		/* GNSS_STATE_IN */
		/* in our trigger sentence. Look for '\r' or '\n' that ends it */
		if ( '\r' == c || '\n' == c ) {
			for ( c=0 ; c<pos ; c++ ) {
				current.gnss_buff[c]=nmea_raw.buff[c];
			}
			current.gnss_buff[c]='\0';
			current.gnss_length=pos;
			
			action.now_gnss_trigger_done=1;
			state=GNSS_STATE_WAITING;
			pos=0;
		}
	}

}
