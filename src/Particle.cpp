///
// sueda - geometry edits Z. Wood
// 3/16
//

#include <iostream>
#include "Particle.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Texture.h"
#include <glm/gtc/random.hpp>


float randFloat(float l, float h)
{
	float r = rand() / (float) RAND_MAX;
	return (1.0f - r) * l + r * h;
}

Particle::Particle(vec3 start) :
	charge(1.0f),
	m(1.0f),
	d(0.0f),
	x(start),
	v(0.0f, 0.0f, 0.0f),
	lifespan(1.0f),
	tEnd(0.0f),
	scale(1.0f),
	color(1.0f, 1.0f, 1.0f, 1.0f)
{
}

Particle::~Particle()
{
}

void Particle::load(vec3 start, float radius, vec3 bias, vec3 vMax, vec3 c, vec2 life)
{
	// Random initialization
	rebirth(0.0f, start, radius, bias, vMax, c, life);
}

/* all particles born at the origin */
void Particle::rebirth(float t, vec3 start, float radius, vec3 bias, vec3 vMax, vec3 c, vec2 life)
{
	charge = randFloat(0.0f, 1.0f) < 0.5 ? -1.0f : 1.0f;	
	m = 1.0f;
  	d = randFloat(0.0f, 0.02f);
	x = start + (radius == 0 ? vec3(0, 0, 0) : glm::ballRand(radius));
	v.x = bias.x + randFloat(-vMax.x, vMax.x);
	v.y = bias.y + randFloat(-vMax.y, vMax.y);
	v.z = bias.z + randFloat(-vMax.z, vMax.z);
	lifespan = life.s + randFloat(0.0f, life.t);
	tEnd = t + lifespan;
	scale = randFloat(0.2, 1.0f);
	color.r = c.r + randFloat(-.1f, .1f);
   	color.g = c.g + randFloat(-.1f, .1f);
   	color.b = c.b + randFloat(-.1f, .1f);
	color.a = 1.0f;
}

void Particle::lock(float t, vec3 pos) {
	x = pos;
	color.a = 1.0f;
}

void Particle::update(float t, float h, const vec3 &g, const vec3 start)
{
	if (t > tEnd) {
		done = true;
	}
	color.a = (tEnd-t)/lifespan;
	x += h*v*(float)std::pow(color.a*2, 2);
}
