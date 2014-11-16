/*
* University of Manchester - Advanced Computer Graphics Coursework
*
*		Particle System: Atomic Particles
*		Student: Joao Paulo T Ruschel (9571945)
*		Professors: Toby Howard; Steve Pettifer
*
*	This class (AtomicParticles) wraps all the Particle System + OpenCL-OpenGL integration.
*	It is directly responsible for all OpenCL-related functions, and calls APEmitter for
*	the particle system-related functions.
*	Most of this code is extremely techonology-specific (OpenCL) and was written heavily based
*	on the samples provided by NVIDIA. They can be downloaded from:
*		https://developer.nvidia.com/opencl
*	I, once again, do not take ownership of the original code.
*/

#ifndef APMAIN_H
#define APMAIN_H

#define GL_INTEROP
#ifdef _WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif

// OpenGL Graphics Includes
#include <GL/glew.h>
#if defined (__APPLE__) || defined(MACOSX)
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#ifdef UNIX
#include <GL/glx.h>
#endif
#endif

// Includes
#include <memory>
#include <iostream>
#include <cassert>
#include "APEmitter.h"
#include "Particle.h"
class APEmitter;

// Utilities, OpenCL and system includes
#include <oclUtils.h>
#include <shrQATest.h>

#if defined (__APPLE__) || defined(MACOSX)
#define GL_SHARING_EXTENSION "cl_APPLE_gl_sharing"
#else
#define GL_SHARING_EXTENSION "cl_khr_gl_sharing"
#endif

// Class definition
static class AtomicParticles
{
public:
	// OpenCL vars
	static cl_platform_id cpPlatform;
	static cl_context cxGPUContext;
	static cl_device_id* cdDevices;
	static cl_uint uiDevCount;
	static cl_command_queue cqCommandQueue;
	static cl_kernel ckKernel;
	static cl_program cpProgram;
	static cl_int ciErrNum;
	static char* cPathAndName;					// var for full paths to data, src, etc.
	static char* cSourceCL;						// Buffer to hold source for compilation 
	static size_t* szGlobalWorkSize;
	static char* cExecutableName;

	// vbo variables
	static GLuint id_vbo_pos, id_vbo_pos_vel;
	static GLuint id_vbo_color, id_vbo_color_vel;
	static cl_mem vbo_position, vbo_position_vel;
	static cl_mem vbo_color, vbo_color_vel;
	static GLuint id_vbo_part;
	static cl_mem vbo_part;

	// Physics 
	static float GravityX, GravityY;

	// Handle to the GLUT window
	static int iGLUTWindowHandle;

	// The main particle emitter
	static APEmitter *emitter;

	/* Initialize the system. Since it's a static class, there's no constructor.
	This must be called along with the program initialization, after OpenGL initial
	initialization, but before the main loop begins.
	*/
	static void Initialize(int argc, char** argv, unsigned int maxparticles);

	// Check OpenGL extensions
	static void CheckOpenGl();

	// Cleanup
	static void Cleanup(int iExitCode);

	// Returns the maximum number of particles allowed in the system.
	static unsigned int GetMaxParticles();

	// Create VBO (duh)
	static GLuint createVBO(cl_mem* vbo, unsigned int size);
protected:
private:
	// Program arguments
	shrBOOL bQATest = shrFALSE;
	shrBOOL bNoPrompt = shrFALSE;

	// Program arguments. Used in some methods.
	static int *pArgc;
	static char **pArgv;

	// Current max number of particles (size of VBO).
	//	This is NOT to be changed.
	static unsigned int maxParticles;
};

#endif