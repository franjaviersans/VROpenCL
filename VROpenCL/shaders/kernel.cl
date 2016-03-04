#define opacityThreshold 0.99


struct Ray
{
	float4 o;   // origin
	float4 d;   // direction
};

struct float4x4
{
	float4 m[4];
};

// intersect ray with a box
// http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter3.htm


// transform vector by matrix with translation
float4 multiplication(struct float4x4 M, float4 v)
{
	float4 r;
	r.x = dot(v, M.m[0]);
	r.y = dot(v, M.m[1]);
	r.z = dot(v, M.m[2]);
	r.w = dot(v, M.m[3]);
	return r;
}


int intersectBox(struct Ray r, float *tnear, float *tfar)
{
	float3 boxmin = (float3)(-.5f, -.5f, -.5f);
	float3 boxmax = (float3)(.5f, .5f, .5f);
	/*float3 boxmin = make_float3(-1.f, -1.f, -1.f);
	float3 boxmax = make_float3(1.f, 1.f, 1.f);*/
	// compute intersection of ray with all six bbox planes
	float3 invR = (float3)(1.0f / r.d.x, 1.0f / r.d.y, 1.0f /r.d.z);
	float3 tbot = invR * (float3)(boxmin.x - r.o.x, boxmin.y - r.o.y, boxmin.z - r.o.z);
	float3 ttop = invR * (float3)(boxmax.x - r.o.x, boxmax.y - r.o.y, boxmax.z - r.o.z);

	// re-order intersections to find smallest and largest on each axis

	float3 tmin = (float3)(FLT_MAX);
	float3 tmax = (float3)(0.0f);

	tmin = fmin(tmin, fmin(ttop, tbot));
	tmax = fmax(tmax, fmax(ttop, tbot));

	// find the largest tmin and the smallest tmax
	float largest_tmin = fmax(fmax(tmin.x, tmin.y), fmax(tmin.x, tmin.z));
	float smallest_tmax = fmin(fmin(tmax.x, tmax.y), fmin(tmax.x, tmax.z));

	*tnear = largest_tmin;
	*tfar = smallest_tmax;

	return smallest_tmax > largest_tmin;
}




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

	int2 global_id = (int2)(get_global_id(0), get_global_id(1));

	if (global_id.x < constantWidth && global_id.y < constantHeight){
		/*float sample = tex3D(volume, float(x) / constantWidth, float(y) / constantHeight, 0.5f);
		float4 color = tex1D(transferFunction, sample);

		result[tpos] = make_uchar4(color.x * 255, color.y * 255, color.z * 255, 255);*/
		//result[tpos] = make_uchar4(color.x * 255, color.y * 255, color.z * 255, 255);
		float2 Pos = (float2)(global_id.x, global_id.y);

		//Flip the Y axis
		//Pos.y = constantHeight - Pos.y;


		//ok u and v between -1 and 1
		float u = (((global_id.x + 0.5f) / (float)constantWidth)*2.0f - 1.0f);
		float v = (((global_id.y + 0.5f) / (float)constantHeight)*2.0f - 1.0f);



		// calculate eye ray in world space
		struct Ray eyeRay;
		float tangent = tan(constantAngle); // angle in radians
		float ar = (constantWidth / (float)constantHeight);
		struct float4x4 mat;
		mat.m[0] = (float4)(c_invViewMatrix[0], c_invViewMatrix[1], c_invViewMatrix[2], c_invViewMatrix[3]);
		mat.m[1] = (float4)(c_invViewMatrix[4], c_invViewMatrix[5], c_invViewMatrix[6], c_invViewMatrix[7]);
		mat.m[2] = (float4)(c_invViewMatrix[8], c_invViewMatrix[9], c_invViewMatrix[10], c_invViewMatrix[11]);
		mat.m[3] = (float4)(c_invViewMatrix[12], c_invViewMatrix[13], c_invViewMatrix[14], c_invViewMatrix[15]);
		eyeRay.o = multiplication(mat, (float4)(0.0f, 0.0f, 0.0f, 1.0f));
		eyeRay.d = normalize((float4)(u * tangent * ar, v * tangent, -1.0f, 0.0f));
		eyeRay.d = multiplication(mat, eyeRay.d);
		eyeRay.d = normalize(eyeRay.d);

		// find intersection with box
		float tnear, tfar;
		int hit = intersectBox(eyeRay, &tnear, &tfar);  // this must be wrong....anything else seems to be ok now

		float4 color, bg = (float4)(0.15f); //bg color here

		if (hit){

			/*if (tnear < ncp)
			tnear = ncp;     // clamp to near plane*/


			//float3 last = firsHit[tpos];
			//float3 first = lastHit[tpos];
			float4 f4 = eyeRay.o + eyeRay.d*tnear;
			float3 first = (float3)(f4.x, f4.y, f4.z);
			float4 l4 = eyeRay.o + eyeRay.d*tfar;
			float3 last = (float3)(l4.x, l4.y, l4.z);

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
				float4 scalar = read_imagef(volume, volumeSampler, 
					(float4)(trans.x + 0.5f, trans.y + 0.5f, 1.0f - (trans.z + 0.5f), 0.0f)); //convert to texture space
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
		}else{
			bg.w = 1.0f;
			write_imagef(d_output, global_id, bg);
			
		}
		

		

		/*float4 color = read_imagef(volume, volumeSampler, (float4)(gx / (float)constantWidth, gy / (float)constantHeight, 0.5f, 0.0f));

		//write_imagef(d_output, (int2)(gx, gy), color);
		write_imagef(d_output, (int2)(gx, gy), (float4)(color));*/
	}
}