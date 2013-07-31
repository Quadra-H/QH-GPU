__kernel void hello_kernel(__global const float *a,
						__global const float *b,
						__global int *result)
{
    int gid = get_global_id(0);

    result[gid] = (int)(a[gid] + b[gid]);
}
