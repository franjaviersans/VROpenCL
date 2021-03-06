#include "Definitions.h"
#include "GLSLProgram.h"
#include "TransferFunction.h"
#include "TextureManager.h"
#include "CubeIntersection.h"
#include "VBOCube.h"
#include "VBOQuad.h"
#include "Volume.h"
#include "FinalImage.h"
#include "kernel.h"
#include "Timer.h" 
#include <iostream>
#include <fstream>

using std::cout;
using std::endl;



namespace glfwFunc
{
	GLFWwindow* glfwWindow;
	int WINDOW_WIDTH = 1024;
	int WINDOW_HEIGHT = 768;
	std::string strNameWindow = "OpenCL Volume Ray Casting";

	const float NCP = 1.0f;
	const float FCP = 10.0f;
	const float fAngle = 45.f * (3.14f / 180.0f); //In radians

	//Declare the transfer function
	TransferFunction *g_pTransferFunc;

	char * volume_filepath = "./Raw/volume.raw";
	char * transfer_func_filepath = NULL;
	glm::ivec3 vol_size = glm::ivec3(256, 256, 256);
	glm::ivec2 block_dimension = glm::ivec2(16, 16);
	glm::mat4 scale = glm::mat4();
	bool bits8 = true;
	int offset = 0;

	//Class to wrap opencl code
	OpenCLClass * opencl;

	float color[]={1,1,1};
	bool pintar = false;

	glm::mat4x4 mProjMatrix, mModelViewMatrix, mMVP;

	//Variables to do rotation
	glm::quat quater = glm::quat(0.8847f, -.201f, 0.398f, -0.1339f), q2; //begin with a diagonal view
	glm::mat4x4 RotationMat = glm::mat4x4();
	float angle = 0;
	float *vector=(float*)malloc(sizeof(float)*3);
	double lastx, lasty;
	bool pres = false;

#ifdef NOT_RAY_BOX
	CCubeIntersection *m_BackInter, *m_FrontInter;
#endif
	CFinalImage *m_FinalImage;

#ifdef MEASURE_TIME
	std::ofstream time_file("Time.txt", std::ios::out);
	// helper variable
	TimerManager timer;
	int num;
#endif



	Volume *volume = NULL;

	
	///< Callback function used by GLFW to capture some possible error.
	void errorCB(int error, const char* description)
	{
		printf("%s\n",description );
	}


	///
	/// The keyboard function call back
	/// @param window id of the window that received the event
	/// @param iKey the key pressed or released
	/// @param iScancode the system-specific scancode of the key.
	/// @param iAction can be GLFW_PRESS, GLFW_RELEASE or GLFW_REPEAT
	/// @param iMods Bit field describing which modifier keys were held down (Shift, Alt, & so on)
	///
	void keyboardCB(GLFWwindow* window, int iKey, int iScancode, int iAction, int iMods)
	{
		if (iAction == GLFW_PRESS)
		{
			switch (iKey)
			{
				case GLFW_KEY_ESCAPE:
				case GLFW_KEY_Q:
					glfwSetWindowShouldClose(window, GL_TRUE);
					break;
				case GLFW_KEY_SPACE:
				{
					static bool visible = false;
					visible = !visible;
					g_pTransferFunc->SetVisible(visible);
				}
					break;
				case GLFW_KEY_S:
					g_pTransferFunc->SaveToFile("TransferFunction.txt");
					break;
			}
		}
	}

	inline int TwEventMousePosGLFW3(GLFWwindow* window, double xpos, double ypos)
	{ 
	
		g_pTransferFunc->CursorPos(int(xpos), int(ypos));
		if(pres){
			//Rotation
			float dx = float(xpos - lastx);
			float dy = float(ypos - lasty);

			if(!(dx == 0 && dy == 0)){
				//Calculate angle and rotation axis
				float angle = sqrtf(dx*dx + dy*dy)/50.0f;
					
				//Acumulate rotation with quaternion multiplication
				q2 = glm::angleAxis(angle, glm::normalize(glm::vec3(dy,dx,0.0f)));
				quater = glm::cross(q2, quater);

				lastx = xpos;
				lasty = ypos;
			}
			return false;
		}
		return true;
	}

	int TwEventMouseButtonGLFW3(GLFWwindow* window, int button, int action, int mods)
	{ 
		pres = false;

		double x, y;   
		glfwGetCursorPos(window, &x, &y);  

		if (!g_pTransferFunc->MouseButton((int)x, (int)y, button, action)){
			
			if(button == GLFW_MOUSE_BUTTON_LEFT){
				if(action == GLFW_PRESS){
					lastx = x;
					lasty = y;
					pres = true;
				}

				return true;
			}else{
				if(action == GLFW_PRESS){			
				}
				
			}
			
			return false;
		}

		return true;
	}
	
	///< The resizing function
	void resizeCB(GLFWwindow* window, int iWidth, int iHeight)
	{

		WINDOW_WIDTH = iWidth;
		WINDOW_HEIGHT = iHeight;

		if(iHeight == 0) iHeight = 1;
		float ratio = iWidth / float(iHeight);
		glViewport(0, 0, iWidth, iHeight);

		mProjMatrix = glm::perspective(float(fAngle), ratio, 1.0f, 10.0f);

		// Update size in some buffers!!!
#ifdef NOT_RAY_BOX
		m_BackInter->SetResolution(iWidth, iHeight);
		m_FrontInter->SetResolution(iWidth, iHeight);
#endif
		m_FinalImage->SetResolution(iWidth, iHeight);
		g_pTransferFunc->Resize(&WINDOW_WIDTH, &WINDOW_HEIGHT);
		
		//Set image Size
		opencl->openCLSetImageSize(iWidth, iHeight, NCP, fAngle / 2.0f);
		
	}

	///< The main rendering function.
	void draw()
	{
		RotationMat = glm::mat4_cast(glm::normalize(quater));

		mModelViewMatrix =  glm::translate(glm::mat4(), glm::vec3(0.0f,0.0f,-2.0f)) * 
							RotationMat * scale; 


		mMVP = mProjMatrix * mModelViewMatrix;

#ifndef NOT_RAY_BOX
		opencl->openCLUpdateMatrix(glm::value_ptr(glm::transpose(glm::inverse(mModelViewMatrix))));
#else
		//Obtain Back hits
		m_BackInter->Draw(mMVP);
		//Obtain the front hits
		m_FrontInter->Draw(mMVP);
#endif

#ifdef LIGHTING
		vec3 lightDir = vec3(inverse(mModelViewMatrix) * vec4(0.0, 0.0, -1.0f, 0.0f));
		lightDir = normalize(lightDir);
		opencl->openCLUpdateLight(glm::value_ptr(lightDir));
#endif



		//Opencl volume ray casting
		opencl->openCLRC();


		
		

		//Draw the quad with the final result
		glClearColor(0.15f, 0.15f, 0.15f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		//Blend with bg
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		opencl->Use(GL_TEXTURE0); //Use the texture
		m_FinalImage->Draw();
		glDisable(GL_BLEND);



		//Draw the AntTweakBar
		//TwDraw();

		//Draw the transfer function
		g_pTransferFunc->Display();

		glfwSwapBuffers(glfwWindow);
		
	}

	///< Function to warup opencl
	void WarmUP(unsigned int cycles, bool measure = false){

		
#ifdef MEASURE_TIME
		if (measure){
			timer.Start();
		}
#endif

		for (int i = 0; i < cycles; ++i) {
			RotationMat = glm::mat4_cast(glm::normalize(quater));

			mModelViewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, -2.0f)) *
				RotationMat * scale;

			mMVP = mProjMatrix * mModelViewMatrix;

			
#ifdef LIGHTING
			vec3 lightDir = vec3(inverse(mModelViewMatrix) * vec4(0.0, 0.0, -1.0f, 0.0f));
			lightDir = normalize(lightDir);
			opencl->openCLUpdateLight(glm::value_ptr(lightDir));
#endif


#ifndef NOT_RAY_BOX
			opencl->openCLUpdateMatrix(glm::value_ptr(glm::transpose(glm::inverse(mModelViewMatrix))));
#else
			//Obtain Back hits
			m_BackInter->Draw(mMVP);
			//Obtain the front hits
			m_FrontInter->Draw(mMVP);
#endif






			//Opencl volume ray casting
			opencl->openCLRC();
		}


#ifdef MEASURE_TIME
		if (measure){
			timer.Stop();
			time_file << timer.GetAverageTime(cycles) << endl;
			time_file.close();
		}
#endif

	}
	


	///
	/// Init all data and variables.
	/// @return true if everything is ok, false otherwise
	///
	bool initialize()
	{
		//Init GLEW
		//load with glad
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			printf("Something went wrong!\n");
			exit(-1);
		}
		printf("OpenGL version: %s\n", glGetString(GL_VERSION));
		printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
		printf("Vendor: %s\n", glGetString(GL_VENDOR));
		printf("Renderer: %s\n", glGetString(GL_RENDERER));


		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
		glCullFace(GL_BACK);

		opencl = new OpenCLClass(glfwWindow, block_dimension);


		//Init the transfer function
		g_pTransferFunc = new TransferFunction();
		g_pTransferFunc->InitContext(&WINDOW_WIDTH, &WINDOW_HEIGHT, transfer_func_filepath, -1, -1);

		opencl->openCLSetTransferFunction();


		// send window size events to AntTweakBar
		glfwSetWindowSizeCallback(glfwWindow, resizeCB);
		glfwSetMouseButtonCallback(glfwWindow, (GLFWmousebuttonfun)TwEventMouseButtonGLFW3);
		glfwSetCursorPosCallback(glfwWindow, (GLFWcursorposfun)TwEventMousePosGLFW3);
		glfwSetKeyCallback(glfwWindow, (GLFWkeyfun)keyboardCB);



		//Create volume
		volume = new Volume();
		volume->Load(volume_filepath, vol_size.x, vol_size.y, vol_size.z, bits8, offset);

		opencl->openCLSetVolume(vol_size.x, vol_size.y, vol_size.z, volume->m_fDiagonal);

#ifdef LIGHTING
		opencl->openCLUpdateVoxelSize(glm::value_ptr(volume->getVoxelSize()));
#endif	

#ifdef NOT_RAY_BOX
		m_BackInter = new CCubeIntersection(false, WINDOW_WIDTH, WINDOW_HEIGHT);
		m_FrontInter = new CCubeIntersection(true, WINDOW_WIDTH, WINDOW_HEIGHT);
#endif
		m_FinalImage = new CFinalImage(WINDOW_WIDTH, WINDOW_HEIGHT);

#ifdef MEASURE_TIME
		// get the tick frequency from the OS
		timer.Init();
		num = 0;
#endif


		return true;
	}


	/// Here all data must be destroyed + glfwTerminate
	void destroy()
	{
#ifdef NOT_RAY_BOX
		delete m_BackInter;
		delete g_pTransferFunc;
#endif
		delete opencl;
		TextureManager::Inst().UnloadAllTextures();
		glfwTerminate();
		glfwDestroyWindow(glfwWindow);
	}
}

int main(int argc, char** argv)
{

#ifdef NOT_RAY_BOX
	cout << "Using image intersection" << endl;
#else
	cout << "Using ray box intersection" << endl;
#endif

#ifdef LIGHTING
	cout << "Using lighting" << endl;
#else
	cout << "Without lighting" << endl;
#endif

#ifdef NOT_DISPLAY
	cout << "NOT display" << endl;
#else
	cout << "Display result in screen" << endl;
#endif

#ifdef MEASURE_TIME
	cout << "Measuring time" << endl;
#else
	cout << "NOT Measuring time" << endl;
#endif

	if (argc == 12 || argc == 13) {

		//Copy volume file path
		glfwFunc::volume_filepath = new char[strlen(argv[1]) + 1];
		strncpy_s(glfwFunc::volume_filepath, strlen(argv[1]) + 1, argv[1], strlen(argv[1]));

		//Volume size
		int width = atoi(argv[2]), height = atoi(argv[3]), depth = atoi(argv[4]);
		glfwFunc::vol_size = glm::ivec3(width, height, depth);

		//number of bits;
		int bits = atoi(argv[5]);
		glfwFunc::bits8 = (bits == 8);

		//scale factor of the volume
		glfwFunc::scale = glm::scale(glm::mat4(), glm::vec3(atof(argv[6]), atof(argv[7]), atof(argv[8])));

		//offset
		glfwFunc::offset = atoi(argv[9]);

		//working group size
		glfwFunc::block_dimension.x = atoi(argv[10]);
		glfwFunc::block_dimension.y = atoi(argv[11]);

		//Copy volume transfer function path
		if (argc == 13){
			glfwFunc::transfer_func_filepath = new char[strlen(argv[12]) + 1];
			strncpy_s(glfwFunc::transfer_func_filepath, strlen(argv[12]) + 1, argv[12], strlen(argv[12]));
		}

	}
	else if (argc > 13) {
		printf("Too many arguments supplied!!!! \n");
	}

	glfwSetErrorCallback(glfwFunc::errorCB);
	if (!glfwInit())	exit(EXIT_FAILURE);
	glfwFunc::glfwWindow = glfwCreateWindow(glfwFunc::WINDOW_WIDTH, glfwFunc::WINDOW_HEIGHT, glfwFunc::strNameWindow.c_str(), NULL, NULL);
	if (!glfwFunc::glfwWindow)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(glfwFunc::glfwWindow);
	if(!glfwFunc::initialize()) exit(EXIT_FAILURE);
	glfwFunc::resizeCB(glfwFunc::glfwWindow, glfwFunc::WINDOW_WIDTH, glfwFunc::WINDOW_HEIGHT);	//just the 1st time



	//WarmUP!!!!
	glfwFunc::WarmUP(20);


#ifndef NOT_DISPLAY
	// main loop!
#ifndef MEASURE_TIME
	while (!glfwWindowShouldClose(glfwFunc::glfwWindow))
	{
#else
	glfwFunc::timer.Start();
	while (glfwFunc::num <= NUM_CYCLES)
	{
#endif

#ifndef MEASURE_TIME
		if (glfwFunc::g_pTransferFunc->NeedUpdate()) // Check if the color palette changed    
		{
			glfwFunc::g_pTransferFunc->UpdatePallete();
			glfwFunc::g_pTransferFunc->SetUpdate(false);
		}
#endif
		glfwFunc::draw();

#ifndef MEASURE_TIME
		glfwPollEvents();	//or glfwWaitEvents()
#else
		++glfwFunc::num;
#endif
	}

#ifdef MEASURE_TIME
	
	glfwFunc::timer.Stop();
	glfwFunc::time_file << glfwFunc::timer.GetAverageTime(glfwFunc::num) << endl;
	glfwFunc::time_file.close();
#endif
#else

	glfwFunc::WarmUP(NUM_CYCLES, true);

#endif

	glfwFunc::destroy();
	return EXIT_SUCCESS;
}
