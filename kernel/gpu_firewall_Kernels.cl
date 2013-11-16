__kernel void packet_matching(__global uint * packet_buff, const int PACKET_BUFF_SIZE, const int base_ip ) {
	int i;
	int packet_buff_index = get_global_id(0);

	if( packet_buff_index < PACKET_BUFF_SIZE ) {
		for( i = 0 ; i < 0x4000 ; i++ ) {
			if( packet_buff[packet_buff_index] == base_ip + i ) {
				result_buff[packet_buff_index] = 1;
				break;
			}
		}
	}
	if( result_buff[packet_buff_index] != 1 )
		result_buff[packet_buff_index] = 0;
}