// $Id: $
//
// Author: Francisco Sans franjaviersans@gmail.com
//
// Complete history on bottom of file

#ifndef CUBEINTER_H
#define CUBEINTER_H

//Includes
#include "Definitions.h"
#include "FBOCube.h"
#include "GLSLProgram.h"


class CCubeIntersection
{
	//Functions

	public:
		///Default constructor
		CCubeIntersection(bool front, GLuint W, GLuint H);

		///Default destructor
		~CCubeIntersection();

		///Set resolution
		void SetResolution(GLuint ResW, GLuint ResH);

		///Method to Draw the Quad
		void Draw(glm::mat4 &);

		///Debug Texture
		void Debug();

		///Use texture
		void Use(GLenum);

	//Variables
	private:
		GLuint m_iFrameBuffer, depthrenderbuffer, m_uiWidth, m_uiHeight, txtHit;
		GLSLProgram m_program;
		bool m_bFront;
};


#endif //FBOQuad_H