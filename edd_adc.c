
#define MEAN_FILTER_WIDTH    16


//-------------------------------------------------------------
// This function calculates the Mean (average).

int16 mean_filter(int16 latest_element) {
	static int16 input_buffer[MEAN_FILTER_WIDTH];
	static char inbuf_index = 0;
	static char num_elements = 0;
	int32 mean;
	int32 sum;
	char i;

	// Insert incoming data element into circular input buffer.
	input_buffer[inbuf_index] = latest_element;
	inbuf_index++;
	if(inbuf_index >= MEAN_FILTER_WIDTH)  // If index went past buffer end
	   inbuf_index = 0;       // then reset it to start of buffer

	if(num_elements < MEAN_FILTER_WIDTH)
	   num_elements++;

	// Calculate the mean.  This is done by summing up the
	// values and dividing by the number of elements.
	sum = 0;
	for(i = 0; i < num_elements; i++)
		sum += input_buffer[i];

	// Round-off the result by adding half the divisor to
	// the numerator.
	mean = (sum + (int32)(num_elements >> 1)) / num_elements;

	return((int16)mean);
}


void sample_adc(void) {
	int j;
	int16 value;

	/* battery voltage */
	set_adc_channel(0);	
	delay_ms(1);

	/* sample 16 times and mean filter */
	for ( j=0 ; j<16 ; j++ ) {
		value = mean_filter(read_adc());
	}
	current.input_voltage_adc=value;


}



