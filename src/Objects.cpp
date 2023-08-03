#include "Objects.h"

#include <SDL_image.h>
#include <cassert>

static const char* OBJ_NAMES[OBJ_COUNT] = { "Blue.png", "Green.png", "Purple.png", "Red.png", "Yellow.png" };
static const SDL_Point OBJ_SIZES[OBJ_COUNT] = { { 35, 36 }, { 35, 35 }, { 35, 35 }, { 34, 36 }, { 38, 37 } };

bool Image::Load(SDL_Renderer* rend, const char* name)
{
	m_text = IMG_LoadTexture(rend, name);

	return m_text != 0;
}

Image::~Image()
{
	SDL_DestroyTexture(m_text);
}

bool Objects::Load(SDL_Renderer* rend)
{
	for (int i = 0; i < OBJ_COUNT; ++i)
	{
		if (!m_images[i].Load(rend, OBJ_NAMES[i]))
			return false;
	}

	return true;
}

void Objects::DrawTexture(SDL_Renderer* rend, int x, int y, int w, int h, int idx, double scale)
{
	double scaleX = double(w) / OBJ_WIDTH;
	double scaleY = double(h) / OBJ_HEIGHT;
	const int obj_width  = int(Size(idx).x * scaleX * scale + 0.5);
	const int obj_height = int(Size(idx).y * scaleY * scale + 0.5);
	const int x_adj = int((OBJ_WIDTH  * scaleX - obj_width)  / 2 + 0.5);
	const int y_adj = int((OBJ_HEIGHT * scaleY - obj_height) / 2 + 0.5);
	const SDL_Rect dest = { x + x_adj, y + y_adj, obj_width, obj_height };
	SDL_RenderCopy(rend, Texture(idx), NULL,  &dest);
}

SDL_Texture* Objects::Texture(int idx)
{
	assert(idx < OBJ_COUNT);
	return m_images[idx].Texture();
}

const SDL_Point& Objects::Size(int idx)
{
	assert(idx < OBJ_COUNT);
	return OBJ_SIZES[idx];
}