__kernel void packet_matching(__global uint * packet_buff, const int PACKET_BUFF_SIZE, const int base_ip ) {
	int i;
	int packet_buff_index_x = get_global_id(0);
	int packet_buff_index_y = get_global_id(1);

	int packet_buff_index=packet_buff_index_y*16+packet_buff_index_x;

	if( packet_buff_index < PACKET_BUFF_SIZE ) {
		for( i = 0 ; i < 0x4000 ; i++ ) {
			if( packet_buff[packet_buff_index] == base_ip + i ) {
				packet_buff[packet_buff_index] = 1;
				break;
			}
		}
		if( packet_buff[packet_buff_index] != 1 )
			packet_buff[packet_buff_index] = 0;
	}
}