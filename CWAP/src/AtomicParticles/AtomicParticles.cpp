/*
* University of Manchester - Advanced Computer Graphics Coursework
*
*		Particle System: Atomic Particles
*		Student: Joao Paulo T Ruschel (9571945)
*		Professors: Toby Howard; Steve Pettifer
*
*	Implementation file for the class AtomicParticles.
*		For more info, please refer to AtomicParticles.h
*/

#include "AtomicParticles.h"

// Helpers
void Cleanup(int iExitCode);
void(*pCleanup)(int) = &Cleanup;

// Redefinition of all static variables declared in the header file.
// This is necessary so these are created properly on the memory segment.
APEmitter *AtomicParticles::emitter;
cl_platform_id AtomicParticles::cpPlatform;
cl_context AtomicParticles::cxGPUContext;
cl_device_id* AtomicParticles::cdDevices;
cl_uint AtomicParticles::uiDevCount;
cl_command_queue AtomicParticles::cqCommandQueue;
cl_kernel AtomicParticles::ckKernel;
cl_program AtomicParticles::cpProgram;
cl_int AtomicParticles::ciErrNum;
char* AtomicParticles::cPathAndName;
char* AtomicParticles::cSourceCL;
size_t* AtomicParticles::szGlobalWorkSize;
char* AtomicParticles::cExecutableName;
GLuint AtomicParticles::id_vbo_pos, AtomicParticles::id_vbo_pos_vel;
GLuint AtomicParticles::id_vbo_color, AtomicParticles::id_vbo_color_vel;
cl_mem AtomicParticles::vbo_position, AtomicParticles::vbo_position_vel;
cl_mem AtomicParticles::vbo_color, AtomicParticles::vbo_color_vel;

GLuint AtomicParticles::id_vbo_part;
cl_mem AtomicParticles::vbo_part;

float AtomicParticles::GravityX, AtomicParticles::GravityY;
int AtomicParticles::iGLUTWindowHandle;
int *AtomicParticles::pArgc;
char **AtomicParticles::pArgv;
unsigned int AtomicParticles::maxParticles;

// Initializes all OpenCL environment, as well as the emitter
//*****************************************************************************
void AtomicParticles::Initialize(int argc, char** argv, unsigned int maxparticles)
{
	pArgc = &argc;
	pArgv = argv;

	// initial init
	cPathAndName = NULL;
	cSourceCL = NULL;
	maxParticles = maxparticles;
	szGlobalWorkSize = new size_t[0]();
	szGlobalWorkSize[0] = maxParticles;
	//szGlobalWorkSize[1] = maxParticles;
	cExecutableName = NULL;
	iGLUTWindowHandle = 0;

	// Initial physics
	GravityX = 0.0f;
	GravityY = 9.8f;

	// start logs 
	shrQAStart(argc, argv);
	cExecutableName = argv[0];
	shrSetLogFileName("AT_log.txt");
	//shrLog("%s Starting...\n\n", argv[0]);

	//Get the NVIDIA platform
	ciErrNum = oclGetPlatformID(&cpPlatform);
	oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);

	// Get the number of GPU devices available to the platform
	ciErrNum = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 0, NULL, &uiDevCount);
	oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);

	// Create the device list
	cdDevices = new cl_device_id[uiDevCount];
	ciErrNum = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, uiDevCount, cdDevices, NULL);
	oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);

	// Get device requested on command line, if any
	unsigned int uiDeviceUsed = 0;
	unsigned int uiEndDev = uiDevCount - 1;
	if (shrGetCmdLineArgumentu(argc, (const char**)argv, "device", &uiDeviceUsed))
	{
		uiDeviceUsed = CLAMP(uiDeviceUsed, 0, uiEndDev);
		uiEndDev = uiDeviceUsed;
	}

	// Check if the requested device (or any of the devices if none requested) supports context sharing with OpenGL
	bool bSharingSupported = false;
	for (unsigned int i = uiDeviceUsed; (!bSharingSupported && (i <= uiEndDev)); ++i)
	{
		size_t extensionSize;
		ciErrNum = clGetDeviceInfo(cdDevices[i], CL_DEVICE_EXTENSIONS, 0, NULL, &extensionSize);
		oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);
		if (extensionSize > 0)
		{
			char* extensions = (char*)malloc(extensionSize);
			ciErrNum = clGetDeviceInfo(cdDevices[i], CL_DEVICE_EXTENSIONS, extensionSize, extensions, &extensionSize);
			oclCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);
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

	shrLog("%s...\n\n", bSharingSupported ? "Using CL-GL Interop" : "No device found that supports CL/GL context sharing");
	oclCheckErrorEX(bSharingSupported, true, pCleanup);

	// Define OS-specific context properties and create the OpenCL context
#if defined (__APPLE__)
	CGLContextObj kCGLContext = CGLGetCurrentContext();
	CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
	cl_context_properties props[] =
	{
		CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)kCGLShareGroup,
		0
	};
	cxGPUContext = clCreateContext(props, 0, 0, NULL, NULL, &ciErrNum);
#else
#ifdef UNIX
	cl_context_properties props[] =
	{
		CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
		CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
		CL_CONTEXT_PLATFORM, (cl_context_properties)cpPlatform,
		0
	};
	cxGPUContext = clCreateContext(props, 1, &cdDevices[uiDeviceUsed], NULL, NULL, &ciErrNum);
#else // Win32
	cl_context_properties props[] =
	{
		CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
		CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
		CL_CONTEXT_PLATFORM, (cl_context_properties)cpPlatform,
		0
	};
	cxGPUContext = clCreateContext(props, 1, &cdDevices[uiDeviceUsed], NULL, NULL, &ciErrNum);
#endif
#endif
	shrCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);

	// Log device used (reconciled for requested requested and/or CL-GL interop capable devices, as applies)
	shrLog("Device # %u, ", uiDeviceUsed);
	oclPrintDevName(LOGBOTH, cdDevices[uiDeviceUsed]);
	shrLog("\n");

	// create a command-queue
	cqCommandQueue = clCreateCommandQueue(cxGPUContext, cdDevices[uiDeviceUsed], 0, &ciErrNum);
	shrCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);

	// Program Setup
	size_t program_length;
	cPathAndName = shrFindFilePath("particle_processing.cl", argv[0]);
	shrCheckErrorEX(cPathAndName != NULL, shrTRUE, pCleanup);
	cSourceCL = oclLoadProgSource(cPathAndName, "", &program_length);
	shrCheckErrorEX(cSourceCL != NULL, shrTRUE, pCleanup);

	// create the program
	cpProgram = clCreateProgramWithSource(cxGPUContext, 1,
		(const char **)&cSourceCL, &program_length, &ciErrNum);
	shrCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);

	// build the program
	ciErrNum = clBuildProgram(cpProgram, 0, NULL, "-cl-fast-relaxed-math", NULL, NULL);
	if (ciErrNum != CL_SUCCESS)
	{
		// write out standard error, Build Log and PTX, then cleanup and exit
		shrLogEx(LOGBOTH | ERRORMSG, ciErrNum, STDERROR);
		oclLogBuildInfo(cpProgram, oclGetFirstDev(cxGPUContext));
		oclLogPtx(cpProgram, oclGetFirstDev(cxGPUContext), "at_clgl.ptx");
		Cleanup(EXIT_FAILURE);
	}

	// create the kernel
	ckKernel = clCreateKernel(cpProgram, "part_proc", &ciErrNum);
	shrCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);

	// create VBOs (position and color)
	id_vbo_pos = createVBO(&vbo_position, (maxParticles * 4 * sizeof(float)));
	id_vbo_color = createVBO(&vbo_color, (maxParticles * 4 * sizeof(float)));
	id_vbo_pos_vel = createVBO(&vbo_position_vel, (maxParticles * 4 * sizeof(float)));
	id_vbo_color_vel = createVBO(&vbo_color_vel, (maxParticles * 4 * sizeof(float)));
	id_vbo_part = createVBO(&vbo_part, (maxParticles * 4 * sizeof(Particle)));

	// set the args values 
	/*ciErrNum = clSetKernelArg(ckKernel, 0, sizeof(cl_mem), (void *)&vbo_position);
	ciErrNum |= clSetKernelArg(ckKernel, 1, sizeof(cl_mem), (void *)&vbo_color);
	ciErrNum |= clSetKernelArg(ckKernel, 2, sizeof(cl_mem), (void *)&vbo_position_vel);
	ciErrNum |= clSetKernelArg(ckKernel, 3, sizeof(cl_mem), (void *)&vbo_color_vel);
	ciErrNum |= clSetKernelArg(ckKernel, 4, sizeof(float), &GravityX);
	ciErrNum |= clSetKernelArg(ckKernel, 5, sizeof(float), &GravityY);*/
	ciErrNum = clSetKernelArg(ckKernel, 0, sizeof(cl_mem), (void *)&vbo_part);
	ciErrNum |= clSetKernelArg(ckKernel, 1, sizeof(float), &GravityX);
	ciErrNum |= clSetKernelArg(ckKernel, 2, sizeof(float), &GravityY);
	shrCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);

	// init timer 1 for fps measurement 
	shrDeltaT(1);

	// Initialize the emitter
	emitter = new APEmitter();
}

// Create VBO
//*****************************************************************************
GLuint AtomicParticles::createVBO(cl_mem* vbo, unsigned int size)
{
	// create buffer object
	GLuint id = 0;
	glGenBuffers(1, &id);
	glBindBuffer(GL_ARRAY_BUFFER, id);

	// initialize buffer object
	glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);

	// create OpenCL buffer from GL VBO
	*vbo = clCreateFromGLBuffer(cxGPUContext, CL_MEM_WRITE_ONLY, id, NULL);

	// check for errors and return
	shrCheckErrorEX(ciErrNum, CL_SUCCESS, pCleanup);
	return id;
}
/*
GLuint createVBO(const void* data, int dataSize, GLenum target, GLenum usage)
{
GLuint id = 0; // 0 is reserved, glGenBuffersARB() will return non-zero id if success
glGenBuffers(1, &id); // create a vbo
glBindBuffer(target, id); // activate vbo id to use
glBufferData(target, dataSize, data, usage); // upload data to video card
// check data size in VBO is same as input array, if not return 0 and delete VBO
int bufferSize = 0;
glGetBufferParameteriv(target, GL_BUFFER_SIZE, &bufferSize);
if (dataSize != bufferSize)
{
glDeleteBuffers(1, &id);
id = 0;
//cout << "[createVBO()] Data size is mismatch with input array\n";
printf("[createVBO()] Data size is mismatch with input array\n");
}
//this was important for working inside blender!
glBindBuffer(target, 0);
return id; // return VBO id
}*/

// Check for some necessary extensions
//*****************************************************************************
void AtomicParticles::CheckOpenGl()
{
	GLboolean bGLEW = glewIsSupported("GL_VERSION_2_0 GL_ARB_pixel_buffer_object");
	shrCheckErrorEX(bGLEW, shrTRUE, pCleanup);
}

// Returns the maximum number of particles allowed in the system.
//*****************************************************************************
unsigned int AtomicParticles::GetMaxParticles()
{
	return maxParticles;
}

// Function to clean up and exit
//*****************************************************************************
void AtomicParticles::Cleanup(int iExitCode)
{
	// Cleanup allocated objects
	shrLog("\nStarting Cleanup...\n\n");
	if (ckKernel)       clReleaseKernel(ckKernel);
	if (cpProgram)      clReleaseProgram(cpProgram);
	if (cqCommandQueue) clReleaseCommandQueue(cqCommandQueue);
	
	// clean vbos
	if (id_vbo_pos)
	{
		glBindBuffer(1, id_vbo_pos);
		glDeleteBuffers(1, &id_vbo_pos);
		id_vbo_pos = 0;
	}
	if (vbo_position)clReleaseMemObject(vbo_position);
	
	if (id_vbo_color)
	{
		glBindBuffer(1, id_vbo_color);
		glDeleteBuffers(1, &id_vbo_color);
		id_vbo_color = 0;
	}
	if (vbo_color)clReleaseMemObject(vbo_color);

	if (id_vbo_pos_vel)
	{
		glBindBuffer(1, id_vbo_pos_vel);
		glDeleteBuffers(1, &id_vbo_pos_vel);
		id_vbo_pos_vel = 0;
	}
	if (vbo_position_vel)clReleaseMemObject(vbo_position_vel);

	if (id_vbo_color_vel)
	{
		glBindBuffer(1, id_vbo_color_vel);
		glDeleteBuffers(1, &id_vbo_color_vel);
		id_vbo_color_vel = 0;
	}
	if (vbo_color_vel)clReleaseMemObject(vbo_color_vel);
	
	// clean contexts, devices, etc
	if (cxGPUContext)clReleaseContext(cxGPUContext);
	if (cPathAndName)free(cPathAndName);
	if (cSourceCL)free(cSourceCL);
	if (cdDevices)delete(cdDevices);

	// finalize logs and leave
	//shrQAFinish2(bQATest, *pArgc, (const char **)pArgv, (iExitCode == 0) ? QA_PASSED : QA_FAILED);
	shrLogEx(LOGBOTH | CLOSELOG, 0, "%s Exiting...\nPress <Enter> to Quit\n", cExecutableName);
#ifdef WIN32
	getchar();
#endif

	// exit
	exit(iExitCode);
}
void Cleanup(int iExitCode)
{
	AtomicParticles::Cleanup(iExitCode);
}