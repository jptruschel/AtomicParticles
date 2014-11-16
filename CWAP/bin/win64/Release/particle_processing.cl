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

///////////////////////////////////////////////////////////////////////////////
// Kernel used to process each particle.
// Here is where the particles get updated.
///////////////////////////////////////////////////////////////////////////////
__kernel void part_proc(__global float4* pos, __global float4* color, unsigned int width, unsigned int height, float time)
{
    unsigned int n = get_global_id(0);

    // calculate uv coordinates
    float u = n / (float) width;
    float v = n / (float) height;
    u = u*8.0f - 4.0f;
    v = v*8.0f - 4.0f;

    // calculate simple sine wave pattern
    float freq = 4.0f;
    float w = sin(n*freq + time) * cos(n*freq + time) * 2.0f;

    // write output vertex
    //pos[y*width+x] = (float4)(u, w, v, 1.0f);
	pos[n] = (float4)((float)(n*0.001f), w, sin(n*freq + time), 0.0f);
	color[n] = (float4)((sin(n*freq + time)+1.0f)*0.5f, (cos(n*freq + time)+1.0f)*0.5f, (tan(n*freq + time)+1.0f)*0.5f, 0.8f);
}

