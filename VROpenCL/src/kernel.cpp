#include "kernel.h"
#include <iostream>
using namespace std;


#define opacityThreshold 0.99

OpenCLClass::OpenCLClass()
{


	pbo = 999999;


	pbo_cl = NULL;
	device_id = NULL;
	context = NULL;
	command_queue = NULL;
	memobj = NULL;
	program = NULL;
	kernel = NULL;
	platform_id = NULL;
	ret_num_devices;
	ret_num_platforms;
	ret;


	localSize[0] = 2;
	localSize[1] = 2;

	globalSize[0] = 10;
	globalSize[1] = 10;

	FILE *fp;
	char fileName[] = "./shaders/kernel.cl";
	char *source_str;
	size_t source_size;

	/* Load the source code containing the kernel*/
	fp = fopen(fileName, "r");
	if (!fp) {
		fprintf(stderr, "Failed to load kernel.\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);

	/* Get Platform and Device Info */
	ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

	/* Create OpenCL context */
	context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);

	/* Create Command Queue */
	command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

	/* Create Kernel Program from the source */
	program = clCreateProgramWithSource(context, 1, (const char **)&source_str,
		(const size_t *)&source_size, &ret);

	/* Build Kernel Program */
	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

	/* Create OpenCL Kernel */
	kernel = clCreateKernel(program, "volumeRendering", &ret);

	free(source_str);



	// init invViewMatrix
	d_invViewMatrix = clCreateBuffer(context, CL_MEM_READ_ONLY, 12 * sizeof(float), 0, &ciErrNum);
}

OpenCLClass::~OpenCLClass()
{
	/* Finalization */
	if (volumeSamplerLinear)clReleaseSampler(volumeSamplerLinear);
	if (transferFuncSampler)clReleaseSampler(transferFuncSampler);
	if (d_volumeArray)clReleaseMemObject(d_volumeArray);
	if (d_transferFuncArray)clReleaseMemObject(d_transferFuncArray);
	if (pbo_cl)clReleaseMemObject(pbo_cl);
	if (d_invViewMatrix)clReleaseMemObject(d_invViewMatrix);
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(memobj);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);

	if (pbo != 999999)
	glDeleteBuffers(1, &pbo);	
}


// Helper function for using CUDA to add vectors in parallel.
void OpenCLClass::openCLRC(/*, unsigned int width, unsigned int height, float h, float4 *d_buffer, float4 *d_lastHit*/)
{

	glFinish();
	// Acquire PBO for OpenCL writing
	ciErrNum = clEnqueueAcquireGLObjects(command_queue, 1, &pbo_cl, 0, 0, 0);


	/* Execute OpenCL Kernel */
	ciErrNum = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, globalSize, localSize, 0, 0, 0);


	/* Execute OpenCL Kernel */
	//ret = clEnqueueTask(command_queue, kernel, 0, NULL, NULL);

	/* Copy results from the memory buffer */
	/*ret = clEnqueueReadBuffer(command_queue, memobj, CL_TRUE, 0,
		MEM_SIZE * sizeof(char), string, 0, NULL, NULL);*/


	//finish 
	clFinish(command_queue);
	ciErrNum = clEnqueueReleaseGLObjects(command_queue, 1, &pbo_cl, 0, 0, 0);
}


void OpenCLClass::openCLSetVolume(cl_char *vol, unsigned int width, unsigned int height, unsigned int depth, float diagonal)
{
	// create 3D array and copy data to device
	cl_image_format volume_format;
	volume_format.image_channel_order = CL_RGBA;
	volume_format.image_channel_data_type = CL_UNORM_INT8;
	cl_uchar* h_tempVolume = (cl_uchar*)malloc(width * height * depth * 4);
	for (int i = 0; i <(int)(width * height * depth); i++)
	{
		h_tempVolume[4 * i] = vol[i];
	}
	d_volumeArray = clCreateImage3D(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, &volume_format,
		width, height, depth,
		(width * 4), (width * height * 4),
		h_tempVolume, &ciErrNum);
	free(h_tempVolume);

	// set image and sampler args
	ciErrNum = clSetKernelArg(kernel, 7, sizeof(cl_mem), (void *)&d_volumeArray);
	ciErrNum = clSetKernelArg(kernel, 9, sizeof(cl_sampler), &volumeSamplerLinear);

	//Set the step
	float step = 1.f / diagonal;
	ciErrNum = clSetKernelArg(kernel, 1, sizeof(unsigned int), &diagonal);
}


void OpenCLClass::openCLSetImageSize(unsigned int width, unsigned int height, float NCP, float angle){
	globalSize[0] = width;
	globalSize[1] = height;
	Width = width;
	Height = height;



	/*if (pbo != 999999) {
		// delete old buffer
		clReleaseMemObject(pbo_cl);
	}*/

	// create pixel buffer object for display
	/*if (pbo == 999999) glGenBuffers(1, &pbo);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * sizeof(GLubyte) * 4, 0, GL_STREAM_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);*/



	// create OpenCL buffer from GL PBO
	//pbo_cl = clCreateFromGLBuffer(context, CL_MEM_WRITE_ONLY, pbo, &ciErrNum);
	pbo_cl = clCreateFromGLTexture(context, CL_MEM_WRITE_ONLY,
		GL_TEXTURE_2D, 0, TextureManager::Inst()->GetID(TEXTURE_FINAL_IMAGE), NULL);

	

	ciErrNum = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&pbo_cl);
	ciErrNum = clSetKernelArg(kernel, 4, sizeof(unsigned int), &width);
	ciErrNum = clSetKernelArg(kernel, 5, sizeof(unsigned int), &height);

	ciErrNum = clSetKernelArg(kernel, 2, sizeof(unsigned int), &angle);
	ciErrNum = clSetKernelArg(kernel, 3, sizeof(unsigned int), &NCP);
	
}


void OpenCLClass::openCLUpdateMatrix(const float * matrix){
	ciErrNum = clEnqueueWriteBuffer(command_queue, d_invViewMatrix, CL_FALSE, 0, 16 * sizeof(float), matrix, 0, 0, 0);
	ciErrNum = clSetKernelArg(kernel, 6, sizeof(cl_mem), (void *)&d_invViewMatrix);
}


void OpenCLClass::openCLSetTransferFunction(cl_float4 *transferFunction, unsigned int width){
	cl_image_format transferFunc_format;
	transferFunc_format.image_channel_order = CL_RGBA;
	transferFunc_format.image_channel_data_type = CL_FLOAT;
	d_transferFuncArray = clCreateImage2D(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		&transferFunc_format, 9, 1, sizeof(float) * 9 * 4, transferFunction, &ciErrNum);

	// Create samplers for transfer function, linear interpolation and nearest interpolation 
	transferFuncSampler = clCreateSampler(context, true, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_LINEAR, &ciErrNum);
	volumeSamplerLinear = clCreateSampler(context, true, CL_ADDRESS_REPEAT, CL_FILTER_LINEAR, &ciErrNum);
	
	// set image and sampler args
	ciErrNum = clSetKernelArg(kernel, 8, sizeof(cl_mem), (void *)&d_transferFuncArray);
	ciErrNum = clSetKernelArg(kernel, 10, sizeof(cl_sampler), &transferFuncSampler);
}



void OpenCLClass::Use(GLenum activeTexture){
	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// copy from pbo to texture
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
	glActiveTexture(activeTexture);
	TextureManager::Inst()->BindTexture(TEXTURE_FINAL_IMAGE);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}