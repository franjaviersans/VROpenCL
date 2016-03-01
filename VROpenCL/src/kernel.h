

#ifndef OPENCL_H
#define OPENCL_H


#include <stdio.h>
#include <stdlib.h>


#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#if defined (__APPLE__) || defined(MACOSX)
	#define GL_SHARING_EXTENSION "cl_APPLE_gl_sharing"
#else
	#define GL_SHARING_EXTENSION "cl_khr_gl_sharing"
#endif


#include "../include/GL/glew.h"
#include "Definitions.h"
#include "TextureManager.h"
#include "helper_math.h"

#define MEM_SIZE (128)
#define MAX_SOURCE_SIZE (0x100000)

class OpenCLClass
{
public:
	OpenCLClass();
	~OpenCLClass();
	int Width, Height;
	cl_float4 *d_lastHit, *d_FirstHit;
	//cudaArray *d_volume, *d_texture;
	unsigned int num_vert, num_tri;
	GLuint pbo;     // OpenGL pixel buffer object
	//struct cudaGraphicsResource *cuda_pbo_resource; // CUDA Graphics Resource (to transfer PBO)

	void openCLUpdateMatrix(const float * matrix);
	void openCLRC(/*, unsigned int, unsigned int, float, float4 *, float4 **/);
	void openCLSetVolume(cl_char *vol, unsigned int width, unsigned int height, unsigned int depth, float diagonal);
	void openCLSetImageSize(unsigned int width, unsigned int height, float NCP, float angle);
	void openCLSetTransferFunction(cl_float4 *d_transferFunction, unsigned int width = 256);
	void Use(GLenum activeTexture);


private:
	cl_device_id device_id;
	cl_context context;
	cl_command_queue command_queue;
	cl_mem memobj;
	cl_program program;
	cl_kernel kernel;
	cl_platform_id platform_id;
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	cl_int ret;
	size_t globalSize[2];
	size_t localSize[2];
	cl_mem d_invViewMatrix;
	cl_int ciErrNum;
	cl_mem pbo_cl;
	cl_mem d_volumeArray;
	cl_mem d_transferFuncArray;
	cl_sampler volumeSamplerLinear;
	cl_sampler transferFuncSampler;
};


#endif