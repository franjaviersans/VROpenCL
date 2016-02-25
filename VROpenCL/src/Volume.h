// $Id: $
//
// Author: Francisco Sans franjaviersans@gmail.com
//
// Complete history on bottom of file

#ifndef VOLUME_H
#define VOLUME_H

//Includes
#include "Definitions.h"
#include <string>


/**
* Class Volume.
* Class to load a volume from a file and store it in a 3D texture to display.
*/
class Volume
{
	//Functions

	public:

		float m_fDiagonal, m_fWidht, m_fHeigth, m_fDepth;

		char * volume;
		

		///Default constructor
		Volume();

		///Default destructor
		~Volume();

		///
		void Init();

		///
		void Load(std::string , GLuint , GLuint , GLuint );

		///
		void Use(GLenum );


	//Variables

	private:
};


#endif //FBOCube_H