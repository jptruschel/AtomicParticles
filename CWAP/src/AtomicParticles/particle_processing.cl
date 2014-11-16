/*
 * University of Manchester - Advanced Computer Graphics Coursework
 *
 *		Particle System: Atomic Particles
 *		Student: Joao Paulo T Ruschel (9571945)
 *		Professors: Toby Howard; Steve Pettifer
 *
 *	This is the code that is runnable on the GPU. It is written in a C-like
 *	programming language. For more information, refer to Kronos docs at:
 *		https://www.khronos.org/registry/cl/sdk/2.0/docs/man/xhtml/
 */

 struct __attribute__ ((packed)) Particle {
	float4 pos;
	float4 color;
	float4 pos_vel;
	float4 color_vel;
 };

///////////////////////////////////////////////////////////////////////////////
// Kernel used to process each particle.
// Here is where the particles get updated.
///////////////////////////////////////////////////////////////////////////////
__kernel void part_proc(
__global struct Particle* particles,
float gravityX, 
float gravityY, 
float time)
{
    unsigned int n = get_global_id(0);

    // calculate uv coordinates
    float u = n / (float) 512;
    float v = n / (float) 512;
    u = u*8.0f - 4.0f;
    v = v*8.0f - 4.0f;

    // calculate simple sine wave pattern
    float freq = 4.0f;
    float w = sin(n*freq + time) * cos(n*freq + time) * 2.0f;

    // write output vertex
    //pos[y*width+x] = (float4)(u, w, v, 1.0f);
	particles[n].pos = (float4)((float)(n*0.001f), w, sin(n*freq + time), 1.0f);
	particles[n].color = (float4)((sin(n*freq + time)+1.5f)*0.25f, (cos(n*freq + time)+1.5f)*0.25f, (tan(n*freq + time)+1.5f)*0.25f, 0.8f);
	//color[n] = (float4)(0.0f, 0.0f, 1.0f, 1.0f);
}

