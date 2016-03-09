#include "kernel.h"
#include <iostream>
#include <vector>
using namespace std;


#define opacityThreshold 0.99

OpenCLClass::OpenCLClass(GLFWwindow* glfwWindow, glm::ivec2 dim)
{

	cl_uint ret_num_platforms;

	pbo_cl = NULL;
	context = NULL;
	command_queue = NULL;
	program = NULL;
	kernel = NULL;
	platform_id = NULL;
	d_volumeArray = NULL;
	d_transferFuncArray = NULL;
	volumeSamplerLinear = NULL;
	transferFuncSampler = NULL;


	localSize[0] = dim.x;
	localSize[1] = dim.y;

	globalSize[0] = 10;
	globalSize[1] = 10;

	FILE *fp;
#ifndef NOT_RAY_BOX
	char fileName[] = "./shaders/kernel.cl";
#else
	char fileName[] = "./shaders/kernelImage.cl";
#endif
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
	ciErrNum = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	oclCheckError(ciErrNum, CL_SUCCESS);
	//ciErrNum = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &ret_num_devices);
	oclCheckError(ciErrNum, CL_SUCCESS);


	// Get string containing supported device extensions
	/*int ext_size = 1024;
	char* ext_string = (char*)malloc(ext_size);
	ciErrNum = clGetDeviceInfo(device_id, CL_DEVICE_EXTENSIONS, ext_size, ext_string, &ext_size);*/

	cl_context_properties context_properties[] =
	{
		CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
		CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
		CL_CONTEXT_PLATFORM, (cl_context_properties)platform_id,
		NULL
	};
	

	clGetGLContextInfoKHR_fn clGetGLContextInfo = NULL;

	clGetGLContextInfo = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddress("clGetGLContextInfoKHR");

	size_t bytes = 0;


	// queuing how much bytes we need to read
	ciErrNum |= clGetGLContextInfo(context_properties, CL_DEVICES_FOR_GL_CONTEXT_KHR, 0, NULL, &bytes);
	oclCheckError(ciErrNum, CL_SUCCESS);
	// allocating the mem
	size_t devNum = bytes / sizeof(cl_device_id);
	std::vector<cl_device_id> devs(devNum);
	//reading the info
	ciErrNum |= clGetGLContextInfo(context_properties, CL_DEVICES_FOR_GL_CONTEXT_KHR, bytes, &devs[0], NULL);
	oclCheckError(ciErrNum, CL_SUCCESS);
	
	unsigned int uiDeviceUsed = -1; 
	bool bSharingSupported = false;
	for (unsigned int i = 0; (!bSharingSupported && (i <= devs.size())); ++i)
	{
		size_t extensionSize;
		ciErrNum |= clGetDeviceInfo(devs[i], CL_DEVICE_EXTENSIONS, 0, NULL, &extensionSize);
		oclCheckError(ciErrNum, CL_SUCCESS);
		if (extensionSize > 0)
		{
			char* extensions = (char*)malloc(extensionSize);
			ciErrNum = clGetDeviceInfo(devs[i], CL_DEVICE_EXTENSIONS, extensionSize, extensions, &extensionSize);
			oclCheckError(ciErrNum, CL_SUCCESS);
			std::string stdDevString(extensions);
			free(extensions);

			size_t szOldPos = 0;
			size_t szSpacePos = stdDevString.find(' ', szOldPos); // extensions string is space delimited
			while (szSpacePos != stdDevString.npos)
			{
				if (strcmp(GL_SHARING_EXTENSION, stdDevString.substr(szOldPos, szSpacePos - szOldPos).c_str()) == 0)
				{
					// Device supports context sharing with OpenGL
					uiDeviceUsed = i;
					bSharingSupported = true;
					break;
				}
				do
				{
					szOldPos = szSpacePos + 1;
					szSpacePos = stdDevString.find(' ', szOldPos);
				} while (szSpacePos == szOldPos);
			}
		}
	}
	

	/* Create OpenCL context */
	context = clCreateContext(context_properties, 1, &devs[uiDeviceUsed], NULL, NULL, &ciErrNum);
	oclCheckError(ciErrNum, CL_SUCCESS);

	/* Create Command Queue */
	command_queue = clCreateCommandQueue(context, devs[uiDeviceUsed], 0, &ciErrNum);
	oclCheckError(ciErrNum, CL_SUCCESS);

	/* Create Kernel Program from the source */
	program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ciErrNum);
	oclCheckError(ciErrNum, CL_SUCCESS);

	/* Build Kernel Program */
	ciErrNum = clBuildProgram(program, 1, &devs[uiDeviceUsed], NULL, NULL, NULL);


	//Check errors in opencl kernel
	if (ciErrNum == CL_BUILD_PROGRAM_FAILURE) {
		// Determine the size of the log
		size_t log_size;
		clGetProgramBuildInfo(program, devs[uiDeviceUsed], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

		// Allocate memory for the log
		char *log = (char *)malloc(log_size);

		// Get the log
		clGetProgramBuildInfo(program, devs[uiDeviceUsed], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

		// Print the log
		printf("%s\n", log);
	}


	oclCheckError(ciErrNum, CL_SUCCESS);

	/* Create OpenCL Kernel */
	kernel = clCreateKernel(program, "volumeRendering", &ciErrNum);
	oclCheckError(ciErrNum, CL_SUCCESS);

	free(source_str);



	d_invViewMatrix = clCreateBuffer(context, CL_MEM_READ_ONLY, 16 * sizeof(float), 0, &ciErrNum);
	oclCheckError(ciErrNum, CL_SUCCESS);
}

OpenCLClass::~OpenCLClass()
{
	/* Finalization */
	if (volumeSamplerLinear)ciErrNum |= clReleaseSampler(volumeSamplerLinear);
	oclCheckError(ciErrNum, CL_SUCCESS);
	if (transferFuncSampler)ciErrNum |= clReleaseSampler(transferFuncSampler);
	oclCheckError(ciErrNum, CL_SUCCESS);
	if (d_volumeArray)ciErrNum |= clReleaseMemObject(d_volumeArray);
	oclCheckError(ciErrNum, CL_SUCCESS);
#ifdef NOT_RAY_BOX
	if (d_textureFirst)ciErrNum |= clReleaseMemObject(d_textureFirst);
	oclCheckError(ciErrNum, CL_SUCCESS);
	if (d_textureLast)ciErrNum |= clReleaseMemObject(d_textureLast);
	oclCheckError(ciErrNum, CL_SUCCESS);
	if (hitSampler)ciErrNum |= clReleaseSampler(hitSampler);
	oclCheckError(ciErrNum, CL_SUCCESS);
#endif
	if (d_transferFuncArray)ciErrNum |= clReleaseMemObject(d_transferFuncArray);
	oclCheckError(ciErrNum, CL_SUCCESS);
	if (pbo_cl)ciErrNum |= clReleaseMemObject(pbo_cl);
	oclCheckError(ciErrNum, CL_SUCCESS);
	if (d_invViewMatrix)ciErrNum |= clReleaseMemObject(d_invViewMatrix);
	oclCheckError(ciErrNum, CL_SUCCESS);
	ciErrNum |= clFlush(command_queue);
	oclCheckError(ciErrNum, CL_SUCCESS);
	ciErrNum |= clFinish(command_queue);
	oclCheckError(ciErrNum, CL_SUCCESS);
	ciErrNum |= clReleaseKernel(kernel);
	oclCheckError(ciErrNum, CL_SUCCESS);
	ciErrNum |= clReleaseProgram(program);
	oclCheckError(ciErrNum, CL_SUCCESS);
	ciErrNum |= clReleaseCommandQueue(command_queue);
	oclCheckError(ciErrNum, CL_SUCCESS);
	ciErrNum |= clReleaseContext(context);
	oclCheckError(ciErrNum, CL_SUCCESS);
}


// Helper function for using CUDA to add vectors in parallel.
void OpenCLClass::openCLRC(/*, unsigned int width, unsigned int height, float h, float4 *d_buffer, float4 *d_lastHit*/)
{

	glFinish();
	// Acquire PBO for OpenCL writing
	ciErrNum |= clEnqueueAcquireGLObjects(command_queue, 1, &pbo_cl, 0, 0, 0);
	ciErrNum |= clEnqueueAcquireGLObjects(command_queue, 1, &d_transferFuncArray, 0, 0, 0);
	ciErrNum |= clEnqueueAcquireGLObjects(command_queue, 1, &d_volumeArray, 0, 0, 0);
#ifdef NOT_RAY_BOX
	ciErrNum |= clEnqueueAcquireGLObjects(command_queue, 1, &d_textureFirst, 0, 0, 0);
	ciErrNum |= clEnqueueAcquireGLObjects(command_queue, 1, &d_textureLast, 0, 0, 0);
#endif
	oclCheckError(ciErrNum, CL_SUCCESS);

	/* Execute OpenCL Kernel */
	ciErrNum |= clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, globalSize, localSize, 0, 0, 0);
	oclCheckError(ciErrNum, CL_SUCCESS);


	//finish 
	clFinish(command_queue);
	ciErrNum |= clEnqueueReleaseGLObjects(command_queue, 1, &pbo_cl, 0, 0, 0);
	ciErrNum |= clEnqueueReleaseGLObjects(command_queue, 1, &d_transferFuncArray, 0, 0, 0);
	ciErrNum |= clEnqueueReleaseGLObjects(command_queue, 1, &d_volumeArray, 0, 0, 0);
#ifdef NOT_RAY_BOX
	ciErrNum |= clEnqueueReleaseGLObjects(command_queue, 1, &d_textureFirst, 0, 0, 0);
	ciErrNum |= clEnqueueReleaseGLObjects(command_queue, 1, &d_textureLast, 0, 0, 0);
#endif
	oclCheckError(ciErrNum, CL_SUCCESS);
}


void OpenCLClass::openCLSetVolume(unsigned int width, unsigned int height, unsigned int depth, float diagonal)
{
	if (d_volumeArray != NULL) {
		// delete old buffer
		clReleaseMemObject(d_transferFuncArray);
	}


	d_volumeArray = clCreateFromGLTexture(context, CL_MEM_READ_ONLY, GL_TEXTURE_3D, 0, TextureManager::Inst()->GetID(TEXTURE_VOLUME), &ciErrNum);
	oclCheckError(ciErrNum, CL_SUCCESS);


	volumeSamplerLinear = clCreateSampler(context, CL_TRUE, CL_ADDRESS_CLAMP, CL_FILTER_LINEAR, &ciErrNum);

	// set image and sampler args
	ciErrNum |= clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&d_volumeArray);
	ciErrNum |= clSetKernelArg(kernel, 6, sizeof(cl_sampler), &volumeSamplerLinear);

	//Set the step
	float step = 1.f / diagonal;
	ciErrNum |= clSetKernelArg(kernel, 1, sizeof(float), &step);

	oclCheckError(ciErrNum, CL_SUCCESS);
}


void OpenCLClass::openCLSetImageSize(unsigned int width, unsigned int height, float NCP, float angle){
	int r = width % localSize[0];
	globalSize[0] = (r == 0) ? width : width + localSize[0] - r;
	r = height % localSize[1];
	globalSize[1] = (r == 0) ? height : height + localSize[1] - r;



	if (pbo_cl != NULL) {
		// delete old buffer
		clReleaseMemObject(pbo_cl);
#ifdef NOT_RAY_BOX
		clReleaseMemObject(d_textureFirst);
		clReleaseMemObject(d_textureLast);
#endif
	}


	oclCheckError(ciErrNum, CL_SUCCESS);
	// create OpenCL buffer from GL PBO
	pbo_cl = clCreateFromGLTexture(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, TextureManager::Inst()->GetID(TEXTURE_FINAL_IMAGE), &ciErrNum);
	oclCheckError(ciErrNum, CL_SUCCESS);

#ifdef NOT_RAY_BOX
	d_textureFirst = clCreateFromGLTexture(context, CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, TextureManager::Inst()->GetID(TEXTURE_FRONT_HIT), &ciErrNum);
	d_textureLast = clCreateFromGLTexture(context, CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, TextureManager::Inst()->GetID(TEXTURE_BACK_HIT), &ciErrNum);
	oclCheckError(ciErrNum, CL_SUCCESS);
	ciErrNum |= clSetKernelArg(kernel, 8, sizeof(cl_mem), (void *)&d_textureFirst);
	oclCheckError(ciErrNum, CL_SUCCESS);
	ciErrNum |= clSetKernelArg(kernel, 9, sizeof(cl_mem), (void *)&d_textureLast);
	oclCheckError(ciErrNum, CL_SUCCESS);
	hitSampler = clCreateSampler(context, CL_TRUE, CL_ADDRESS_CLAMP, CL_FILTER_NEAREST, &ciErrNum);
	ciErrNum |= clSetKernelArg(kernel, 10, sizeof(cl_sampler), &hitSampler);
#endif

	ciErrNum |= clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&pbo_cl);
	oclCheckError(ciErrNum, CL_SUCCESS);
	ciErrNum |= clSetKernelArg(kernel, 2, sizeof(unsigned int), &width);
	oclCheckError(ciErrNum, CL_SUCCESS);
	ciErrNum |= clSetKernelArg(kernel, 3, sizeof(unsigned int), &height);
	oclCheckError(ciErrNum, CL_SUCCESS);
#ifndef NOT_RAY_BOX
	ciErrNum |= clSetKernelArg(kernel, 9, sizeof(float), &angle);
	oclCheckError(ciErrNum, CL_SUCCESS);
	ciErrNum |= clSetKernelArg(kernel, 10, sizeof(float), &NCP);
	oclCheckError(ciErrNum, CL_SUCCESS);
#endif
}

#ifndef NOT_RAY_BOX
void OpenCLClass::openCLUpdateMatrix(const float * matrix){
	ciErrNum |= clEnqueueWriteBuffer(command_queue, d_invViewMatrix, CL_FALSE, 0, 16 * sizeof(float), matrix, 0, 0, 0);
	ciErrNum |= clSetKernelArg(kernel, 8, sizeof(cl_mem), (void *)&d_invViewMatrix);

	oclCheckError(ciErrNum, CL_SUCCESS);
}
#endif


void OpenCLClass::openCLSetTransferFunction(){
	
	if (d_transferFuncArray != NULL) {
		// delete old buffer
		clReleaseMemObject(d_transferFuncArray);
	}
	
	
	//Pass texture to Opencl
	d_transferFuncArray = clCreateFromGLTexture(context, CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, TextureManager::Inst()->GetID(TEXTURE_TRANSFER_FUNC), &ciErrNum);
	oclCheckError(ciErrNum, CL_SUCCESS);


	// Create samplers for transfer function, linear interpolation and nearest interpolation 
	transferFuncSampler = clCreateSampler(context, CL_TRUE, CL_ADDRESS_CLAMP, CL_FILTER_NEAREST, &ciErrNum);


	
	// set image and sampler args
	ciErrNum |= clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *)&d_transferFuncArray);
	ciErrNum |= clSetKernelArg(kernel, 7, sizeof(cl_sampler), &transferFuncSampler);

	oclCheckError(ciErrNum, CL_SUCCESS);
}



void OpenCLClass::Use(GLenum activeTexture){
	// copy from pbo to texture
	glActiveTexture(activeTexture);
	TextureManager::Inst()->BindTexture(TEXTURE_FINAL_IMAGE);
}