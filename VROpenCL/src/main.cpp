#include "Definitions.h"
#include "GLSLProgram.h"
#include "TransferFunction.h"
#include "TextureManager.h"
#include "CubeIntersection.h"
#include "FBOCube.h"
#include "FBOQuad.h"
#include "Volume.h"
#include "FinalImage.h"
#include "kernel.h"
#include <iostream>

using std::cout;
using std::endl;



namespace glfwFunc
{
	GLFWwindow* glfwWindow;
	int WINDOW_WIDTH = 1024;
	int WINDOW_HEIGHT = 768;
	std::string strNameWindow = "Hello GLFW";

	const float NCP = 0.01f;
	const float FCP = 45.0f;
	const float fAngle = 45.f * (3.14f / 180.0f); //In radians

	//Declare the transfer function
	TransferFunction *g_pTransferFunc;

	//Class to wrap opencl code
	OpenCLClass * opencl;

	float color[]={1,1,1};
	bool pintar = false;

	glm::mat4x4 mProjMatrix, mModelViewMatrix, mMVP;

	//Variables to do rotation
	glm::quat quater, q2;
	glm::mat4x4 RotationMat = glm::mat4x4();
	float angle = 0;
	float *vector=(float*)malloc(sizeof(float)*3);
	double lastx, lasty;
	bool pres = false;

	CCubeIntersection *m_BackInter, *m_FrontInter;
	CFinalImage *m_FinalImage;


	Volume *volume = NULL;


	void TW_CALL pressExit(void *clientData)
	{ 
		TwTerminate();
		exit(0);
	}

	
	///< Callback function used by GLFW to capture some possible error.
	void errorCB(int error, const char* description)
	{
		printf("%s\n",description );
	}


	inline int TwEventMouseWheelGLFW3(GLFWwindow* window, double xoffset, double yoffset)
	{return TwEventMouseWheelGLFW((int)yoffset);}
	inline int TwEventCharGLFW3(GLFWwindow* window, int codepoint)
	{return TwEventCharGLFW(codepoint, GLFW_PRESS);}
	inline int TwWindowSizeGLFW3(GLFWwindow* window, int width, int height)
	{return TwWindowSize(width, height);}


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
		if(!TwEventKeyGLFW(iKey, iAction)){
			if (iAction == GLFW_PRESS)
			{
				switch (iKey)
				{
					case GLFW_KEY_ESCAPE:
					case GLFW_KEY_Q:
						glfwSetWindowShouldClose(window, GL_TRUE);
						break;
					case GLFW_KEY_SPACE:
						g_pTransferFunc->isVisible = !g_pTransferFunc->isVisible;
						break;
				}
			}
		}
	}

	inline int TwEventMousePosGLFW3(GLFWwindow* window, double xpos, double ypos)
	{ 
	
		TwMouseMotion(int(xpos), int(ypos));
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

		int t1 = TwEventMouseButtonGLFW(button, action);
		bool t2 = g_pTransferFunc->MouseButton((int)x, (int)y, button, action);
		if(!t1 && !t2){
			
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
		TwWindowSizeGLFW3(window, iWidth, iHeight);
		m_BackInter->SetResolution(iWidth, iHeight);
		m_FrontInter->SetResolution(iWidth, iHeight);
		m_FinalImage->SetResolution(iWidth, iHeight);
		g_pTransferFunc->Resize(&WINDOW_WIDTH, &WINDOW_HEIGHT);
		
		//Set image Size
		opencl->openCLSetImageSize(iWidth, iHeight, NCP, fAngle / 2.0f);
		
	}

	///< The main rendering function.
	void draw()
	{

		GLenum err = GL_NO_ERROR;
		while((err = glGetError()) != GL_NO_ERROR)
		{
		  std::cout<<"INICIO "<< err<<std::endl;
		}
		
		RotationMat = glm::mat4_cast(glm::normalize(quater));

		mModelViewMatrix =  glm::translate(glm::mat4(), glm::vec3(0.0f,0.0f,-2.0f)) * 
							RotationMat; 


		mMVP = mProjMatrix * mModelViewMatrix;

		opencl->openCLUpdateMatrix(glm::value_ptr(glm::transpose(glm::inverse(mModelViewMatrix))));

		/*//Obtain Back hits
		m_BackInter->Draw(mMVP);
		//Obtain the front hits
		m_FrontInter->Draw(mMVP);*/


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

		while((err = glGetError()) != GL_NO_ERROR)
		{
		  std::cout<<"Swap "<< err<<std::endl;
		  //exit(0);
		}

		
	}
	

	//Con esta funcion se puede obtener el valor 
	void TW_CALL SetVarCallback(const void *value, void *clientData)
	{
		color[0] = ((const GLfloat *)value)[0]; 
		color[1] = ((const GLfloat *)value)[1]; 
		color[2] = ((const GLfloat *)value)[2]; 
	}

	void TW_CALL GetVarCallback(void *value, void *clientData)
	{
		((GLfloat*) value)[0] = color[0];
		((GLfloat*) value)[1] = color[1];
		((GLfloat*) value)[2] = color[2];
	}


	///
	/// Init all data and variables.
	/// @return true if everything is ok, false otherwise
	///
	bool initialize()
	{
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
		glCullFace(GL_BACK);

		//Init GLEW
		glewExperimental = GL_TRUE;
		if (glewInit() != GLEW_OK)
		{
			printf("- glew Init failed :(\n");
			return false;
		}
		printf("OpenGL version: %s\n", glGetString(GL_VERSION));
		printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
		printf("Vendor: %s\n", glGetString(GL_VENDOR));
		printf("Renderer: %s\n", glGetString(GL_RENDERER));

		opencl = new OpenCLClass(glfwWindow);


		//Init the transfer function
		g_pTransferFunc = new TransferFunction();
		g_pTransferFunc->InitContext(glfwWindow, &WINDOW_WIDTH, &WINDOW_HEIGHT, -1, -1);

		opencl->openCLSetTransferFunction((cl_float4 *)g_pTransferFunc->colorPalette, 256);

		TwInit(TW_OPENGL_CORE, NULL);
		TwWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);

		// send window size events to AntTweakBar
		glfwSetWindowSizeCallback(glfwWindow, resizeCB);
		glfwSetMouseButtonCallback(glfwWindow, (GLFWmousebuttonfun)TwEventMouseButtonGLFW3);
		glfwSetCursorPosCallback(glfwWindow, (GLFWcursorposfun)TwEventMousePosGLFW3);
		glfwSetScrollCallback(glfwWindow, (GLFWscrollfun)TwEventMouseWheelGLFW3);
		glfwSetKeyCallback(glfwWindow, (GLFWkeyfun)keyboardCB);
		glfwSetCharCallback(glfwWindow, (GLFWcharfun)TwEventCharGLFW3);

		TwBar *myBar;
		myBar = TwNewBar("Opciones");

		//Definicion de un boton para cambiar un color utilizando callbacks
		TwAddVarCB(myBar, "Color", TW_TYPE_COLOR3F, SetVarCallback, GetVarCallback, color, "label='Color Triangulo' group=Triangulo");

		//Definicion de un boton para cambiar un color sin utilizar callbacks
		TwAddVarRW(myBar, "Color2", TW_TYPE_COLOR3F, color, "label='Color Triangulo2' group=Triangulo");
		TwAddButton(myBar, "Salir", pressExit, NULL, "label='Salir' group=Archivo");
		TwAddVarRW(myBar, "Dibujar", TW_TYPE_BOOLCPP, &pintar, "label='Dibujar' group=Triangulo");
		TwAddVarRW(myBar, "ObjRotation", TW_TYPE_QUAT4F, &quater, " label='Object rotation' open help='Change the object orientation.' ");


		//Create volume
		volume = new Volume();
		volume->Load("Raw/foot_8_256_256_256.raw", 256, 256, 256);

		opencl->openCLSetVolume((cl_char *)volume->volume, 256, 256, 256, volume->m_fDiagonal);

		


		m_BackInter = new CCubeIntersection(false, WINDOW_WIDTH, WINDOW_HEIGHT);
		m_FrontInter = new CCubeIntersection(true, WINDOW_WIDTH, WINDOW_HEIGHT);
		m_FinalImage = new CFinalImage(WINDOW_WIDTH, WINDOW_HEIGHT);


		return true;
	}


	/// Here all data must be destroyed + glfwTerminate
	void destroy()
	{
		delete m_BackInter;
		delete g_pTransferFunc;
		delete opencl;
		TextureManager::Inst()->UnloadAllTextures();
		glfwTerminate();
		glfwDestroyWindow(glfwWindow);
	}
}

int main(int argc, char** argv)
{

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

	// main loop!
	while (!glfwWindowShouldClose(glfwFunc::glfwWindow))
	{

		if(glfwFunc::g_pTransferFunc->updateTexture) // Check if the color palette changed    
		{
			//glfwFunc::g_pTransferFunc->UpdatePallete();
			glfwFunc::g_pTransferFunc->updateTexture = false;
			glfwFunc::opencl->openCLSetTransferFunction((cl_float4 *)glfwFunc::g_pTransferFunc->colorPalette, 256);
		}
		glfwFunc::draw();
		glfwPollEvents();	//or glfwWaitEvents()
	}

	glfwFunc::destroy();
	return EXIT_SUCCESS;
}
