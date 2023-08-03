#pragma once

#include <SDL_rect.h>
#include <memory>
#include <list>

#include "Grid.h"

class Objects;
struct SDL_Renderer;

class Animation
{
public:
	virtual ~Animation() {}
	virtual bool Draw() = 0;
};

class Animations
{
public:
	~Animations();

	void AddAnimation(std::auto_ptr<Animation> anim);

	bool Active() const { return !m_animations.empty(); }

	void Draw(SDL_Renderer* rend);

	void Cancel();

private:
	std::list<Animation*> m_animations;
	Uint32 m_animEvent;
};

class AnimateSwap : public Animation
{
public:
	AnimateSwap(Grid& grid, int x1, int y1, int clr1, int x2, int y2, int clr2, bool wrong, Uint32 delay);

	virtual bool Draw();

	static const Uint32 DURATION;

private:
	Grid& m_grid;
	int m_clr1, m_clr2;
	bool m_wrong;
	Uint32 m_delay;

	Uint32 m_animStartTS;
	SDL_Rect m_rc;
	bool m_swapped;	
};

class AnimateScaling : public Animation
{
public:
	AnimateScaling(Grid& grid, int x, int y, int count, int xMult, int yMult, int clr, Uint32 delay, bool scaleDown);

	virtual bool Draw();

	static const Uint32 DURATION;

private:
	Grid& m_grid;
	int m_x, m_y;	
	int m_count;
	int m_xMult, m_yMult;
	int m_clr;
	bool m_scaleDown;
	Uint32 m_animStartTS;
	Uint32 m_delay;
};

class AnimateRemoval :  public AnimateScaling
{
public:
	AnimateRemoval(Grid& grid, int x, int y, int count, int xMult, int yMult, int clr, Uint32 delay)
		: AnimateScaling(grid, x, y, count, xMult, yMult, clr, delay, true)
	{
	}
};

class AnimateAddition :  public AnimateScaling
{
public:
	AnimateAddition(Grid& grid, int x, int y, int count, int xMult, int yMult, int clr, Uint32 delay)
		: AnimateScaling(grid, x, y, count, xMult, yMult, clr, delay, false)
	{
	}
};

class AnimateHorzRemoval : public AnimateRemoval
{
public:
	AnimateHorzRemoval(Grid& grid, int x, int y, int count, int clr, Uint32 delay)
		: AnimateRemoval(grid, x, y, count, 1, 0, clr, delay)
	{
	}
};

class AnimateVertRemoval : public AnimateRemoval
{
public:
	AnimateVertRemoval(Grid& grid, int x, int y, int count, int clr, Uint32 delay)
		: AnimateRemoval(grid, x, y, count, 0, 1, clr, delay)
	{
	}
};

class AnimateSlide : public Animation
{
public:
	AnimateSlide(Grid& grid, int x, int y1, int y2, int* column, Uint32 duration, Uint32 delay);
		
	bool Draw();

	static const Uint32 STEP_DURATION;

private:
	Grid& m_grid;
	int m_x;
	int m_y1;
	int m_y2;	
	int m_length;
	int m_column[GRID_HEIGHT];
	Uint32 m_animStartTS;
	Uint32 m_duration;
	Uint32 m_delay;
};
