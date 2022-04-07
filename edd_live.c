int16 crc_chk_seeded(int16 reg_crc, int8 *data, int8 length) {
	int8 j;
//	int16 reg_crc=0xFFFF;

	while ( length-- ) {
		reg_crc ^= *data++;

		for ( j=0 ; j<8 ; j++ ) {
			if ( reg_crc & 0x01 ) {
				reg_crc=(reg_crc>>1) ^ 0xA001;
			} else {
				reg_crc=reg_crc>>1;
			}
		}	
	}
	
	return reg_crc;
}

void live_send(void) {
	int8 buff[24];
	int16 packet_length;
	int16 lCRC;
	int8 i;


	packet_length=sizeof(buff) + current.gnss_length + 2;

	buff[0]='#';
	buff[1]=SERIAL_PREFIX;
	buff[2]=make8(SERIAL_NUMBER,1);
	buff[3]=make8(SERIAL_NUMBER,0);
	/* since packet length will exceed 254, we will use extended packet length in buff[6] and buff[7] */
	buff[4]=0xff; /* packet length */
	buff[5]=38; /* packet type */
	buff[6]=make8(packet_length,1);
	buff[7]=make8(packet_length,0);

	buff[8]=make8(current.sequence,1);
	buff[9]=make8(current.sequence,0);

	/* wind counter */
#if 0
	buff[10]=make8(current.strobed_pulse_period,1); /* wind speed */
	buff[11]=make8(current.strobed_pulse_period,0);
	buff[12]=make8(current.strobed_pulse_min_period,1); /* wind gust */
	buff[13]=make8(current.strobed_pulse_min_period,0); 
	buff[14]=make8(current.strobed_pulse_count,1); /* wind pulse count */
	buff[15]=make8(current.strobed_pulse_count,0); 
#endif

	/* analog */
	buff[16]=make8(current.input_voltage_adc,1);
	buff[17]=make8(current.input_voltage_adc,0);

#if 0
	buff[18]=make8(current.vertical_anemometer_adc,1);
	buff[19]=make8(current.vertical_anemometer_adc,0);
	buff[20]=make8(current.wind_vane_adc,1);
	buff[21]=make8(current.wind_vane_adc,0);
#endif

	/* meta data */
	buff[22]=current.live_age;

	/* NMEA data */
	buff[23]=current.gnss_age;

	/* CRC of the first part of the packet */
	lCRC=crc_chk_seeded(0xFFFF, buff+1, 23);

	/* CRC with the addition of the raw NMEA data */
	lcrc = crc_chk_seeded(lCRC, current.gnss_buff, current.gnss_length);




	/* send to wireless modem */
	for ( i=0 ; i<sizeof(buff) ; i++ ) {
		fputc(buff[i],SERIAL_XTC);
	}	

	for ( i=0 ; i<current.gnss_length ; i++ ) {
		fputc(current.gnss_buff[i],SERIAL_XTC);
	}

	fputc(make8(lCRC,1),SERIAL_XTC);
	fputc(make8(lCRC,0),SERIAL_XTC);

	current.sequence++;

}