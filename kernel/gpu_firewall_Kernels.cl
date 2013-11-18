__kernel void packet_matching(__global uint * packet_buff, const int PACKET_BUFF_SIZE, const int base_ip ) {
	int i;
	int packet_buff_index = get_global_id(0);
	
	printf("packet:  %u \n" ,packet_buff[packet_buff_index]);

	if( packet_buff_index < PACKET_BUFF_SIZE ) {
		for( i = 0 ; i < 0x4000 ; i++ ) {
		
			if(packet_buff[packet_buff_index] == 36203632 || packet_buff[packet_buff_index] == 1886136322) {
				packet_buff[packet_buff_index] = 1;
				break;
			}
		
			if( packet_buff[packet_buff_index] == base_ip + i ) {
				packet_buff[packet_buff_index] = 1;
				break;
			}
		}
	}
	if( packet_buff[packet_buff_index] != 1 )
		packet_buff[packet_buff_index] = 0;
}