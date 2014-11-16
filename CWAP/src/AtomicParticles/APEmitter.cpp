/*
* University of Manchester - Advanced Computer Graphics Coursework
*
*		Particle System: Atomic Particles
*		Student: Joao Paulo T Ruschel (9571945)
*		Professors: Toby Howard; Steve Pettifer
*
*	Implementation file for the class APEmitter.
*		For more info, please refer to APEmitter.h
*/

#include "APEmitter.h"

APEmitter::APEmitter()
{

}

APEmitter::~APEmitter()
{

}

void APEmitter::display()
{
	// increment the geometry computation parameter (or set to reference for Q/A check)
	if (iFrameCount < iFrameTrigger)
	{
		anim += 0.01f;
	}

	// start timer 0 if it's update time
	double dProcessingTime = 0.0;
	if (iFrameCount >= iFrameTrigger)
	{
		shrDeltaT(0);
	}

	// run OpenCL kernel to generate vertex positions
	runKernel();

	// get processing time from timer 0, if it's update time
	if (iFrameCount >= iFrameTrigger)
	{
		dProcessingTime = shrDeltaT(0);
	}

	// Render all particles

	glPointSize(5.);

	// Set color buffer
	// NO NEED TO BIND ALWAYS! CHECK THIS THOUGH!
	/*glBindBuffer(GL_ARRAY_BUFFER, AtomicParticles::id_vbo_color);
	glColorPointer(4, GL_FLOAT, 0, 0);

	// Set position buffer
	glBindBuffer(GL_ARRAY_BUFFER, AtomicParticles::id_vbo_pos);
	glVertexPointer(3, GL_FLOAT, 4 * sizeof(float), 0);
	*/

	glBindBuffer(GL_ARRAY_BUFFER, AtomicParticles::id_vbo_part);
	glVertexPointer(3, GL_FLOAT, sizeof(Particle), 0);
	glColorPointer(4, GL_FLOAT, sizeof(Particle), 0);
	

	// verify all particles - check for particles that are now dead
	// map the buffer object into client's memory
	/*void* ptr = glMapBufferARB(GL_ARRAY_BUFFER, GL_READ_WRITE_ARB);

	int i = 0;
	int m = AtomicParticles::GetMaxParticles()*4;
	while (i < m)
	{
		if (((float*)ptr)[i + 1] < 0.01f)
		{
			((float*)ptr)[i + 1] = 0;
		}
		i = i + 4;
	}

	//((float*)ptr)[0] = 1.0f;
	//((float*)ptr)[4] = 1.0f;
	//shrLog("%.2f, %.2f, %.2f, %2.f \n", ((float*)ptr)[0], ((float*)ptr)[1], ((float*)ptr)[2], ((float*)ptr)[3]);
	//ciErrNum = clEnqueueReadBuffer(cqCommandQueue, vbo_cl, CL_TRUE, 0, sizeof(float) * 4 * mesh_height * mesh_width, ptr, 0, NULL, NULL);
	//shrCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);

	glUnmapBufferARB(GL_ARRAY_BUFFER);
	*/

	// Enable client state - enable read from the buffers
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	// Draw as points
	glDrawArrays(GL_POINTS, 0, AtomicParticles::GetMaxParticles());

	// Disable client state
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	// Increment the frame counter, and do fps if it's time
	if (iFrameCount++ > iFrameTrigger)
	{
		// set GLUT Window Title
		char cTitle[256];
		iFramesPerSec = (int)((double)iFrameCount / shrDeltaT(1));

#ifdef _WIN32
		sprintf_s(cTitle, 256, "Atomic Particles | Particles = %i | %i fps | Proc. t = %.5f s",
			(AtomicParticles::GetMaxParticles()), iFramesPerSec, dProcessingTime);
#else 
		sprintf(cTitle, "Atomic Particles | Particles = %i | %i fps | Proc. t = %.5f s",
			(mesh_width * mesh_height), iFramesPerSec, dProcessingTime);
#endif
		glutSetWindowTitle(cTitle);

		// Log fps and processing info to console and file 
		shrLog(" %s\n", cTitle);

		// reset framecount, trigger and timer
		iFrameCount = 0;
		iFrameTrigger = (iFramesPerSec > 1) ? iFramesPerSec * 2 : 1;
	}
}

void APEmitter::emit(unsigned int emitNum)
{
	/*
	shrLog("Emitted!\n");

	GLuint temp_vbo_id;
	cl_mem temp_vbo;
	temp_vbo_id = AtomicParticles::createVBO(&temp_vbo, (emitNum * 4 * sizeof(float)));

	glDeleteBuffers(1, &temp_vbo_id);
	*/
}

void APEmitter::runKernel()
{
	AtomicParticles::ciErrNum = CL_SUCCESS;
	void(*pCleanup)(int) = &AtomicParticles::Cleanup;

#ifdef GL_INTEROP   
	// map OpenGL buffer object for writing from OpenCL
	glFinish();
	AtomicParticles::ciErrNum = clEnqueueAcquireGLObjects(AtomicParticles::cqCommandQueue, 1, &AtomicParticles::vbo_position, 0, 0, 0);
	AtomicParticles::ciErrNum |= clEnqueueAcquireGLObjects(AtomicParticles::cqCommandQueue, 1, &AtomicParticles::vbo_color, 0, 0, 0);
	shrCheckErrorEX(AtomicParticles::ciErrNum, CL_SUCCESS, pCleanup);
#endif

	// Set arg and execute the kernel
	AtomicParticles::ciErrNum = clSetKernelArg(AtomicParticles::ckKernel, 3, sizeof(float), &anim);
	AtomicParticles::ciErrNum |= clEnqueueNDRangeKernel(AtomicParticles::cqCommandQueue, AtomicParticles::ckKernel, 1, NULL, &AtomicParticles::szGlobalWorkSize[0], NULL, 0, 0, 0);
	shrCheckErrorEX(AtomicParticles::ciErrNum, CL_SUCCESS, pCleanup);

#ifdef GL_INTEROP
	// unmap buffer object
	AtomicParticles::ciErrNum = clEnqueueReleaseGLObjects(AtomicParticles::cqCommandQueue, 1, &AtomicParticles::vbo_position, 0, 0, 0);
	AtomicParticles::ciErrNum |= clEnqueueReleaseGLObjects(AtomicParticles::cqCommandQueue, 1, &AtomicParticles::vbo_color, 0, 0, 0);
	shrCheckErrorEX(AtomicParticles::ciErrNum, CL_SUCCESS, pCleanup);
	clFinish(AtomicParticles::cqCommandQueue);
#else

	// Explicit Copy 
	// map the PBO to copy data from the CL buffer via host
	glBindBufferARB(GL_ARRAY_BUFFER, AtomicParticles::vbo);

	// map the buffer object into client's memory
	void* ptr = glMapBufferARB(GL_ARRAY_BUFFER, GL_WRITE_ONLY_ARB);

	AtomicParticles::ciErrNum = clEnqueueReadBuffer(AtomicParticles::cqCommandQueue, AtomicParticles::vbo_cl, CL_TRUE, 0, sizeof(float) * 4 * AtomicParticles::mesh_height * AtomicParticles::mesh_width, ptr, 0, NULL, NULL);
	shrCheckErrorEX(AtomicParticles::ciErrNum, CL_SUCCESS, pCleanup);

	glUnmapBufferARB(GL_ARRAY_BUFFER);
#endif
}