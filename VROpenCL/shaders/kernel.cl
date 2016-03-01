__kernel void volumeRendering(/*__write_only*/__global cl_uint4 * d_output,
								__constant float constantH, 
								__constant float constantAngle, 
								__constant float cosntantNCP,
								__constant unsigned int constantWidth, 
								__constant unsigned int constantHeight,
								__constant float * c_invViewMatrix, 
								__read_only image3d_t volume,
								__read_only image2d_t transferFunc,
								sampler_t volumeSampler,
								sampler_t transferFuncSampler

)
{

	uint gx = get_global_id(0);
	uint gy = get_global_id(1);

	uint grx = get_group_id(0);
	uint gry = get_group_id(1);

	uint lx = get_local_id(0);
	uint ly = get_local_id(1);

	uint width = get_global_size(0);
	uint height = get_global_size(1);

	if (gx < width && gy < height){

		cl_uint4 color;
		color.x = 255;
		color.y = 0;
		color.z = 0;
		color.w = 255;
		d_output = color;
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



	if (gx % (10 - 1) == 0 && gx != 0) string[gy * width + gx] = '\n';
	else  string[gy * width + gx] = '0' + gx;*/

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