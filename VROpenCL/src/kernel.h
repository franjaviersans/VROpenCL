

#ifndef OPENCL_H
#define OPENCL_H


#include <stdio.h>
#include <stdlib.h>


#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#include <CL/cl_gl.h>
#endif

#if defined (__APPLE__) || defined(MACOSX)
	#define GL_SHARING_EXTENSION "cl_APPLE_gl_sharing"
#else
	#define GL_SHARING_EXTENSION "cl_khr_gl_sharing"
#endif


#include "Definitions.h"
#include "TextureManager.h"
#include "helper_math.h"

#define MEM_SIZE (128)
#define MAX_SOURCE_SIZE (0x100000)


// Helper function to get OpenCL error string from constant
// *********************************************************************
inline const char* oclErrorString(cl_int error)
{
	static const char* errorString[] = {
		"CL_SUCCESS",
		"CL_DEVICE_NOT_FOUND",
		"CL_DEVICE_NOT_AVAILABLE",
		"CL_COMPILER_NOT_AVAILABLE",
		"CL_MEM_OBJECT_ALLOCATION_FAILURE",
		"CL_OUT_OF_RESOURCES",
		"CL_OUT_OF_HOST_MEMORY",
		"CL_PROFILING_INFO_NOT_AVAILABLE",
		"CL_MEM_COPY_OVERLAP",
		"CL_IMAGE_FORMAT_MISMATCH",
		"CL_IMAGE_FORMAT_NOT_SUPPORTED",
		"CL_BUILD_PROGRAM_FAILURE",
		"CL_MAP_FAILURE",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"CL_INVALID_VALUE",
		"CL_INVALID_DEVICE_TYPE",
		"CL_INVALID_PLATFORM",
		"CL_INVALID_DEVICE",
		"CL_INVALID_CONTEXT",
		"CL_INVALID_QUEUE_PROPERTIES",
		"CL_INVALID_COMMAND_QUEUE",
		"CL_INVALID_HOST_PTR",
		"CL_INVALID_MEM_OBJECT",
		"CL_INVALID_IMAGE_FORMAT_DESCRIPTOR",
		"CL_INVALID_IMAGE_SIZE",
		"CL_INVALID_SAMPLER",
		"CL_INVALID_BINARY",
		"CL_INVALID_BUILD_OPTIONS",
		"CL_INVALID_PROGRAM",
		"CL_INVALID_PROGRAM_EXECUTABLE",
		"CL_INVALID_KERNEL_NAME",
		"CL_INVALID_KERNEL_DEFINITION",
		"CL_INVALID_KERNEL",
		"CL_INVALID_ARG_INDEX",
		"CL_INVALID_ARG_VALUE",
		"CL_INVALID_ARG_SIZE",
		"CL_INVALID_KERNEL_ARGS",
		"CL_INVALID_WORK_DIMENSION",
		"CL_INVALID_WORK_GROUP_SIZE",
		"CL_INVALID_WORK_ITEM_SIZE",
		"CL_INVALID_GLOBAL_OFFSET",
		"CL_INVALID_EVENT_WAIT_LIST",
		"CL_INVALID_EVENT",
		"CL_INVALID_OPERATION",
		"CL_INVALID_GL_OBJECT",
		"CL_INVALID_BUFFER_SIZE",
		"CL_INVALID_MIP_LEVEL",
		"CL_INVALID_GLOBAL_WORK_SIZE",
	};

	const int errorCount = sizeof(errorString) / sizeof(errorString[0]);

	const int index = -error;

	return (index >= 0 && index < errorCount) ? errorString[index] : "Unspecified Error";
}




// Error and Exit Handling Macros... 
// *********************************************************************
// Full error handling macro with Cleanup() callback (if supplied)... 
// (Companion Inline Function lower on page)
#define oclCheckErrorEX(a, b, c) __oclCheckErrorEX(a, b, c, __FILE__ , __LINE__) 

// Short version without Cleanup() callback pointer
// Both Input (a) and Reference (b) are specified as args
#define oclCheckError(a, b) oclCheckErrorEX(a, b, 0) 

inline void __oclCheckErrorEX(cl_int iSample, cl_int iReference, void(*pCleanup)(int), const char* cFile, const int iLine)
{
	// An error condition is defined by the sample/test value not equal to the reference
	if (iReference != iSample)
	{
		// If the sample/test value isn't equal to the ref, it's an error by defnition, so override 0 sample/test value
		iSample = (iSample == 0) ? -9999 : iSample;

		printf("\n !!! Error # %i (%s) at line %i , in file %s !!!\n\n", iSample, oclErrorString(iSample), iLine, cFile);


		// Cleanup and exit, or just exit if no cleanup function pointer provided.  Use iSample (error code in this case) as process exit code.
		if (pCleanup != NULL)
		{
			pCleanup(iSample);
		}
		else
		{
			exit(iSample);
		}

		
	}
}

class OpenCLClass
{
public:
	OpenCLClass(GLFWwindow* glfwWindow, glm::ivec2 dim = glm::ivec2(16,16));
	~OpenCLClass();
	
	
	//cudaArray *d_volume, *d_texture;
	//struct cudaGraphicsResource *cuda_pbo_resource; // CUDA Graphics Resource (to transfer PBO)
#ifndef NOT_RAY_BOX
	void openCLUpdateMatrix(const float * matrix);
#endif
	void openCLRC(/*, unsigned int, unsigned int, float, float4 *, float4 **/);
	void openCLSetVolume(unsigned int width, unsigned int height, unsigned int depth, float diagonal);
	void openCLSetImageSize(unsigned int width, unsigned int height, float NCP, float angle);
	void openCLSetTransferFunction();
	void Use(GLenum activeTexture);


private:
	cl_context context;
	cl_command_queue command_queue;
	cl_program program;
	cl_kernel kernel;
	cl_platform_id platform_id;
	size_t globalSize[2];
	size_t localSize[2];
	cl_mem d_invViewMatrix;
	cl_int ciErrNum;
	cl_mem pbo_cl, d_volumeArray, d_transferFuncArray;
	cl_sampler volumeSamplerLinear, transferFuncSampler;

#ifdef NOT_RAY_BOX
	cl_mem d_textureFirst, d_textureLast;
#endif
};


#endif