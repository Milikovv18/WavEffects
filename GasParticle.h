#pragma once

#include <corecrt_math_defines.h>

#include "Vec2.h"

#define POW_2(x) ((x)*(x))
#define POW_3(x) ((x)*(x)*(x))


namespace particle_const
{
	// Solver parameters
	constexpr float REST_DENS = 1000.0f; // Rest density
	constexpr float GAS_CONST = 2000.0f; // Const for equation of state
	constexpr float MASS = 105.0f; // Assume all particles have the same mass
	constexpr float VISC = 50.0f; // Viscosity constant

	constexpr float H = 3.0f; // Kernel radius
	constexpr float HSQ = H * H; // Radius^2 for optimization

	// Smoothing kernels defined in Muller and their gradients
	constexpr float POLY6 = 315.0f / (65.0f * (float)M_PI * POW_3(POW_3(H)));
	constexpr float SPIKY_GRAD = -45.0f / ((float)M_PI * POW_3(POW_2(H)));
	constexpr float VISC_LAP = 45.0f / ((float)M_PI * POW_3(POW_2(H)));

	// Simulation parameters
	// How close a particle should be to the wall to delete it
	constexpr float EPS = 1.0f; // Boundary epsilon
}


// A fire particle, that has a fire color
// and decays with the time (becames gray)
struct GasParticle
{
	GasParticle() :
		m_vel{ 0.0f, 0.0f }, m_force{ 0.0f, 0.0f },
		m_dens(0.0f), m_pres(0.0f), m_decay(0.0f) {}

	// External (gravitational) forces
	inline static float gravity = -100'000.0f;
	// Integration timestep (not taking into account the speed of CPU)
	inline static float DT = 0.0008f;

	// Position, velocity and forces, acting to the particle
	Vec2* m_pos = nullptr, m_vel, m_force;
	// Density, pressure and decay time of a particle
	float m_dens, m_pres, m_decay;
};
