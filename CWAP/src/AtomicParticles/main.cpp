/*
* University of Manchester - Advanced Computer Graphics Coursework
*
*		Particle System: Atomic Particles
*		Student: Joao Paulo T Ruschel (9571945)
*		Professors: Toby Howard; Steve Pettifer
*
*	The code for the particles, particle system, and OpenCL kernels were
*	written by me (the student).
*	OpenCL intialization, logging and clean-up functions were used from the
*	examples provided by NVIDIA, and are protected by copyrights.
*	By no means do I wish to pass them as my own, nor to take credit for have
*	written any of it.
*	The examples can be downloaded from:
*		https://developer.nvidia.com/opencl
*	From those samples were also used the mouse motion methods.
*
*	The class AtomicParticles handles all the OpenCL calls and must be called
*	properly. For more information, refer to AtomicParticles.h
*	This file only handles some OpenGL-related callbacks and creates the test
*	environment.
*/

#ifndef _USINGORIGINAL

#include "AtomicParticles.h"

// Constants, defines, typedefs and global declarations
//*****************************************************************************
#define REFRESH_DELAY	  0 //ms

// Rendering window vars
const unsigned int window_width = 1024;
const unsigned int window_height = 768;

// Default number of particles
unsigned int maxParticles = 10000000;

// view, GLUT and display params
int ox, oy;
int buttonState = 0;
float camera_trans_original[] = { 0, 0, -3 };
float camera_rot_original[] = { 0, 0, 0 };
float camera_trans_lag_original[] = { 0, 0, -3 };
float camera_rot_lag_original[] = { 0, 0, 0 };
float camera_trans[] = { camera_trans_original[0], camera_trans_original[1], camera_trans_original[2] };
float camera_rot[] = { camera_rot_original[0], camera_rot_original[1], camera_rot_original[2] };
float camera_trans_lag[] = { camera_trans_lag_original[0], camera_trans_lag_original[1], camera_trans_lag_original[2] };
float camera_rot_lag[] = { camera_rot_lag_original[0], camera_rot_lag_original[1], camera_rot_lag_original[2] };
const float inertia = 0.1f;

// Editor options
//*****************************************************************************
int vsync = 1;

// Program arguments
int *pArgc = NULL;
char **pArgv = NULL;

// Display list for coordinate axis
GLuint axisList;
int AXIS_SIZE = 200;
int axisEnabled = 1;

// Forward Function declarations
//*****************************************************************************
// OpenCL functionality
void runKernel();
void saveResultOpenCL(int argc, const char** argv, const GLuint& vbo);

// GL functionality
void InitGL(int* argc, char** argv);
void createVBO(GLuint* vbo);
void DisplayGL();
void KeyboardGL(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void timerEvent(int value);
void drawString(void * font, char *s, float x, float y);
void updateViewTransform();
void resetView();
void makeAxes();
void setVSync(bool sync);

// Main program
//*****************************************************************************
int main(int argc, char** argv)
{
	// First of all, initialize OpenGL
	shrLog("Initializing ... \n  .OpenGL ... ");
	InitGL(&argc, argv);

	// Set vsync
	setVSync(vsync);

	// Initialize Atomic Particles
	shrLog("Done.\n  .Atomic Particles ... \n");
	AtomicParticles::Initialize(argc, argv, maxParticles);

	// Set color buffer
	glBindBuffer(GL_ARRAY_BUFFER, AtomicParticles::id_vbo_color);
	glColorPointer(4, GL_FLOAT, 0, 0);

	// Set position buffer
	glBindBuffer(GL_ARRAY_BUFFER, AtomicParticles::id_vbo_pos);
	glVertexPointer(3, GL_FLOAT, 4 * sizeof(float), 0);

	// Start main GLUT rendering loop for processing and rendering, 
	// or otherwise run No-GL Q/A test sequence
	shrLog("Done. \n\nInitialize GL loop ... \n\n");
	glutMainLoop();

	// Normally unused return path
	AtomicParticles::Cleanup(EXIT_SUCCESS);
}

// Initialize GL
//*****************************************************************************
void InitGL(int* argc, char** argv)
{
	// initialize GLUT 
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowPosition(glutGet(GLUT_SCREEN_WIDTH) / 2 - window_width / 2,
		glutGet(GLUT_SCREEN_HEIGHT) / 2 - window_height / 2);
	glutInitWindowSize(window_width, window_height);
	AtomicParticles::iGLUTWindowHandle = glutCreateWindow("Atomic Particles");
#if !(defined (__APPLE__) || defined(MACOSX))
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
#endif

	// register GLUT callback functions
	glutDisplayFunc(DisplayGL);
	glutKeyboardFunc(KeyboardGL);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutTimerFunc(REFRESH_DELAY, timerEvent, 0);

	// initialize necessary OpenGL extensions
	glewInit();
	AtomicParticles::CheckOpenGl();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POINT_SMOOTH);

	// default initialization
	glClearColor(0.0, 0.0, 0.0, 1.0);
	//glDisable(GL_DEPTH_TEST);

	// viewport
	glViewport(0, 0, window_width, window_height);

	// projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (GLfloat)window_width / (GLfloat)window_height, 0.1, 1000.0);

	// set view matrix
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	updateViewTransform();

	// Initialize axes
	makeAxes();

	return;
}

// Display callback
//*****************************************************************************
void DisplayGL()
{
	// Clear
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Render the particles
	AtomicParticles::emitter->display();

	// If enabled, draw coordinate axis
	if (axisEnabled) glCallList(axisList);

	// Render debug info
	glColor4f(0.1f, 1.0f, 0.1f, 1.0f);
	char buffer[5][50];
	sprintf(buffer[0], "Debug info: ", vsync);
	sprintf(buffer[1], " vsync: %i", vsync);

	drawString(GLUT_BITMAP_HELVETICA_12, buffer[0], -(window_width * 0.0073f), (window_height * 0.007f));
	drawString(GLUT_BITMAP_HELVETICA_12, buffer[1], -(window_width * 0.0073f), (window_height * 0.0067f));

	// Flip backbuffer to screen
	glutSwapBuffers();
}

// Calls glutPostRedisplay() every REFRESH_DELAY miliseconds
//*****************************************************************************
void timerEvent(int value)
{
	glutPostRedisplay();
	glutTimerFunc(REFRESH_DELAY, timerEvent, 0);
}

// Keyboard events handle rvery fire
//*****************************************************************************
void KeyboardGL(unsigned char key, int x, int y)
{
	switch (key)
	{
	case '\033': // escape quits
	case '\015': // Enter quits    
	case 'Q':    // Q quits
	case 'q':    // q (or escape) quits
		// Cleanup up and quit
		//bNoPrompt = shrTRUE;
		AtomicParticles::Cleanup(EXIT_SUCCESS);
		break;
		// Toggle vSync
	case 'v':
		vsync = !vsync;
		setVSync(vsync);
		break;
		// Reset view
	case 'r':
		resetView();
		updateViewTransform();
		break;
	case 'e':
		AtomicParticles::emitter->emit(50);
		break;
	}
}

// Draws a string on the screen (screen-space)
//*****************************************************************************
void drawString(void * font, char *s, float x, float y)
{
	// Reset view to default
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRasterPos3f(x, y, -10.0f);

	// Renders all characters
	unsigned int i;
	for (i = 0; i < strlen(s); i++)
		glutBitmapCharacter(font, s[i]);

	// Update view back to the desired
	updateViewTransform();
}

// Reset the view to the intial position
//*****************************************************************************
void resetView()
{
	camera_trans[0] = camera_trans_original[0];
	camera_trans[1] = camera_trans_original[1];
	camera_trans[2] = camera_trans_original[2];
	camera_rot[0] = camera_rot_original[0];
	camera_rot[1] = camera_rot_original[1];
	camera_rot[2] = camera_rot_original[2];
	camera_trans_lag[0] = camera_trans_lag_original[0];
	camera_trans_lag[1] = camera_trans_lag_original[1];
	camera_trans_lag[2] = camera_trans_lag_original[2];
	camera_rot_lag[0] = camera_rot_lag_original[0];
	camera_rot_lag[1] = camera_rot_lag_original[1];
	camera_rot_lag[2] = camera_rot_lag_original[2];
}

// Handler for GLUT Mouse events
//*****************************************************************************
void mouse(int button, int state, int x, int y)
{
	int mods;

	if (state == GLUT_DOWN)
		buttonState |= 1 << button;
	else if (state == GLUT_UP)
		buttonState = 0;

	mods = glutGetModifiers();
	if (mods & GLUT_ACTIVE_SHIFT)
	{
		buttonState = 2;
	}
	else if (mods & GLUT_ACTIVE_CTRL)
	{
		buttonState = 3;
	}

	ox = x;
	oy = y;
}

// GLUT mouse motion callback
//*****************************************************************************
void motion(int x, int y)
{
	float dx, dy;
	dx = (float)(x - ox);
	dy = (float)(y - oy);

	if (buttonState == 3)
	{
		// left+middle = zoom
		camera_trans[2] += (dy / 100.0f) * 0.5f * fabs(camera_trans[2]);
	}
	else if (buttonState & 2)
	{
		// middle = translate
		camera_trans[0] += dx / 500.0f;
		camera_trans[1] -= dy / 500.0f;
	}
	else if (buttonState & 1)
	{
		// left = rotate
		camera_rot[0] += dy / 5.0f;
		camera_rot[1] += dx / 5.0f;
	}

	ox = x;
	oy = y;

	// update view transform based upon mouse inputs
	updateViewTransform();
}

// GLUT key event handler
//*****************************************************************************
void updateViewTransform()
{
	// Set view transform
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	for (int c = 0; c < 3; ++c)
	{
		camera_trans_lag[c] += (camera_trans[c] - camera_trans_lag[c]) * inertia;
		camera_rot_lag[c] += (camera_rot[c] - camera_rot_lag[c]) * inertia;
	}
	glTranslatef(camera_trans_lag[0], camera_trans_lag[1], camera_trans_lag[2]);
	glRotatef(camera_rot_lag[0], 1.0, 0.0, 0.0);
	glRotatef(camera_rot_lag[1], 0.0, 1.0, 0.0);
}

// Initialize a render list for axes
//*****************************************************************************
void makeAxes()
{
	// Create a display list for drawing coord axis
	axisList = glGenLists(1);
	glNewList(axisList, GL_COMPILE);
	glLineWidth(1.0);
	glBegin(GL_LINES);
	glColor4f(1.0, 0.0, 0.0, 0.6f);       // X axis - red
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(AXIS_SIZE, 0.0, 0.0);

	glColor4f(0.0, 1.0, 0.0, 0.6f);       // Y axis - green
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, AXIS_SIZE, 0.0);

	glColor4f(0.0, 0.0, 1.0, 0.6f);       // Z axis - blue
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, 0.0, AXIS_SIZE);
	glEnd();
	glEndList();
}

// Calls some weird functions and toggles vsync.
// Taken from: http://www.3dbuzz.com/forum/threads/188320-Enable-or-Disable-VSync-in-OpenGL
//*****************************************************************************
void setVSync(bool sync)
{
	// Function pointer for the wgl extention function we need to enable/disable
	// vsync
	typedef BOOL(APIENTRY *PFNWGLSWAPINTERVALPROC)(int);
	PFNWGLSWAPINTERVALPROC wglSwapIntervalEXT = 0;

	const char *extensions = (char*)glGetString(GL_EXTENSIONS);

	if (strstr(extensions, "WGL_EXT_swap_control") == 0)
	{
		return;
	}
	else
	{
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALPROC)wglGetProcAddress("wglSwapIntervalEXT");

		if (wglSwapIntervalEXT)
			wglSwapIntervalEXT(sync);
	}
}

#endif