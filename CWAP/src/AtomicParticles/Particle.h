#ifndef PARTICLE_H
#define PARTICLE_H

// Packed particle struct
#pragma pack(push, 1)
struct Particle 
{
	float pos[4];
	float color[4];
	float pos_vel[4];
	float color_vel[4];
};
#pragma pack(pop)

#endif