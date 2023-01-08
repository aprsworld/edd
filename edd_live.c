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

void live_send_edd(void) {
	int8 buff[80];
	int16 lCRC;
	int8 i;


	buff[0]='#';
	buff[1]=SERIAL_PREFIX;
	buff[2]=make8(SERIAL_NUMBER,1);
	buff[3]=make8(SERIAL_NUMBER,0);
	buff[4]=82; /* packet length */
	buff[5]=39; /* packet type */

	buff[6]=make8(current.sequence,1);
	buff[7]=make8(current.sequence,0);

	/* data from UBX-NAV-PVT */
	buff[8] =current.ubx_buff_pvt[0];  /* iTOW */
	buff[9] =current.ubx_buff_pvt[1];  /* iTOW */
	buff[10]=current.ubx_buff_pvt[2];  /* iTOW */
	buff[11]=current.ubx_buff_pvt[3];  /* iTOW */

	buff[12]=current.ubx_buff_pvt[4];  /* year */
	buff[13]=current.ubx_buff_pvt[5];  /* year */
	buff[14]=current.ubx_buff_pvt[6];  /* month */
	buff[15]=current.ubx_buff_pvt[7];  /* day */
	buff[16]=current.ubx_buff_pvt[8];  /* hour */
	buff[17]=current.ubx_buff_pvt[9];  /* minute */
	buff[18]=current.ubx_buff_pvt[10]; /* second */

	buff[19]=current.ubx_buff_pvt[20]; /* fixType */
	buff[20]=current.ubx_buff_pvt[21]; /* flags */

	buff[21]=current.ubx_buff_pvt[23]; /* numSV */
	buff[22]=current.ubx_buff_pvt[24]; /* longitude */
	buff[23]=current.ubx_buff_pvt[25]; /* longitude */
	buff[24]=current.ubx_buff_pvt[26]; /* longitude */
	buff[25]=current.ubx_buff_pvt[27]; /* longitude */
	buff[26]=current.ubx_buff_pvt[28]; /* latitude */
	buff[27]=current.ubx_buff_pvt[29]; /* latitude */
	buff[28]=current.ubx_buff_pvt[30]; /* latitude */
	buff[29]=current.ubx_buff_pvt[31]; /* latitude */

	buff[30]=current.ubx_buff_pvt[36]; /* height above mean sea level */
	buff[31]=current.ubx_buff_pvt[37]; /* height above mean sea level */
	buff[32]=current.ubx_buff_pvt[38]; /* height above mean sea level */
	buff[33]=current.ubx_buff_pvt[39]; /* height above mean sea level */

	/* data from UBX-NAV-GEOFENCE */
	buff[34]=current.ubx_buff_geofence[0];  /* iTOW */
	buff[35]=current.ubx_buff_geofence[1];  /* iTOW */
	buff[36]=current.ubx_buff_geofence[2];  /* iTOW */
	buff[37]=current.ubx_buff_geofence[3];  /* iTOW */

	buff[38]=current.ubx_buff_geofence[5];  /* status */
	buff[39]=current.ubx_buff_geofence[6];  /* numFences */
	buff[40]=current.ubx_buff_geofence[7];  /* combState */


	/* data from us */
	buff[41]=make8(current.input_voltage_adc,1); /* input voltage */
	buff[42]=make8(current.input_voltage_adc,0); /* input voltage */
	
	buff[43]=current.flags_a; /* EDD flags */
	buff[44]=current.flags_b; /* EDD flags */

	buff[45]=make8(current.last_command_seconds,1); /* seconds since last command */
	buff[46]=make8(current.last_command_seconds,0); /* seconds since last command */

	buff[47]=make8(current.countdown_a,1); /* output a timer */
	buff[48]=make8(current.countdown_a,0); /* output a timer */

	buff[49]=make8(current.countdown_b,1); /* output b timer */
	buff[50]=make8(current.countdown_b,0); /* output b timer */

	/* configuration from us */
	buff[51]=config.geofence_latitude[0];
	buff[52]=config.geofence_latitude[1];
	buff[53]=config.geofence_latitude[2];
	buff[54]=config.geofence_latitude[3];

	buff[55]=config.geofence_longitude[0];
	buff[56]=config.geofence_longitude[1];
	buff[57]=config.geofence_longitude[2];
	buff[58]=config.geofence_longitude[3];

	buff[59]=config.geofence_radius[0];
	buff[60]=config.geofence_radius[1];
	buff[61]=config.geofence_radius[2];
	buff[62]=config.geofence_radius[3];

	buff[63]=config.geofence_confidence;

	buff[64]=make8(config.geofence_altitude,3);
	buff[65]=make8(config.geofence_altitude,2);
	buff[66]=make8(config.geofence_altitude,1);
	buff[67]=make8(config.geofence_altitude,0);

	buff[68]=make8(config.delay_geofence_a,1);
	buff[69]=make8(config.delay_geofence_a,0);
	buff[70]=make8(config.delay_altitude_a,1);
	buff[71]=make8(config.delay_altitude_a,0);
	buff[72]=make8(config.on_time_a,1);
	buff[73]=make8(config.on_time_a,0);

	buff[74]=make8(config.delay_geofence_b,1);
	buff[75]=make8(config.delay_geofence_b,0);
	buff[76]=make8(config.delay_altitude_b,1);
	buff[77]=make8(config.delay_altitude_b,0);
	buff[78]=make8(config.on_time_b,1);
	buff[79]=make8(config.on_time_b,0);



	/* CRC of the first part of the packet */
	lCRC=crc_chk_seeded(0xFFFF, buff+1,79);


	/* send to wireless modem */
	for ( i=0 ; i<sizeof(buff) ; i++ ) {
		fputc(buff[i],SERIAL_XTC);
	}	

	fputc(make8(lCRC,1),SERIAL_XTC);
	fputc(make8(lCRC,0),SERIAL_XTC);

	current.sequence++;

}
