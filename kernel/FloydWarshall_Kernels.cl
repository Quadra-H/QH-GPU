__kernel void floydWarshallPass(__global uint * adj_mat, const int MAT_SIZE, const int k) {
	int j = get_global_id(0);
	int i = get_global_id(1);

	int tempWeight = adj_mat[i * MAT_SIZE + k] + adj_mat[k * MAT_SIZE + j];

	if( adj_mat[i * MAT_SIZE + j] > tempWeight )
		adj_mat[i * MAT_SIZE + j] = tempWeight;
}
