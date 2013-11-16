__kernel void packet_matching(__global uint * packet_buff, __global uint * result_buff, const int PACKET_BUFF_SIZE, const int base_ip ) {
	int i;
	int packet_buff_index = get_global_id(0);

	for( i = 0 ; i < 0x4000 ; i++ ) {
		if( packet_buff[packet_buff_index] == base_ip + i ) {
			result_buff[packet_buff_index] = 0;
			return ;
		}
	}
	
	result_buff[packet_buff_index] = 1;
	return ;
}
