__kernel void volumeRendering(	__write_only image2d_t d_output,
								float constantH, 
								float constantAngle, 
								float cosntantNCP,
								unsigned int constantWidth, 
								unsigned int constantHeight,
								__constant float * c_invViewMatrix, 
								__read_only image3d_t volume,
								__read_only image2d_t transferFunc,
								sampler_t volumeSampler,
								sampler_t transferFuncSampler)
{

	uint gx = get_global_id(0);
	uint gy = get_global_id(1);

	uint grx = get_group_id(0);
	uint gry = get_group_id(1);

	uint lx = get_local_id(0);
	uint ly = get_local_id(1);

	if (gx < constantWidth && gy < constantHeight){
		float4 color = read_imagef(volume, volumeSampler, (float4)(gx / (float)constantWidth, gy / (float)constantHeight, 0.5f, 0.0f));

		//write_imagef(d_output, (int2)(gx, gy), color);
		write_imagef(d_output, (int2)(gx, gy), (float4)(color));
	}
}


/*uint gx = get_global_id(0);
uint gy = get_global_id(1);

uint grx = get_group_id(0);
uint gry = get_group_id(1);

uint lx = get_local_id(0);
uint ly = get_local_id(1);

uint width = get_global_size(0);
uint height = get_global_size(1);
//uint y = get_global_id(1);



if (gx % (10-1) == 0 && gx != 0) string[gy * width + gx] = '\n';
else  string[gy * width + gx] = '0' + gx;*/