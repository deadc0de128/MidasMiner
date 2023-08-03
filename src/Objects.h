#pragma once

struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Point;

#include <cassert>

#if defined(__unix__)
    #define ASSET_NAME(s) "assets/" s
#else
    #define ASSET_NAME(s) "assets\\" s
#endif

class Image
{
public:
	Image() : m_text(0)	{ }
	~Image();

	bool Load(SDL_Renderer* rend, const char* name);
	SDL_Texture* Texture() { assert(m_text); return m_text; }

private:
	SDL_Texture* m_text;
};

const int OBJ_WIDTH = 40;
const int OBJ_HEIGHT = 40;
const int OBJ_COUNT = 5;

class Objects
{
public:
	bool Load(SDL_Renderer* rend);
	void DrawTexture(SDL_Renderer* rend, int x, int y, int w, int h, int idx, double scale);

private:
	SDL_Texture* Texture(int idx);
	const SDL_Point& Size(int idx);

	Image m_images[OBJ_COUNT];
};
