#include "libs/glad_loader_docs.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include "PolygonWithTexture.cpp"
#include <learnopengl/shader_m.h>
#include "stb_image.h"

using namespace glm;

int SCR_WIDTH = 1280;
int SCR_HEIGHT = 720;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
const float ROOM_HALF = 8.0f;   
const float ROOM_H = 6.0f;      
const float TABLE_Y = 1.0f;     
const float CARD_W = 0.43f;
const float CARD_H = 0.60f;
const int NUM_CARDS = 5;
enum CardState
{
	IN_DECK, 
	MOVING,  
	PLAYED   
};

struct Card
{
	GLuint texture;
	vec3 pos;      
	vec3 startPos; 
	vec3 endPos;   
	float t;       
	float angle;   
	CardState state;
};

std::vector<Card> cards;
int playedCount = 0;

float slotX(int i) { return -1.2f + 0.6f * i; }
const float DECK_Z = 1.05f;   
const float PLAYED_Z = 0.0f;  

const float MOVE_DURATION = 0.9f; 
const float ROTATE_SPEED = 150.0f;

int cameraMode = 1;

const vec3 EYE_1 = vec3(0.0f, TABLE_Y + 4.5f, 0.0f);
const vec3 TARGET_1 = vec3(0.0f, TABLE_Y, 0.0f);
const vec3 UP_1 = vec3(0.0f, 0.0f, -1.0f);

const vec3 EYE_2 = vec3(0.0f, TABLE_Y + 1.9f, 3.6f);
const vec3 TARGET_2 = vec3(0.0f, TABLE_Y + 0.4f, -1.5f);
const vec3 UP_2 = vec3(0.0f, 1.0f, 0.0f);

vec3 camEye = EYE_1;
vec3 camTarget = TARGET_1;
vec3 camUp = UP_1;

bool gWasPressed = false;

void processInput(GLFWwindow *window);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);


unsigned int loadTexture(const char *path)
{
	unsigned int id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
	if (data)
	{
		GLenum format = GL_RGB;
		if (nrChannels == 1)
			format = GL_RED;
		else if (nrChannels == 4)
			format = GL_RGBA;
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture: " << path << std::endl;
		unsigned char pink[3] = {255, 0, 255}; 
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, pink);
	}
	stbi_image_free(data);
	return id;
}

PolygonWithTexture makeQuad(vec3 a, vec3 b, vec3 c, vec3 d, float uRep = 1.0f, float vRep = 1.0f)
{
	std::vector<vec3> v = {a, b, c, d};
	std::vector<vec2> t = {vec2(0.0f, 0.0f), vec2(0.0f, vRep), vec2(uRep, vRep), vec2(uRep, 0.0f)};
	return PolygonWithTexture(v, t);
}

PolygonWithTexture makeCircle(float radius, int segments)
{
	std::vector<vec3> v;
	std::vector<vec2> t;
	for (int i = 0; i < segments; i++)
	{
		float a = 2.0f * 3.14159265f * i / segments;
		v.push_back(vec3(radius * cos(a), radius * sin(a), 0.0f));
		t.push_back(vec2(0.5f + 0.5f * cos(a), 0.5f + 0.5f * sin(a)));
	}
	return PolygonWithTexture(v, t);
}

struct Box
{
	std::vector<PolygonWithTexture> faces;
	std::vector<float> shade; 

	Box()
	{
		float h = 0.5f;
		faces.push_back(makeQuad(vec3(-h, -h, h), vec3(-h, h, h), vec3(h, h, h), vec3(h, -h, h)));     
		faces.push_back(makeQuad(vec3(h, -h, -h), vec3(h, h, -h), vec3(-h, h, -h), vec3(-h, -h, -h))); 
		faces.push_back(makeQuad(vec3(-h, -h, -h), vec3(-h, h, -h), vec3(-h, h, h), vec3(-h, -h, h))); 
		faces.push_back(makeQuad(vec3(h, -h, h), vec3(h, h, h), vec3(h, h, -h), vec3(h, -h, -h)));     
		faces.push_back(makeQuad(vec3(-h, h, h), vec3(-h, h, -h), vec3(h, h, -h), vec3(h, h, h)));     
		faces.push_back(makeQuad(vec3(-h, -h, -h), vec3(-h, -h, h), vec3(h, -h, h), vec3(h, -h, -h))); 
		shade = {0.90f, 0.75f, 0.80f, 0.80f, 1.0f, 0.55f};
	}

	void draw(Shader &shader, mat4 model, GLuint texture, vec3 color)
	{
		for (int i = 0; i < 6; i++)
		{
			faces[i].setTexture(texture);
			faces[i].setColor(color * shade[i]);
			faces[i].transformation(model);
			faces[i].draw(shader);
		}
	}
};

mat4 boxModel(vec3 center, vec3 size)
{
	return scale(translate(mat4(1.0f), center), size);
}

mat4 cardModel(const Card &c)
{
	mat4 m = mat4(1.0f);
	m = translate(m, c.pos + vec3(0.0f, 0.003f, 0.0f));      
	m = rotate(m, radians(c.angle), vec3(1.0f, 0.0f, 0.0f)); 
	m = translate(m, vec3(0.0f, CARD_H / 2.0f, 0.0f));       
	return m;
}

void playRandomCard()
{
	std::vector<int> deckIndices;
	for (int i = 0; i < (int)cards.size(); i++)
		if (cards[i].state == IN_DECK)
			deckIndices.push_back(i);

	if (deckIndices.empty())
		return;

	int chosen = deckIndices[rand() % deckIndices.size()];
	Card &c = cards[chosen];
	c.state = MOVING;
	c.t = 0.0f;
	c.startPos = c.pos;
	c.endPos = vec3(slotX(playedCount), TABLE_Y, PLAYED_Z);
	playedCount++;
}

void updateCards(float dt)
{
	float targetAngle = (cameraMode == 1) ? -90.0f : 0.0f;

	for (Card &c : cards)
	{
		if (c.angle < targetAngle)
			c.angle = std::min(c.angle + ROTATE_SPEED * dt, targetAngle);
		else if (c.angle > targetAngle)
			c.angle = std::max(c.angle - ROTATE_SPEED * dt, targetAngle);

		if (c.state == MOVING)
		{
			c.t += dt / MOVE_DURATION;
			if (c.t >= 1.0f)
			{
				c.t = 1.0f;
				c.state = PLAYED;
			}
			float s = c.t * c.t * (3.0f - 2.0f * c.t);             
			c.pos = mix(c.startPos, c.endPos, s);                  
			c.pos.y += sin(c.t * 3.14159265f) * 0.5f;              
		}
	}
}

int main()
{
	srand((unsigned)time(nullptr));

	glfwInit();
	GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Card Game - HW 2026", NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	stbi_set_flip_vertically_on_load(true);

	Shader ourShader("./shaders/vs/HW.vs", "./shaders/fs/HW.fs");

	GLuint texWhite = loadTexture("./textures/white.jpg");
	GLuint texWood = loadTexture("./textures/wood.jpg");
	GLuint texWallpaper = loadTexture("./textures/wallpaper.jpg");
	GLuint texPoster = loadTexture("./textures/poster.jpg");
	GLuint texZoneDeck = loadTexture("./textures/zone_deck.jpg");     
	GLuint texZonePlayed = loadTexture("./textures/zone_played.jpg"); 
	GLuint texZoneEnemy = loadTexture("./textures/zone_enemy.jpg");   
	GLuint texCards[NUM_CARDS];
	texCards[0] = loadTexture("./textures/card1.jpg");
	texCards[1] = loadTexture("./textures/card2.jpg");
	texCards[2] = loadTexture("./textures/card3.jpg");
	texCards[3] = loadTexture("./textures/card4.jpg");
	texCards[4] = loadTexture("./textures/card5.jpg");

	float R = ROOM_HALF, H = ROOM_H;
	PolygonWithTexture floorQ = makeQuad(vec3(-R, 0, R), vec3(-R, 0, -R), vec3(R, 0, -R), vec3(R, 0, R), 8.0f, 8.0f);
	PolygonWithTexture ceilQ = makeQuad(vec3(-R, H, -R), vec3(-R, H, R), vec3(R, H, R), vec3(R, H, -R));
	PolygonWithTexture wallBack = makeQuad(vec3(-R, 0, -R), vec3(-R, H, -R), vec3(R, H, -R), vec3(R, 0, -R), 6.0f, 2.25f);
	PolygonWithTexture wallFront = makeQuad(vec3(R, 0, R), vec3(R, H, R), vec3(-R, H, R), vec3(-R, 0, R), 6.0f, 2.25f);
	PolygonWithTexture wallLeft = makeQuad(vec3(-R, 0, R), vec3(-R, H, R), vec3(-R, H, -R), vec3(-R, 0, -R), 6.0f, 2.25f);
	PolygonWithTexture wallRight = makeQuad(vec3(R, 0, -R), vec3(R, H, -R), vec3(R, H, R), vec3(R, 0, R), 6.0f, 2.25f);

	Box box; 

	float zy = TABLE_Y + 0.001f;
	PolygonWithTexture deckZone = makeQuad(vec3(-1.7f, zy, 1.15f), vec3(-1.7f, zy, 0.30f), vec3(1.7f, zy, 0.30f), vec3(1.7f, zy, 1.15f));
	PolygonWithTexture playedZone = makeQuad(vec3(-1.7f, zy, 0.10f), vec3(-1.7f, zy, -0.75f), vec3(1.7f, zy, -0.75f), vec3(1.7f, zy, 0.10f));
	PolygonWithTexture enemyZone = makeQuad(vec3(-1.7f, zy, -0.90f), vec3(-1.7f, zy, -1.50f), vec3(1.7f, zy, -1.50f), vec3(1.7f, zy, -0.90f));
	deckZone.setTexture(texZoneDeck);
	playedZone.setTexture(texZonePlayed);
	enemyZone.setTexture(texZoneEnemy);

	float w2 = CARD_W / 2.0f, h2 = CARD_H / 2.0f, m = 0.025f;
	PolygonWithTexture cardBase = makeQuad(vec3(-w2, -h2, 0), vec3(-w2, h2, 0), vec3(w2, h2, 0), vec3(w2, -h2, 0));
	PolygonWithTexture cardFace = makeQuad(vec3(-w2 + m, -h2 + m, 0.002f), vec3(-w2 + m, h2 - m, 0.002f),
										   vec3(w2 - m, h2 - m, 0.002f), vec3(w2 - m, -h2 + m, 0.002f));
	cardBase.setTexture(texWhite);
	cardBase.setColor(vec3(0.95f, 0.93f, 0.88f));

	for (int i = 0; i < NUM_CARDS; i++)
	{
		Card c;
		c.texture = texCards[i];
		c.pos = vec3(slotX(i), TABLE_Y, DECK_Z);
		c.startPos = c.endPos = c.pos;
		c.t = 0.0f;
		c.angle = -90.0f; 
		c.state = IN_DECK;
		cards.push_back(c);
	}

	vec3 pc = vec3(-2.4f, 2.9f, -R + 0.02f);
	float pw = 1.25f, ph = 1.0f; 
	PolygonWithTexture poster = makeQuad(pc + vec3(-pw, -ph, 0), pc + vec3(-pw, ph, 0), pc + vec3(pw, ph, 0), pc + vec3(pw, -ph, 0));
	poster.setTexture(texPoster);

	vec3 clockC = vec3(2.2f, 3.2f, -R);
	PolygonWithTexture clockRim = makeCircle(0.80f, 48);
	PolygonWithTexture clockFace = makeCircle(0.70f, 48);
	PolygonWithTexture clockPin = makeCircle(0.04f, 16);
	PolygonWithTexture handQ = makeQuad(vec3(-0.5f, 0, 0), vec3(-0.5f, 1, 0), vec3(0.5f, 1, 0), vec3(0.5f, 0, 0));
	clockRim.setTexture(texWood);
	clockRim.setColor(vec3(0.6f, 0.45f, 0.35f));
	clockFace.setTexture(texWhite);
	clockFace.setColor(vec3(0.97f, 0.96f, 0.9f));
	clockPin.setTexture(texWhite);
	clockPin.setColor(vec3(0.8f, 0.1f, 0.1f));
	handQ.setTexture(texWhite);

	ourShader.use();

	std::cout << "Controls: [1] top view  [2] player view  [G] play a random card  [ESC] exit" << std::endl;

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);
		updateCards(deltaTime);

		glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		mat4 projection = perspective(radians(45.0f), (float)SCR_WIDTH / (float)std::max(SCR_HEIGHT, 1), 0.1f, 100.0f);
		ourShader.setMat4("projection", projection);

		vec3 goalEye = (cameraMode == 1) ? EYE_1 : EYE_2;
		vec3 goalTarget = (cameraMode == 1) ? TARGET_1 : TARGET_2;
		vec3 goalUp = (cameraMode == 1) ? UP_1 : UP_2;
		float k = 1.0f - exp(-6.0f * deltaTime);
		camEye = mix(camEye, goalEye, k);
		camTarget = mix(camTarget, goalTarget, k);
		camUp = normalize(mix(camUp, goalUp, k));

		mat4 view = lookAt(camEye, camTarget, camUp);
		ourShader.setMat4("view", view);

		ourShader.setFloat("alpha", 1.0f);
		ourShader.setInt("torn", 0);

		floorQ.setTexture(texWood);
		floorQ.draw(ourShader);
		ceilQ.setTexture(texWhite);
		ceilQ.setColor(vec3(0.85f, 0.85f, 0.82f));
		ceilQ.draw(ourShader);
		wallBack.setTexture(texWallpaper);
		wallFront.setTexture(texWallpaper);
		wallLeft.setTexture(texWallpaper);
		wallRight.setTexture(texWallpaper);
		wallLeft.setColor(vec3(0.9f));
		wallRight.setColor(vec3(0.9f));
		wallBack.draw(ourShader);
		wallFront.draw(ourShader);
		wallLeft.draw(ourShader);
		wallRight.draw(ourShader);

		vec3 tableColor = vec3(0.75f, 0.55f, 0.40f);
		box.draw(ourShader, boxModel(vec3(0, TABLE_Y - 0.05f, 0), vec3(4.2f, 0.1f, 3.2f)), texWood, tableColor);
		float legH = TABLE_Y - 0.1f;
		for (int sx = -1; sx <= 1; sx += 2)
			for (int sz = -1; sz <= 1; sz += 2)
				box.draw(ourShader, boxModel(vec3(sx * 1.9f, legH / 2.0f, sz * 1.4f), vec3(0.15f, legH, 0.15f)), texWood, tableColor * 0.8f);

		deckZone.draw(ourShader);
		playedZone.draw(ourShader);
		enemyZone.draw(ourShader); 

		for (Card &c : cards)
		{
			mat4 cm = cardModel(c);
			cardBase.transformation(cm);
			cardBase.draw(ourShader);
			cardFace.setTexture(c.texture);
			cardFace.transformation(cm);
			cardFace.draw(ourShader);
		}

		ourShader.setInt("torn", 1);
		poster.draw(ourShader);
		ourShader.setInt("torn", 0);


    int clockHour = 3;    
    int clockMinute = 45; 
    int clockSecond = 15;  

    float hourA = clockHour * 30.0f + clockMinute * 0.5f;  
    float minA = clockMinute * 6.0f + clockSecond * 0.1f;   
    float secA = clockSecond * 6.0f;                       


		clockRim.transformation(translate(mat4(1.0f), clockC + vec3(0, 0, 0.01f)));
		clockRim.draw(ourShader);
		clockFace.transformation(translate(mat4(1.0f), clockC + vec3(0, 0, 0.02f)));
		clockFace.draw(ourShader);
		for (int i = 0; i < 12; i++) 
		{
			mat4 tm = translate(mat4(1.0f), clockC + vec3(0, 0, 0.025f));
			tm = rotate(tm, radians(-30.0f * i), vec3(0, 0, 1));
			tm = translate(tm, vec3(0, 0.55f, 0));
			tm = scale(tm, vec3(i % 3 == 0 ? 0.05f : 0.025f, 0.11f, 1.0f));
			handQ.setColor(vec3(0.15f));
			handQ.transformation(tm);
			handQ.draw(ourShader);
		}
		struct Hand { float angle, width, length, z; vec3 color; };
		Hand hands[3] = {{hourA, 0.06f, 0.35f, 0.03f, vec3(0.1f)},
						 {minA, 0.04f, 0.55f, 0.035f, vec3(0.1f)},
						 {secA, 0.015f, 0.60f, 0.04f, vec3(0.8f, 0.1f, 0.1f)}};
		for (Hand &hd : hands)
		{
			mat4 hm = translate(mat4(1.0f), clockC + vec3(0, 0, hd.z));
			hm = rotate(hm, radians(-hd.angle), vec3(0, 0, 1));
			hm = scale(hm, vec3(hd.width, hd.length, 1.0f));
			handQ.setColor(hd.color);
			handQ.transformation(hm);
			handQ.draw(ourShader);
		}
		clockPin.transformation(translate(mat4(1.0f), clockC + vec3(0, 0, 0.045f)));
		clockPin.draw(ourShader);

		{
			float bx = -5.6f;      
			float bz = -R + 0.35f; 
			float shelfW = 2.0f, shelfD = 0.6f, shelfH = 3.0f;
			vec3 shelfColor = vec3(0.55f, 0.38f, 0.25f);
			box.draw(ourShader, boxModel(vec3(bx - shelfW / 2, shelfH / 2, bz), vec3(0.06f, shelfH, shelfD)), texWood, shelfColor); 
			box.draw(ourShader, boxModel(vec3(bx + shelfW / 2, shelfH / 2, bz), vec3(0.06f, shelfH, shelfD)), texWood, shelfColor); 
			box.draw(ourShader, boxModel(vec3(bx, shelfH / 2, -R + 0.03f), vec3(shelfW, shelfH, 0.04f)), texWood, shelfColor * 0.7f); 
			for (int s = 0; s < 5; s++) 
				box.draw(ourShader, boxModel(vec3(bx, 0.05f + s * 0.73f, bz), vec3(shelfW, 0.05f, shelfD)), texWood, shelfColor);

			vec3 bookColors[7] = {vec3(0.45f, 0.08f, 0.10f),  
								  vec3(0.08f, 0.28f, 0.16f),  
								  vec3(0.10f, 0.14f, 0.32f),  
								  vec3(0.62f, 0.48f, 0.12f),  
								  vec3(0.36f, 0.20f, 0.10f),  
								  vec3(0.10f, 0.09f, 0.08f),  
								  vec3(0.30f, 0.10f, 0.24f)}; 
			vec3 gold = vec3(0.80f, 0.65f, 0.30f);
			for (int s = 0; s < 4; s++)
			{
				float baseY = 0.075f + s * 0.73f;
				float x = bx - shelfW / 2 + 0.08f;
				float xEnd = bx + shelfW / 2 - 0.08f - (s == 1 ? 0.6f : 0.0f); 
				for (int n = 0;; n++)
				{
					float thick = 0.08f + 0.02f * ((s + n) % 3);
					float bh = 0.40f + 0.05f * ((s * 2 + n) % 4);
					if (x + thick > xEnd)
						break;
					box.draw(ourShader, boxModel(vec3(x + thick / 2, baseY + bh / 2, bz + 0.02f), vec3(thick, bh, 0.42f)),
							 texWhite, bookColors[(s * 5 + n * 3) % 7]);
					for (float band : {0.15f, 0.85f})
						box.draw(ourShader, boxModel(vec3(x + thick / 2, baseY + bh * band, bz + 0.02f), vec3(thick + 0.004f, 0.022f, 0.424f)),
								 texWhite, gold);
					x += thick + 0.01f;
				}
			}
		}

		mat4 broomM = translate(mat4(1.0f), vec3(5.6f, 1.24f, -R + 0.22f)); 
		broomM = rotate(broomM, radians(-10.0f), vec3(1, 0, 0));          
		broomM = rotate(broomM, radians(-90.0f), vec3(0, 0, 1));          

		box.draw(ourShader, broomM * boxModel(vec3(-0.2f, 0, 0), vec3(1.6f, 0.06f, 0.06f)), texWood, vec3(0.45f, 0.3f, 0.2f));
		box.draw(ourShader, broomM * boxModel(vec3(0.65f, 0, 0), vec3(0.1f, 0.1f, 0.1f)), texWhite, vec3(0.25f, 0.15f, 0.08f));
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
			{
				float angZ = -9.0f + 6.0f * i; 
				float angY = -9.0f + 6.0f * j; 
				mat4 twig = broomM;
				twig = translate(twig, vec3(0.65f, 0, 0));            
				twig = rotate(twig, radians(angY), vec3(0, 1, 0));    
				twig = rotate(twig, radians(angZ), vec3(0, 0, 1));
				twig = translate(twig, vec3(0.3f, 0, 0));             
				twig = scale(twig, vec3(0.6f, 0.03f, 0.03f));
				vec3 straw = ((i + j) % 2 == 0) ? vec3(0.85f, 0.68f, 0.35f) : vec3(0.72f, 0.55f, 0.28f);
				box.draw(ourShader, twig, texWhite, straw);
			}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_1) == GLFW_PRESS)
		cameraMode = 1;
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_2) == GLFW_PRESS)
		cameraMode = 2;

	
	bool gDown = glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS;
	if (gDown && !gWasPressed)
		playRandomCard();
	gWasPressed = gDown;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	SCR_WIDTH = width;
	SCR_HEIGHT = height;
	glViewport(0, 0, width, height);
}
