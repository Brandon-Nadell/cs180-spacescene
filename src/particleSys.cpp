#define GLM_ENABLE_EXPERIMENTAL
#include <iostream>
#include <algorithm>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include "particleSys.h"
#include "GLSL.h"
#include <glm/gtc/random.hpp>



particleSys::particleSys(vec3 source, int textureIndex, int numP, float radius, vec3 bias, vec3 vMax, vec3 c, vec2 life, float scale) {

  this->textureIndex = textureIndex;
	this->numP = std::min(300, numP);
  this->scale = scale;
	t = 0.0f;
	h = 0.01f;
	g = vec3(0.0f, -0.098, 0.0f);
	start = source;
  this->radius = radius;
  this->bias = bias;
  this->vMax = vMax;
  this->c = c;
  this->life = life;
	theCamera = glm::mat4(1.0);
}

void particleSys::gpuSetup() {

  // cout << "start: " << start.x << " " << start.y << " " <<start.z << endl;
	for (int i=0; i < numP; i++) {
		points[i*3+0] = start.x;
		points[i*3+1] = start.y;
		points[i*3+2] = start.z;

		auto particle = make_shared<Particle>(start);
		particles.push_back(particle);
		particle->load(start, radius, bias, vMax, c, life);
	}

	//generate the VAO
   glGenVertexArrays(1, &vertArrObj);
   glBindVertexArray(vertArrObj);
   //generate vertex buffer to hand off to OGL - using instancing
   glGenBuffers(1, &vertBuffObj);
   //set the current state to focus on our vertex buffer
   glBindBuffer(GL_ARRAY_BUFFER, vertBuffObj);
   //actually memcopy the data - only do this once
   glBufferData(GL_ARRAY_BUFFER, sizeof(points), &points[0], GL_STREAM_DRAW);

   glGenBuffers(1, &colorbuffer);
   glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(pointColors), &pointColors[0], GL_STREAM_DRAW);
   
   assert(glGetError() == GL_NO_ERROR);
	
}

void particleSys::reSet() {
  cout << "reset" << endl;
	for (int i=0; i < numP; i++) {
		particles[i]->load(start, radius, bias, vMax, c, life);
	}
}

void particleSys::drawMe(std::shared_ptr<Program> prog) {

 	glBindVertexArray(vertArrObj);
	int h_pos = prog->getAttribute("vertPos");
  GLSL::enableVertexAttribArray(h_pos);
	int h_col = prog->getAttribute("vertColor");
  GLSL::enableVertexAttribArray(h_col);
  glBindBuffer(GL_ARRAY_BUFFER, vertBuffObj);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribDivisor(0, 1);
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glVertexAttribDivisor(1, 1);

  glDisable(GL_CULL_FACE);
  glDrawArraysInstanced(GL_POINTS, 0, 1, numP);

  glVertexAttribDivisor(0, 0);
  glVertexAttribDivisor(1, 0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

}

bool particleSys::isDone() {
  return doneParticles.size() == numP;
}

void particleSys::lock(vec3 pos) {
  for (vector<shared_ptr<Particle> >::iterator i = particles.begin(); i != particles.end(); i++) {
      (*i)->lock(t, pos);
  }
  start = pos;
}

void particleSys::update() {

  vec3 pos;
  vec4 col;

  //update the particles
  for (vector<shared_ptr<Particle> >::iterator i = particles.begin(); i != particles.end(); i++) {
      (*i)->update(t, h, g, start);
      if ((*i)->done) {
        doneParticles.insert(*i);
      }
  }
  t += h;
 
  // Sort the particles by Z
  // temp->rotate(camRot, vec3(0, 1, 0));
  //be sure that camera matrix is updated prior to this update
  vec3 s, t, sk;
  vec4 p;
  quat r;
  glm::decompose(theCamera, s, r, t, sk, p);
  sorter.C = glm::toMat4(r); 
  sort(particles.begin(), particles.end(), sorter);


  //go through all the particles and update the CPU buffer
   for (int i = 0; i < numP; i++) {
        pos = particles[i]->getPosition();
        col = particles[i]->getColor();
        points[i*3+0] =pos.x; 
        points[i*3+1] =pos.y; 
        points[i*3+2] =pos.z; 
			  // To do - how can you integrate unique colors per particle?
        pointColors[i*4+0] = col.r;
        pointColors[i*4+1] = col.g;
        pointColors[i*4+2] = col.b;
        pointColors[i*4+3] = col.a;
  } 

  //update the GPU data
   glBindBuffer(GL_ARRAY_BUFFER, vertBuffObj);
   glBufferData(GL_ARRAY_BUFFER, sizeof(points), NULL, GL_STREAM_DRAW);
   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*numP*3, points);
   glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(pointColors), NULL, GL_STREAM_DRAW);
   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*numP*4, pointColors);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

}
