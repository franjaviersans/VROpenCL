#define opacityThreshold 0.99

__kernel void volumeRendering(__write_only image2d_t d_output,						/*0*/
								float constantH,									/*1*/
								unsigned int constantWidth, 						/*2*/
								unsigned int constantHeight, 						/*3*/
								__read_only image3d_t volume, 						/*4*/
								__read_only image2d_t transferFunc, 				/*5*/
								sampler_t volumeSampler, 							/*6*/
								sampler_t transferFuncSampler, 						/*7*/
								__read_only image2d_t firsttexture, 				/*8*/
								__read_only image2d_t lasttexture,					/*9*/
								sampler_t hitSampler) 								/*10*/
{

	int2 global_id = (int2)(get_global_id(0), get_global_id(1));

	if (global_id.x < constantWidth && global_id.y < constantHeight){
		float2 Pos = (float2)(global_id.x / (float)constantWidth, global_id.y / (float)constantHeight);


		float4 color; //bg color here

		float4 aux4 = read_imagef(firsttexture, hitSampler, Pos);
		float3 first = (float3)(aux4.x, aux4.y, aux4.z);
		aux4 = read_imagef(lasttexture, hitSampler, Pos);
		float3 last = (float3)(aux4.x, aux4.y, aux4.z);

		//Get direction of the ray
		float3 direction = last - first;
		float D = length(direction);
		direction = normalize(direction);

		color = (float4)(0.0f);
		color.w = 1.0f;

		float3 trans = first;
		float3 rayStep = direction * constantH;

		for (float t = 0; t <= D; t += constantH){

			//Sample in the scalar field and the transfer function
			float4 scalar = read_imagef(volume, volumeSampler, (float4)(trans.x, trans.y, trans.z, 0.0f)); //convert to texture space
			//float scalar = tex3D(volume, trans.x, trans.y, trans.z);
			float4 samp = read_imagef(transferFunc, transferFuncSampler, (float2)(scalar.x, 0.5f));
			//float scalar = 0.1;
			//float4 samp = make_float4(0.0f);

			//Calculating alpa
			samp.w = 1.0f - exp(-0.5 * samp.w);

			//Acumulating color and alpha using under operator 
			samp.x = samp.x * samp.w;
			samp.y = samp.y * samp.w;
			samp.z = samp.z * samp.w;

			color.x += samp.x * color.w;
			color.y += samp.y * color.w;
			color.z += samp.z * color.w;
			color.w *= 1.0f - samp.w;

			//Do early termination of the ray
			if (1.0f - color.w > opacityThreshold) break;

			//Increment ray step
			trans += rayStep;
		}

		color.w = 1.0f - color.w;

		write_imagef(d_output, global_id, color);
	}
}