/*
* University of Manchester - Advanced Computer Graphics Coursework
*
*		Particle System: Atomic Particles
*		Student: Joao Paulo T Ruschel (9571945)
*		Professors: Toby Howard; Steve Pettifer
*
*	This class (APEmitter) focuses on the ParticleSystem itself. It considers that all
*	the OpenCL and OpenGL initializations have been made, and everything is working as
*	they should. It relies on some OpenCL functions for running kernels and updating
*	the VBO that were heavily based on the samples provided by NVIDIA. However, all the
*	Particle System-related functions, as well as the method used for the integration
*	between OpenCL and the system itself, were written and developed by the student.
*/

#ifndef APEMITTER_H
#define APEMITTER_H

#include "AtomicParticles.h"
#include "Particle.h"

// Class definition
class APEmitter
{
public:
	// Variables
	float anim = 0.0;
	int iFrameCount = 0;                // FPS count for averaging
	int iFrameTrigger = 90;             // FPS trigger for sampling
	int iFramesPerSec = 0;              // frames per second
	int g_Index = 0;


	// Constructor and Destructor
	APEmitter();
	virtual ~APEmitter();

	// Emit imediate
	void emit(unsigned int emitNum);

	// Configure long-term emitter

	// Display (update and render)
	void display();
protected:
private:
	// Run kernel
	void runKernel();
};

#endif	// APEMITTER_H