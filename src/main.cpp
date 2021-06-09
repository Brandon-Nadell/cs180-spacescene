#include <iostream>
#include <glad/glad.h>
#include <cmath>
#include <vector>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Texture.h"
#include "stb_image.h"
#include "particleSys.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>

using namespace std;
using namespace glm;
unsigned int cubeMapTexture;
int WIDTH = 1280, HEIGHT = 960;

const float PI = 3.14159265358979;

class Application : public EventCallbacks
{

public:

	struct Asteroid {
		float radius;
		float startAngle;
		float rotationSpeed;
		float revolutionSpeed;
		float size;

		Asteroid(float radius, float startAngle, float rotationSpeed, float revolutionSpeed, float size) {
			this->radius = radius;
			this->startAngle = startAngle;
			this->rotationSpeed = rotationSpeed;
			this->revolutionSpeed = revolutionSpeed;
			this->size = size;
		}
	};

	struct Moon {
		vec3 position;
		vec3 absolutePosition;
		int material;
		float rotationSpeed;
		float revolutionSpeed;

		vec3 escapeDirection;
		float speed;
		Moon(vec3 position, int material, float rotationSpeed, float revolutionSpeed) {
			this->position = position;
			this->material = material;
			this->rotationSpeed = rotationSpeed;
			this->revolutionSpeed = revolutionSpeed;
			escapeDirection = vec3(0, 0, 0);
			speed = 0;
		}

		void escape(vec3 cog) {
			speed = revolutionSpeed/60.0f * length(position);
			position = absolutePosition;
			vec3 centripetal = position - cog;
			escapeDirection = vec3(centripetal.x * cos(-PI/2) - centripetal.z * sin(-PI/2), 0, centripetal.x * sin(-PI/2) + centripetal.z * cos(-PI/2));
		}

		void update() {
			position += escapeDirection * speed/3.0f;
		}
	};

	struct Planet {
		vec3 position;
		int material;
		float rotationSpeed;
		float revolutionSpeed;
		vector<Moon> moons;
		Planet(vec3 position, int material, float rotationSpeed, float revolutionSpeed) {
			this->position = position;
			this->material = material;
			this->rotationSpeed = rotationSpeed;
			this->revolutionSpeed = revolutionSpeed;
		}
	};

	struct Rocket {
		vec3 position;
		vec3 direction;
		vec3 rotation;
		float speed;
		float life;
		float lifeEnd;
		Rocket(vec3 position, vec3 direction, vec3 rotation) {
			this->position = position;
			this->direction = normalize(direction);
			this->rotation = rotation;
			speed = .5f;
			life = 0;
			lifeEnd = 240;
		}

		void update() {
			position += direction * speed;
			life++;
		}
	};

	WindowManager * windowManager = nullptr;

	// Shader programs
	std::shared_ptr<Program> prog;
	std::shared_ptr<Program> blurProg;
	std::shared_ptr<Program> cubeProg;
	std::shared_ptr<Program> texProg;
	std::shared_ptr<Program> texProgNoLighting;

	// Meshes
	vector<pair<vector<shared_ptr<Shape> >, float> > meshes;
	shared_ptr<Shape> cube;

	//the image to use as a texture (ground)
	vector<Planet> planets;
	vector<Asteroid> asteroids;
	vector<Moon> looseMoons;
	vector<Rocket> rockets;
	vector<shared_ptr<Texture> > planetTextures;
	vector<shared_ptr<Texture> > shipTextures;
	shared_ptr<Texture> sun;
	shared_ptr<Texture> rocket;

	// Animations
	float planetRotation = 0;
	float sunRadius = 100.0f;
	float ufoRotation = 0;
	int ufoTimer = 0;
	Planet* ufoDst;
	Planet* ufoSrc;
	int matIndex = 5;

	// Controls
	vec3 position = vec3(0, 0, 20);
	vec3 lookAt = vec3(0, 1, -4);
	bool wKey = false;
	bool sKey = false;
	bool aKey = false;
	bool dKey = false;
	float maxSpeed = .35f;
	float speed = 0;
	bool boosters = false;
	vec3 move = vec3(0, 0, 0);
	float friction = .01f;
	float lookPhi = 0;
	float lookTheta = -PI/2;
	float tilt = 0;
	float uptilt = 0;

	// Particles
	std::shared_ptr<Program> partProg;
	vector<shared_ptr<particleSys> > particleSystems;
	vector<shared_ptr<Texture> > particleTextures;

	// Worm Hole
	shared_ptr<MatrixStack> above = make_shared<MatrixStack>();
	glm::mat4 wormHoleView;
	bool fromShip;
	unsigned int fbo;
	unsigned int rbo;
	unsigned int textureColorbuffer;
	int fboRes = 4;
	vec3 wormHoleSrc = vec3(0, 0, 250);
	vec3 wormHoleDst = vec3(100, 0, 0);
	float fov = 180.0f - 18.72f;

	void expandSun() {
		sunRadius += 10;
		createParticles(vec3(0, 0, 0), 0, 1, 0, vec3(0, 0, 0), vec3(0, 0, 0), vec3(1.0f, 0.7f, 0.0f), vec2(100000, 0), 65.0f*sunRadius/100.0f);
		particleSystems[0] = particleSystems[particleSystems.size() - 1];
		particleSystems.erase(particleSystems.end()-1);
	}

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
		if (key == GLFW_KEY_R && action == GLFW_PRESS) {
			rockets.push_back(Rocket(position + 1.5f*normalize(lookAt - position), lookAt - position, vec3(glm::clamp(2*tilt, -PI/4, PI/4), -lookTheta+PI, -lookPhi + uptilt)));
		}
		if (key == GLFW_KEY_M && action == GLFW_PRESS){
			matIndex++;
		}
		if (key == GLFW_KEY_W) {
			if (action == GLFW_PRESS) {
				wKey = true;
			} else if (action == GLFW_RELEASE) {
				wKey = false;
			}
		}
		if (key == GLFW_KEY_S) {
			if (action == GLFW_PRESS) {
				sKey = true;
			} else if (action == GLFW_RELEASE) {
				sKey = false;
			}
		}
		if (key == GLFW_KEY_A) {
			if (action == GLFW_PRESS) {
				aKey = true;
			} else if (action == GLFW_RELEASE) {
				aKey = false;
			}
		}
		if (key == GLFW_KEY_D) {
			if (action == GLFW_PRESS) {
				dKey = true;
			} else if (action == GLFW_RELEASE) {
				dKey = false;
			}
		}
		if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_LEFT_SHIFT) {
			if (action == GLFW_PRESS) {
				boosters = true;
			} else if (action == GLFW_RELEASE) {
				boosters = false;
			}
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			glfwGetCursorPos(window, &posX, &posY);
			cout << "Pos X " << posX <<  " Pos Y " << posY << endl;
		}
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		WIDTH = width;
		HEIGHT = height;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WIDTH, HEIGHT);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH*2*fboRes, HEIGHT*2*fboRes, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WIDTH*2*fboRes, HEIGHT*2*fboRes);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	}

	void scrollCallback(GLFWwindow *window, double deltaX, double deltaY)
	{
		// uncomment for natural scrolling
		// deltaX = -deltaX;
		// deltaY = -deltaY;

		tilt = clamp<float>(tilt + deltaX/20, -PI/8, PI/8);
		uptilt = clamp<float>(uptilt + deltaY/20, -PI/8, PI/8);
		lookTheta -= deltaX/20;
		lookPhi += deltaY/20;
		lookPhi = clamp<float>(lookPhi, -PI*89.999/180, PI*89.99/180);
	}

	bool nearOtherPlanets(vec2 p, float distance) {
		for (int i = 0; i < planets.size(); i++) {
			if (glm::distance(p, vec2(planets[i].position.x, planets[i].position.z)) < distance) {
				return true;
			}
		}
		return false;
	}

	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();
		glClearColor(.2f, 0, 0, 1.0f);

		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("MatAmb");
		prog->addUniform("MatDif");
		prog->addUniform("MatSpec");
		prog->addUniform("MatShine");
		prog->addUniform("lightPos");
		prog->addUniform("lightPos2");
		prog->addUniform("lightPos3");
		prog->addUniform("camPos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");

		blurProg = make_shared<Program>();
		blurProg->setVerbose(true);
		blurProg->setShaderNames(resourceDirectory + "/blur_vert.glsl", resourceDirectory + "/blur_frag.glsl");
		blurProg->init();
		blurProg->addAttribute("aPos");
		blurProg->addAttribute("aTexCoords");

		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
		texProg->init();
		texProg->addUniform("P");
		texProg->addUniform("V");
		texProg->addUniform("M");
		texProg->addUniform("lightPos");
		texProg->addUniform("lightPos2");
		texProg->addUniform("lightPos3");
		texProg->addUniform("camPos");
		texProg->addUniform("Texture0");
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex");
		
		texProgNoLighting = make_shared<Program>();
		texProgNoLighting->setVerbose(true);
		texProgNoLighting->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag1.glsl");
		texProgNoLighting->init();
		texProgNoLighting->addUniform("P");
		texProgNoLighting->addUniform("V");
		texProgNoLighting->addUniform("M");
		texProgNoLighting->addUniform("lightPos");
		texProgNoLighting->addUniform("camPos");
		texProgNoLighting->addUniform("Texture0");
		texProgNoLighting->addAttribute("vertPos");
		texProgNoLighting->addAttribute("vertNor");
		texProgNoLighting->addAttribute("vertTex");

		cubeProg = make_shared<Program>();
		cubeProg->setVerbose(true);
		cubeProg->setShaderNames(resourceDirectory + "/cube_vert.glsl", resourceDirectory + "/cube_frag.glsl");
		cubeProg->init();
		cubeProg->addUniform("P");
		cubeProg->addUniform("V");
		cubeProg->addUniform("M");
		cubeProg->addUniform("skybox");
		cubeProg->addAttribute("vertPos");
		cubeProg->addAttribute("vertNor");
		
		string directs[] = {
			"earth.jpg",
			"jupiter.jpg",
			"mars.jpg",
			"mercury.jpg",
			"neptune.jpg",
			"saturn.jpg",
			"venus.jpg",
			"clouds.jpg",
			"lava.jpg",
			"icy.jpg",
			"volcanic.jpg",
			"martian.jpg",
			"venusian.jpg",
			"terrestrial.jpg",
			"alpine.jpg",
			"tropical.jpg",
			"savannah.jpg",
			"swamp.jpg"
		};
		for (int i = 0; i < 18; i++) {
			shared_ptr<Texture> t = make_shared<Texture>();
			t->setFilename(resourceDirectory + "/planets/" + directs[i]);
			t->init();
			t->setUnit(0);
			t->setWrapModes(GL_REPEAT, GL_REPEAT);
			planetTextures.push_back(t);
		}

		string directs2[] = {
			"cyan.png",
			"yellow.png",
			"orange.png",
			"blue.png",
			"purple.png",
			"red.png",
			"green.png",
			"black.png"
		};
		for (int i = 0; i < 8; i++) {
			shared_ptr<Texture> t = make_shared<Texture>();
			t->setFilename(resourceDirectory + "/ships/" + directs2[i]);
			t->init();
			t->setUnit(0);
			t->setWrapModes(GL_REPEAT, GL_REPEAT);
			shipTextures.push_back(t);
		}

		sun = make_shared<Texture>();
		sun->setFilename(resourceDirectory + "/planets/sun.jpg");
		sun->init();
		sun->setUnit(0);
		sun->setWrapModes(GL_REPEAT, GL_REPEAT);

		rocket = make_shared<Texture>();
		rocket->setFilename(resourceDirectory + "/rocket.png");
		rocket->init();
		rocket->setUnit(0);
		rocket->setWrapModes(GL_REPEAT, GL_REPEAT);

		partProg = make_shared<Program>();
		partProg->setVerbose(true);
		partProg->setShaderNames(
			resourceDirectory + "/lab10_vert.glsl",
			resourceDirectory + "/lab10_frag.glsl");
		if (! partProg->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		partProg->addUniform("P");
		partProg->addUniform("M");
		partProg->addUniform("V");
		partProg->addUniform("alphaTexture");
		partProg->addAttribute("vertPos");
		partProg->addAttribute("vertColor");

		shared_ptr<Texture> particleExplosion = make_shared<Texture>();
		particleExplosion->setFilename(resourceDirectory + "/alpha.bmp");
		particleExplosion->init();
		particleExplosion->setUnit(0);
		particleExplosion->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		particleTextures.push_back(particleExplosion);
		shared_ptr<Texture> particleBeam = make_shared<Texture>();
		particleBeam->setFilename(resourceDirectory + "/beam.jpg");
		particleBeam->init();
		particleBeam->setUnit(0);
		particleBeam->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		particleTextures.push_back(particleBeam);

		for (int i = 0; i < 50; i++) {
			vec2 p;
			do {
				p = glm::diskRand(100.0);
			} while (glm::length(p) < 20 || nearOtherPlanets(p, 15));
			Planet planet(
				vec3(p.x, glm::linearRand(-3, 3), p.y),
				(int)glm::linearRand(0, 17),
				glm::linearRand(0.5, 1.5) * ((.2 < glm::linearRand(0, 1)) ? 1 : -1),
				glm::linearRand(0.025, .03)
			);
			while (glm::linearRand(0, 1) < .5) {
				do {
					p = glm::diskRand(6.0);
				} while (glm::length(p) < 3);
				planet.moons.push_back(Moon(
					vec3(p.x, glm::linearRand(-1, 1), p.y),
					(int)glm::linearRand(0, 17),
					glm::linearRand(0.5, 1.5) * ((.2 < glm::linearRand(0, 1)) ? 1 : -1),
					glm::linearRand(0.5, 2.0)
				));
			}
			planets.push_back(planet);
		}
		for (int i = 0; i < 100; i++) {
			float radius = glm::linearRand(110.0, 160.0);
			float angle = glm::linearRand(0.0f, 2*PI);
			float rot = glm::linearRand(1.0f, 1.5f);
			float rev = glm::linearRand(.075f, 0.125f);
			float size = glm::linearRand(.01f, .0175f);
			asteroids.push_back(Asteroid(radius, angle, rot, rev, size));
		}

		ufoSrc = &planets[std::rand() % planets.size()];
		ufoDst = &planets[std::rand() % planets.size()];

		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);  

		glGenTextures(1, &textureColorbuffer);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH*2*fboRes, HEIGHT*2*fboRes, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WIDTH*2*fboRes, HEIGHT*2*fboRes); // use a single renderbuffer object for both a depth AND stencil buffer.
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
		// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		createParticles(vec3(0, 0, 0), 0, 1, 0, vec3(0, 0, 0), vec3(0, 0, 0), vec3(1.0f, 0.7f, 0.0f), vec2(100000, 0), 65.0f*sunRadius/100.0f);
	}

	void load(string path) {
		vector<tinyobj::shape_t> TOshapes;
		vector<tinyobj::material_t> objMaterials;
		string errStr;

		bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, path.c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			vector<shared_ptr<Shape> > newMeshes;
			float boundingSphereRadius = 0;
			for (int i = 0; i < TOshapes.size(); i++) {
				shared_ptr<Shape> mesh = make_shared<Shape>();
				mesh->createShape(TOshapes[i]);
				mesh->measure();
				mesh->init();
				for (int j = 0; j < TOshapes[i].mesh.positions.size(); j += 3) {
					mesh->min.x = std::min(mesh->min.x, TOshapes[i].mesh.positions[j]);
					mesh->min.y = std::min(mesh->min.y, TOshapes[i].mesh.positions[j+1]);
					mesh->min.z = std::min(mesh->min.z, TOshapes[i].mesh.positions[j+2]);
					mesh->max.x = std::max(mesh->max.x, TOshapes[i].mesh.positions[j]);
					mesh->max.y = std::max(mesh->max.y, TOshapes[i].mesh.positions[j+1]);
					mesh->max.z = std::max(mesh->max.z, TOshapes[i].mesh.positions[j+2]);
				}
				newMeshes.push_back(mesh);
				boundingSphereRadius = std::max(boundingSphereRadius, std::max(mesh->max.x, mesh->max.y));
			}
			meshes.push_back(make_pair(newMeshes, boundingSphereRadius));
		}
	}

	void initGeom(const std::string& resourceDirectory)
	{
		load(resourceDirectory + "/ufo.obj");
		load(resourceDirectory + "/smoothsphere.obj");
		load(resourceDirectory + "/smoothsphere.obj");
		load(resourceDirectory + "/spherenonormals.obj");
		load(resourceDirectory + "/smoothsphere.obj");
		load(resourceDirectory + "/ship.obj");
		load(resourceDirectory + "/rock.obj");
		load(resourceDirectory + "/rock.obj");
		load(resourceDirectory + "/rock.obj");
		load(resourceDirectory + "/rock.obj");
		load(resourceDirectory + "/rock.obj");
		load(resourceDirectory + "/rock.obj");
		load(resourceDirectory + "/Earth.obj");
		load(resourceDirectory + "/rocket.obj");

		// Initialize cube mesh.
		vector<tinyobj::shape_t> TOshapesC;
 		vector<tinyobj::material_t> objMaterialsC;
		string errStr;
		bool rc = tinyobj::LoadObj(TOshapesC, objMaterialsC, errStr, (resourceDirectory + "/cube.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			cube = make_shared<Shape>();
			cube->createShape(TOshapesC[0]);
			cube->measure();
			cube->init();
		}
	}

	void SetView(shared_ptr<Program> shader) {
		if (fromShip) {
			above->loadIdentity();
			above->translate(vec3(0, -1.5, -5));
			glm::mat4 Cam = glm::lookAt(position, lookAt, vec3(0, 1, 0));
			above->multMatrix(Cam);
			glUniformMatrix4fv(shader->getUniform("V"), 1, GL_FALSE, value_ptr(above->topMatrix()));
		} else {
			wormHoleView = glm::lookAt(wormHoleDst, wormHoleDst + (wormHoleSrc - position), vec3(0, 1, 0));
			glUniformMatrix4fv(shader->getUniform("V"), 1, GL_FALSE, value_ptr(wormHoleView));
		}
	}

	unsigned int createSky(string dir, vector<string> faces) {
		unsigned int textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
		
		int width, height, nrChannels;
		stbi_set_flip_vertically_on_load(false);
		for(GLuint i = 0; i < faces.size(); i++) {
			unsigned char *data = stbi_load((dir+faces[i]).c_str(), &width, &height, &nrChannels, 0);
			if (data) {
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			} else {
				cout << "failed to load: " << (dir+faces[i]).c_str() << endl;
			}
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		cout << " creating cube map any errors : " << glGetError() << endl;
		return textureID;
	}

	void SetMaterial(shared_ptr<Program> curS, int i) {
    	switch (i) {
    		case 3: // asteroids
    			glUniform3f(curS->getUniform("MatAmb"), 0.025, 0.025, 0.025);
    			glUniform3f(curS->getUniform("MatDif"), 155/255.0f, 127/255.0f, 111/255.0f);
    			glUniform3f(curS->getUniform("MatSpec"), 0.0, 0.0, 0.0);
    			glUniform1f(curS->getUniform("MatShine"), 0);
    		break;
    		case 4: // ufo 1
    			glUniform3f(curS->getUniform("MatAmb"), 4.3/255.0f, 16.2/255.0f, 4.3/255.0f);
    			glUniform3f(curS->getUniform("MatDif"), 43/255.0f, 162/255.0f, 43/255.0f);
    			glUniform3f(curS->getUniform("MatSpec"), .1, .1, 0);
    			glUniform1f(curS->getUniform("MatShine"), 10.0);
    		break;
			case 6: // ufo 2
    			glUniform3f(curS->getUniform("MatAmb"), 6/255.0f, 6/255.0f, 6/255.0f);
    			glUniform3f(curS->getUniform("MatDif"), 120/255.0f, 120/255.0f, 120/255.0f);
    			glUniform3f(curS->getUniform("MatSpec"), .7, .7, .4);
    			glUniform1f(curS->getUniform("MatShine"), 2.0);
    		break;
  		}
	}

	void collide() {
		position = vec3(0, 0, sunRadius/10.0f + 10);
		lookAt = vec3(0, 0, 0);
		move = vec3(0, 0, 0);
		speed = 0;
		lookPhi = 0;
		lookTheta = -PI/2;
		tilt = 0;
		uptilt = 0;
	}

	void createParticles(vec3 position, int textureIndex, int numP, float radius, vec3 bias, vec3 vMax, vec3 c, vec2 life, float scale) {
		shared_ptr<particleSys> p = make_shared<particleSys>(position, textureIndex, numP, radius, bias, vMax, c, life, scale);
		p->gpuSetup();
		particleSystems.push_back(p);
	}

	void drawSkybox(shared_ptr<MatrixStack> Model, shared_ptr<MatrixStack> Perspective) {
		cubeProg->bind();
		glUniformMatrix4fv(cubeProg->getUniform("P"), 1, GL_FALSE, value_ptr(Perspective->topMatrix()));
		glDepthFunc(GL_LEQUAL);
		SetView(cubeProg);
		auto Identity = make_shared<MatrixStack>();
		Identity->translate(position);
		Identity->scale(vec3(1000, 1000, 1000));
		glUniformMatrix4fv(cubeProg->getUniform("M"), 1, GL_FALSE, value_ptr(Identity->topMatrix()));
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
		cube->draw(cubeProg);
		glDepthFunc(GL_LESS);
		cubeProg->unbind();
	}
	
	void drawEverythingElse(shared_ptr<MatrixStack> Model, shared_ptr<MatrixStack> Perspective) {
		// UFO
		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Perspective->topMatrix()));
		SetView(prog);
		glUniform3f(prog->getUniform("lightPos"), 0, -5, 0);
		glUniform3f(prog->getUniform("lightPos2"), 0, 0, 0);
		glUniform3f(prog->getUniform("lightPos3"), 0, 5, 0);
		if (!planets.empty()) {
			Model->pushMatrix();
			Model->translate(ufoTimer < 120 ? ufoSrc->position : mix(ufoSrc->position, ufoDst->position, (ufoTimer - 120.0f) / 120.0f));
			Model->translate(vec3(0, 5, 0));
			if (fromShip && ufoTimer >= 120) {
				vec3 move = ufoDst->position - ufoSrc->position;
				Model->rotate(.2f, vec3(move.x * cos(-PI/2) - move.z * sin(-PI/2), 0, move.x * sin(-PI/2) + move.z * cos(-PI/2)));
			}
			Model->rotate(ufoRotation, vec3(0, 1, 0));
			Model->scale(vec3(.1, .1, .1));
			glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
			for (int i = 0; i < meshes[0].first.size(); i++) {
				SetMaterial(prog, i == 0 ? 4 : 6);
				meshes[0].first[i]->draw(prog);
			}
			Model->popMatrix();
			if (fromShip) {
				ufoRotation += .05;
				ufoTimer = (ufoTimer + 1) % 240;
				if (ufoTimer == 0) {
					ufoSrc = ufoDst;
					ufoDst = &planets[std::rand() % planets.size()];
					createParticles(ufoSrc->position + vec3(0, 5, 0), 1, 1, 0, vec3(0, 0, 0), vec3(0, 0, 0), vec3(0.5f, 1.0f, 0.0f), vec2(1, 0), 15.0f);
				}
			}
		}
		prog->unbind();

		// PLANETS
		texProg->bind();
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Perspective->topMatrix()));
		SetView(texProg);
		glUniform3f(texProg->getUniform("lightPos"), 0, -5, 0);
		glUniform3f(texProg->getUniform("lightPos2"), 0, 0, 0);
		glUniform3f(texProg->getUniform("lightPos3"), 0, 5, 0);
		glUniform3f(texProg->getUniform("camPos"), position.x, position.y, position.z);
		for (vector<Planet>::iterator i = planets.begin(); i != planets.end(); i++) {
			if (fromShip) {
				i->position += i->revolutionSpeed*normalize(vec3(i->position.x * cos(-PI/2) - i->position.z * sin(-PI/2), 0, i->position.x * sin(-PI/2) + i->position.z * cos(-PI/2)));
			}
			Model->pushMatrix();
			Model->translate(i->position);
			// MOONS
			for (vector<Moon>::iterator j = i->moons.begin(); j != i->moons.end(); j++) {
				Model->pushMatrix();
				Model->rotate(planetRotation*j->revolutionSpeed, vec3(0, 1, 0));
				Model->translate(j->position);
				Model->rotate(planetRotation*j->rotationSpeed, vec3(0, 1, 0));
				Model->scale(vec3(.003, .003, .003));
				glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
				planetTextures[j->material]->bind(texProg->getUniform("Texture0"));
				for (int i = 0; i < meshes[12].first.size(); i++) {
					meshes[12].first[i]->draw(texProg);
				}

				if (fromShip) {
					vec3 p = vec3(Model->topMatrix()[3][0], Model->topMatrix()[3][1], Model->topMatrix()[3][2]);
					j->absolutePosition = p;
					if (glm::distance(p, position) < meshes[12].second*.003 + meshes[5].second*.05) {
						collide();
					}
					for (vector<Rocket>::iterator k = rockets.begin(); k != rockets.end(); k++) {
						if (glm::distance(p, k->position) < meshes[12].second*.003 + meshes[13].second*.05/2) {
							createParticles(j->absolutePosition, 0, 100, .003*meshes[12].second/4.0f, vec3(0, 0, 0), vec3(1, 1, 1), vec3(.5f, .2f, 0.0f), vec2(2.0f, 3.0f), 1.0f);
							rockets.erase(k);
							i->moons.erase(j);
							j--;
							break;
						}
					}
				}
				Model->popMatrix();
			}
			Model->rotate(planetRotation*i->rotationSpeed, vec3(0, 1, 0));
			Model->scale(vec3(.01, .01, .01));
			glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
			planetTextures[i->material]->bind(texProg->getUniform("Texture0"));
			for (int i = 0; i < meshes[12].first.size(); i++) {
				meshes[12].first[i]->draw(texProg);
			}
			Model->popMatrix();

			if (fromShip) {
				if (glm::distance(i->position, position) < meshes[12].second*.01 + meshes[5].second*.05) {
					collide();
				}
				if (glm::distance(vec3(0, 0, 0), i->position) < meshes[12].second*.01 + meshes[12].second*sunRadius/2000.0f) {
					planets.erase(i);
					i--;
					expandSun();
				}
				for (vector<Rocket>::iterator j = rockets.begin(); j != rockets.end(); j++) {
					if (glm::distance(i->position, j->position) < meshes[12].second*.01 + meshes[13].second*.05/2) {
						// collide();
						for (vector<Moon>::iterator j = i->moons.begin(); j != i->moons.end(); j++) {
							j->escape(i->position);
							looseMoons.push_back(*j);
						}
						createParticles(i->position, 0, 300, .01*meshes[12].second/4.0f, vec3(0, 0, 0), vec3(2, 2, 2), vec3(.5f, .2f, 0.0f), vec2(2.0f, 3.0f), 1.0f);
						rockets.erase(j);
						planets.erase(i);
						i--;
						break;
					}

				}
			}
		}
		// LOOSE MOONS
		for (vector<Moon>::iterator i = looseMoons.begin(); i != looseMoons.end(); i++) {
			i->update();
			Model->pushMatrix();
			Model->translate(i->position);
			Model->rotate(planetRotation*i->rotationSpeed, vec3(0, 1, 0));
			Model->scale(vec3(.003, .003, .003));
			glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
			planetTextures[i->material]->bind(texProg->getUniform("Texture0"));
			for (int i = 0; i < meshes[12].first.size(); i++) {
				meshes[12].first[i]->draw(texProg);
			}
			Model->popMatrix();

			if (fromShip) {
				if (glm::distance(i->position, vec3(0, 0, 0)) < meshes[12].second*.003 + meshes[12].second*sunRadius/2000.0f) {
					expandSun();
					looseMoons.erase(i);
					i--;
				}
				i->escapeDirection += (sunRadius/10.0f)/(float)std::pow(glm::distance(i->position, vec3(0, 0, 0)), 2) * -normalize(i->position);
			}
		}
		if (fromShip) {
			planetRotation += .01;
		}

		// ROCKETS
		rocket->bind(texProg->getUniform("Texture0"));
		for (vector<Rocket>::iterator i = rockets.begin(); i != rockets.end();) {
			if (fromShip) {
				i->update();
			}
			Model->pushMatrix();
			Model->translate(i->position);
			Model->rotate(i->rotation.y, vec3(0, 1, 0));
			Model->rotate(i->rotation.z, vec3(0, 0, 1));
			Model->rotate(i->rotation.x, vec3(1, 0, 0));
			Model->scale(vec3(.05, .05, .05));
			glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
			for (int i = 0; i < meshes[13].first.size(); i++) {
				meshes[13].first[i]->draw(texProg);
			}
			Model->popMatrix();
			if (fromShip && i->life >= i->lifeEnd) {
				rockets.erase(i);
			} else {
				i++;
			}
		}

		// SHIP
		Model->pushMatrix();
		Model->translate(position);
		Model->rotate(-lookTheta+PI/2, vec3(0, 1, 0));
		Model->rotate(-lookPhi - uptilt, vec3(1, 0, 0));
		Model->rotate(glm::clamp(-2*tilt, -PI/4, PI/4), vec3(0, 0, 1));
		Model->scale(vec3(.15, .15, .15));
		SetView(texProg);
		glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		shipTextures[matIndex%8]->bind(texProg->getUniform("Texture0"));
		for (int i = 0; i < meshes[5].first.size(); i++) {
			meshes[5].first[i]->draw(texProg);
		}
		Model->popMatrix();
		texProg->unbind();

		// ASTEROIDS
		prog->bind();
		SetMaterial(prog, 3);
		for (int i = 0; i < asteroids.size(); i++) {
			Model->pushMatrix();
			Model->rotate(asteroids[i].startAngle + asteroids[i].revolutionSpeed*planetRotation, vec3(0, 1, 0));
			Model->translate(vec3(asteroids[i].radius, 0, 0));
			Model->rotate(-asteroids[i].rotationSpeed*planetRotation, vec3(1, 0, 0));
			Model->scale(vec3(1, 1, 1)*asteroids[i].size);
			glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
			for (int i = 0; i < meshes[6].first.size(); i++) {
				meshes[6].first[i]->draw(prog);
			}
			Model->popMatrix();
		}
		prog->unbind();
	}

	void drawSun(shared_ptr<MatrixStack> Model, shared_ptr<MatrixStack> Perspective) {
		Model->pushMatrix();
		texProgNoLighting->bind();
		SetView(texProgNoLighting);
		glUniformMatrix4fv(texProgNoLighting->getUniform("P"), 1, GL_FALSE, value_ptr(Perspective->topMatrix()));
		Model->translate(vec3(0, 0, 0));
		Model->rotate(planetRotation*.2, vec3(0, 1, 0));
		Model->scale(vec3(1, 1, 1)*sunRadius/2000.0f);
		glUniformMatrix4fv(texProgNoLighting->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		sun->bind(texProgNoLighting->getUniform("Texture0"));
		for (int i = 0; i < meshes[12].first.size(); i++) {
			meshes[12].first[i]->draw(texProgNoLighting);
		}
		Model->popMatrix();
		texProgNoLighting->unbind();

		if (fromShip && glm::distance(vec3(0, 0, 0), position) < meshes[12].second*sunRadius/2000.0f + meshes[5].second*.05) {
			collide();
		}
	}

	void drawParticles(shared_ptr<MatrixStack> Model, shared_ptr<MatrixStack> Perspective, glm::mat4 View) {
		CHECKED_GL_CALL(glEnable(GL_DEPTH_TEST));
		CHECKED_GL_CALL(glEnable(GL_BLEND));
		CHECKED_GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
		partProg->bind();
		SetView(partProg);
		CHECKED_GL_CALL(glUniformMatrix4fv(partProg->getUniform("P"), 1, GL_FALSE, value_ptr(Perspective->topMatrix())));
		CHECKED_GL_CALL(glUniformMatrix4fv(partProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix())));
		for (vector<shared_ptr<particleSys> >::iterator i = particleSystems.begin(); i != particleSystems.end(); i++) {
			particleTextures[(*i)->textureIndex]->bind(partProg->getUniform("alphaTexture"));
			(*i)->setCamera(View);
			vec3 camPos(inverse(View)[3]);
			glPointSize((*i)->scale * 1000.0f/glm::distance(camPos, (*i)->start));
			(*i)->drawMe(partProg);
			if (fromShip) {
				(*i)->update();
				if ((*i)->textureIndex == 1) {
					(*i)->lock(ufoSrc->position + vec3(0, 5, 0));
				}
				if ((*i)->isDone()) {
					particleSystems.erase(i);
					i--;
				}
			}
		}
		partProg->unbind();
		CHECKED_GL_CALL(glDisable(GL_DEPTH_TEST));
		CHECKED_GL_CALL(glDisable(GL_BLEND));
		CHECKED_GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	}

	void render()
	{
		if (abs(tilt) > .01) {
			tilt -= .01*sign(tilt);
		} else {
			tilt = 0;
		}
		if (abs(uptilt) > .01) {
			uptilt -= .01*sign(uptilt);
		} else {
			uptilt = 0;
		}
		if (wKey){
			move.x += cos(lookPhi)*cos(lookTheta);
			move.y += sin(lookPhi);
			move.z += cos(lookPhi)*sin(lookTheta);
		}
		if (sKey){
			move.x -= cos(lookPhi)*cos(lookTheta);
			move.y -= sin(lookPhi);
			move.z -= cos(lookPhi)*sin(lookTheta);
		}
		if (aKey) {
			move.x += sin(lookTheta);
			move.y -= cos(lookPhi)*sin(tilt);
			move.z -= cos(lookTheta);
		}
		if (dKey) {
			move.x -= sin(lookTheta);
			move.y += cos(lookPhi)*sin(tilt);
			move.z += cos(lookTheta);
		}
		if (wKey || sKey || aKey || dKey) {
			speed = std::min(speed + friction, maxSpeed + (boosters ? 1 : 0));
		} else if (speed <= friction) {
			speed = 0;
			move = vec3(0, 0, 0);
		} else if (speed > friction) {
			speed -= friction;
		}
		if (speed != 0) {
			vec3 normalized = speed*glm::normalize(move);
			move = normalized;
			position += normalized;
		}
		lookAt = position + vec3(10*cos(lookPhi)*cos(lookTheta), 10*sin(lookPhi), 10*cos(lookPhi)*cos(PI/2-lookTheta));

		auto Model = make_shared<MatrixStack>();
		auto View = make_shared<MatrixStack>();
		auto Perspective = make_shared<MatrixStack>();
		auto Perspective2 = make_shared<MatrixStack>();
		glfwGetFramebufferSize(windowManager->getHandle(), &WIDTH, &HEIGHT);
		glViewport(0, 0, WIDTH*fboRes, HEIGHT*fboRes);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float aspect = WIDTH/(float)HEIGHT;
		Perspective->pushMatrix();
		Perspective->perspective(50.0f * PI / 180, aspect, 0.1f, 1000.0f);
		Perspective2->pushMatrix();
		Perspective2->perspective(fov * PI / 180, aspect, 0.1f, 1000.0f);
		View->pushMatrix();
		View->loadIdentity();

		// draw everything to fbo
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
		fromShip = false;
		drawSkybox(Model, Perspective2);
		drawEverythingElse(Model, Perspective2);
        drawSun(Model, Perspective2);
		// drawParticles(Model, Perspective2, wormHoleView);
		fromShip = true;

		glfwGetFramebufferSize(windowManager->getHandle(), &WIDTH, &HEIGHT);
		glViewport(0, 0, WIDTH, HEIGHT);
		
		// draw texturecolorbuffer to wormhole
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        texProgNoLighting->bind();
        Model->pushMatrix();
        Model->translate(wormHoleSrc);
		vec3 perp = wormHoleSrc - position;
		Model->rotate(-lookPhi, vec3(perp.x * cos(-PI/2) - perp.z * sin(-PI/2), 0, perp.x * sin(-PI/2) + perp.z * cos(-PI/2)));
		Model->rotate(-lookTheta + PI, vec3(0, 1, 0));
        Model->scale(vec3(.2, .2, .2));
		SetView(texProgNoLighting);
		glUniformMatrix4fv(texProgNoLighting->getUniform("P"), 1, GL_FALSE, value_ptr(Perspective->topMatrix()));
        glUniformMatrix4fv(texProgNoLighting->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        for (int i = 0; i < meshes[12].first.size(); i++) {
            meshes[12].first[i]->draw(texProgNoLighting);
        }
        Model->popMatrix();
        texProgNoLighting->unbind();
		if (glm::distance(wormHoleSrc, position) < meshes[12].second*.2 + meshes[5].second*.05) {
			position = wormHoleDst;
		}

		// draw normally
		drawSkybox(Model, Perspective);
		drawEverythingElse(Model, Perspective);
        drawSun(Model, Perspective);
		drawParticles(Model, Perspective, above->topMatrix());

		View->popMatrix();
		Perspective->popMatrix();
		Perspective2->popMatrix();

	}
};

int main(int argc, char *argv[])
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(WIDTH, HEIGHT);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;
	vector<std::string> skyFaces = {
		"right.jpg",
		"left.jpg",
		"top.jpg",
		"bottom.jpg",
		"front.jpg",
		"back.jpg",
	};
	cubeMapTexture = application->createSky(resourceDir + "/skybox/", skyFaces);

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
