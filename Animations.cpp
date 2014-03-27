#include "Animations.h"
#include "Objects.h"
#include "Grid.h"

#include <SDL.h>

const Uint32 AnimateSwap::DURATION = 500;
const Uint32 AnimateScaling::DURATION = 500;
const Uint32 AnimateSlide::STEP_DURATION = 150; 

Animations::~Animations()
{
	m_animEvent = SDL_RegisterEvents(1);
	Cancel();
}

void Animations::AddAnimation(std::auto_ptr<Animation> anim)
{
	const bool empty = m_animations.empty();
	m_animations.push_back(anim.release());
	
	if (empty)
	{
		SDL_Event event;
		SDL_zero(event);
		event.type = m_animEvent;
		SDL_PushEvent(&event);
	}
}

void Animations::Draw(SDL_Renderer* rend)
{
	std::list<Animation*>::iterator it = m_animations.begin();

	bool active = false;

	while (it != m_animations.end())
	{
		bool completed = (*it)->Draw();
		active = active || !completed;
		++it;
	}

	if (!active)
		Cancel();	
}

void Animations::Cancel()
{
	std::list<Animation*>::iterator it = m_animations.begin();
		
	while (it != m_animations.end())
	{
		delete *it;
		it = m_animations.erase(it);
	}
}

AnimateSwap::AnimateSwap(Grid& grid, int x1, int y1, int clr1, int x2, int y2, int clr2, bool wrong, Uint32 delay)
	: m_grid(grid)
	, m_clr1(clr1)
	, m_clr2(clr2)
	, m_wrong(wrong)
	, m_delay(delay)
	, m_swapped(false)
{
	m_rc.x = m_grid.ObjectX(std::min(x1, x2));
	m_rc.y = m_grid.ObjectY(std::min(y1, y2));
	m_rc.w = (abs(x1 - x2) + 1) * m_grid.ObjectWidth();
	m_rc.h = (abs(y1 - y2) + 1) * m_grid.ObjectHeight();

	if (x1 > x2 || y1 > y2) std::swap(m_clr1, m_clr2);

	m_animStartTS = SDL_GetTicks();
}

bool AnimateSwap::Draw()
{
	const Uint32 ts = SDL_GetTicks();

	Uint32 timePassed = ts - m_animStartTS;

	if (timePassed < m_delay)
		return false;

	const double durMult = m_wrong ? 2.0 : 1.0;

	double pos = (timePassed - m_delay) / (DURATION / durMult);
		
	bool ret = false;

	if (pos > durMult)
	{
		pos = durMult;
		ret = true;
	}

	if (m_wrong && pos > 1)
	{
		pos = pos - 1;
		if (!m_swapped)
		{
			std::swap(m_clr1, m_clr2);
			m_swapped = true;
		}
	}

//	SDL_Color c = { 0, 0, 128, SDL_ALPHA_OPAQUE };
	m_grid.ClearRect(m_rc);

	const int x1 = m_rc.x + int(pos * (m_rc.w - m_grid.ObjectWidth())  + 0.5);
	const int y1 = m_rc.y + int(pos * (m_rc.h - m_grid.ObjectHeight()) + 0.5);
	const int x2 = m_rc.x + m_rc.w - m_grid.ObjectWidth()  - int(pos * (m_rc.w - m_grid.ObjectWidth())  + 0.5);
	const int y2 = m_rc.y + m_rc.h - m_grid.ObjectHeight() - int(pos * (m_rc.h - m_grid.ObjectHeight()) + 0.5);

	m_grid.DrawObject(x1, y1, m_clr1);
	m_grid.DrawObject(x2, y2, m_clr2);

	return ret;
}

AnimateScaling::AnimateScaling(Grid& grid, int x, int y, int count, int xMult, int yMult, int clr, Uint32 delay, bool scaleDown)
	: m_grid(grid)
	, m_x(x)
	, m_y(y)
	, m_count(count)
	, m_xMult(xMult)
	, m_yMult(yMult)
	, m_clr(clr)
	, m_scaleDown(scaleDown)
	, m_animStartTS(SDL_GetTicks())
	, m_delay(delay)
{
}

bool AnimateScaling::Draw()
{
	const Uint32 ts = SDL_GetTicks();

	Uint32 timePassed = ts - m_animStartTS;

	if (timePassed < m_delay)
		return false;

	double scale;
	bool ret = false;

	if (m_scaleDown)
	{
		scale = 1 - double(timePassed - m_delay) / DURATION;

		if (scale <= 0)
		{
			scale = 0;
			ret = true;
		}
	}
	else
	{
		scale = double(timePassed - m_delay) / DURATION;

		if (scale >= 1)
		{
			scale = 1;
			ret = true;
		}
	}

	int x = m_grid.ObjectX(m_x);
	int y = m_grid.ObjectY(m_y);

	const SDL_Rect rc = { x, y, 
						  m_grid.ObjectWidth()  + m_xMult * (m_count - 1) * m_grid.ObjectWidth(),
						  m_grid.ObjectHeight() + m_yMult * (m_count - 1) * m_grid.ObjectHeight() };
//	SDL_Color c = { 0, 128, 0, SDL_ALPHA_OPAQUE };
	m_grid.ClearRect(rc);

	for (int i = 0; i < m_count; i++, x += (m_xMult * m_grid.ObjectWidth()), y += (m_yMult * m_grid.ObjectHeight()))
		m_grid.DrawObject(x, y, m_clr, scale);

	return ret;
}

AnimateSlide::AnimateSlide(Grid& grid, int x, int y1, int y2, int* column, Uint32 duration, Uint32 delay)
	: m_grid(grid)
	, m_x(x)
	, m_y1(y1)
	, m_y2(y2)
	, m_animStartTS(SDL_GetTicks())
	, m_duration(duration)
	, m_delay(delay)
{
	m_length = m_y1;
		
	while (*column == RND_CELL)
	{
		++column;
		--m_length;
	}

	memcpy(m_column, column, sizeof(int) * m_length);
}
		
bool AnimateSlide::Draw()
{
	const Uint32 ts = SDL_GetTicks();

	Uint32 timePassed = ts - m_animStartTS;

	if (timePassed < m_delay)
		return false;

	timePassed -= m_delay;

	double pc = double(timePassed) / m_duration;

	bool ret = false;

	if (pc >= 1)
	{
		pc = 1;
		ret = true;
	}		

	int y = m_grid.ObjectY(m_y1 - m_length);

	const SDL_Rect rc = { m_grid.ObjectX(m_x), y, m_grid.ObjectWidth(), m_grid.ObjectHeight() * (m_length + m_y2 - m_y1 + 1)  };
//	SDL_Color c = { 128, 0, 0, SDL_ALPHA_OPAQUE };
	m_grid.ClearRect(rc);

	const int shift = int(pc * (m_y2 - m_y1 + 1) * m_grid.ObjectHeight());
	y += shift;

	for (int i = 0; i < m_length; ++i, y += m_grid.ObjectHeight())
	{
		m_grid.DrawObject(m_grid.ObjectX(m_x), y, m_column[i]);
	}

	return ret;
}
