#include "Grid.h"
#include "Objects.h"
#include "Animations.h"

#include <SDL.h>
#include <ctime>
#include <algorithm>

static const int MIN_RANGE = 3;

static const SDL_Color SEL_COLOR   = { 255, 255, 255, SDL_ALPHA_OPAQUE };
static const SDL_Color CLEAR_COLOR = {   0,   0,   0, SDL_ALPHA_OPAQUE };

Grid::Grid(SDL_Renderer* rend, Objects& obj, Animations& anim, const SDL_Rect& pos)
	: m_rend(rend)
	, m_objects(obj)
	, m_animations(anim)
	, m_pos(pos)
	, m_selection(false)
	, m_accumDelay(0)
	, m_score(0)
{
	NewGame();
}

Grid::Grid(SDL_Renderer* rend, Objects& obj, Animations& anim, const SDL_Rect& pos, int cells[GRID_WIDTH][GRID_HEIGHT])
	: m_rend(rend)
	, m_objects(obj)
	, m_animations(anim)
	, m_pos(pos)
	, m_selection(false)
	, m_accumDelay(0)
	, m_score(0)
{
	SDL_zero(m_oldCells);
	memcpy(m_cells, cells, sizeof(m_cells));
}

void Grid::NewGame()
{
	m_accumDelay = 0;

	SDL_zero(m_oldCells);

	for (int x = 0; x < GRID_WIDTH; ++x)
		for (int y = 0; y < GRID_HEIGHT; ++y)
			m_cells[x][y] = RND_CELL;

	Randomize();	

	m_score = 0;
}

bool Grid::CellFromMouseCoord(int x, int y, SDL_Point& pt)
{
	const int gridX = x - m_pos.x;
	const int gridY = y - m_pos.y;

	if (gridX < 0 || gridY < 0 || gridX >= m_pos.w || gridY >= m_pos.h)
		return false;

	const SDL_Point ret = { gridX / ObjectWidth(), gridY / ObjectHeight() };
	pt = ret;
	return true;
}

void Grid::Select(int x, int y)
{
	ClearSelection();
		
	m_selected.x = x;
	m_selected.y = y;
	m_selection = true;
		
	Redraw();
	DrawSelection();	
}

bool Grid::Swap(int x, int y)
{
	if (!m_selection) return false;		

	int dx = abs(m_selected.x - x);
	int dy = abs(m_selected.y - y);

	ClearSelection();

	if ((dx + dy) > 1)
	{
		Redraw();
		return false;
	}
		
	int& clr1 = m_cells[m_selected.x][m_selected.y];
	int& clr2 = m_cells[x][y];

	memcpy(m_oldCells, m_cells, sizeof(m_oldCells));

	if (clr1 == clr2)
	{
		m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateSwap(*this, m_selected.x, m_selected.y, clr1, x, y, clr2, true, 0)));
		return false;
	}

	std::swap(clr1, clr2);

	m_accumDelay = AnimateSwap::DURATION;

	m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateSwap(*this, m_selected.x, m_selected.y, clr2, x, y, clr1, false, 0)));		

	TRanges ranges;
	GetRemovedRanges(ranges);		

	if (ranges.empty())
	{
		std::swap(clr1, clr2);
		m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateSwap(*this, m_selected.x, m_selected.y, clr2, x, y, clr1, false, m_accumDelay)));
		return false;
	}	

	do
	{
		RemoveRanges(ranges);			
		Randomize();			
	}
	while (GetRemovedRanges(ranges));		

	return true;
}

void Grid::Redraw()
{
	SDL_SetRenderDrawColor(m_rend, CLEAR_COLOR.r, CLEAR_COLOR.g, CLEAR_COLOR.b, CLEAR_COLOR.a);
	SDL_RenderFillRect(m_rend, &m_pos);

	for (int x = 0; x < GRID_WIDTH; ++x)
	{
		for (int y = 0; y < GRID_HEIGHT; ++y)			
		{
			DrawObject(ObjectX(x), ObjectY(y), m_cells[x][y]);
		}
	}

	DrawSelection();
}

void Grid::RedrawOld()
{
	SDL_SetRenderDrawColor(m_rend, CLEAR_COLOR.r, CLEAR_COLOR.g, CLEAR_COLOR.b, CLEAR_COLOR.a);
	SDL_RenderFillRect(m_rend, &m_pos);

	for (int x = 0; x < GRID_WIDTH; ++x)
	{
		for (int y = 0; y < GRID_HEIGHT; ++y)			
		{
			DrawObject(ObjectX(x), ObjectY(y), m_oldCells[x][y]);
		}
	}

	DrawSelection();
}

void Grid::DrawSelection()
{
	if (!m_selection) return;

	const SDL_Rect outline = { ObjectX(m_selected.x), ObjectY(m_selected.y),									   
							   ObjectWidth(), ObjectHeight() };
	SDL_SetRenderDrawColor(m_rend, SEL_COLOR.r, SEL_COLOR.g, SEL_COLOR.b, SEL_COLOR.a);
	SDL_RenderDrawRect(m_rend, &outline);
}

void Grid::ClearSelection()
{
	if (!m_selection) return;

	const SDL_Rect outline = { ObjectX(m_selected.x), ObjectY(m_selected.y),									   
							   ObjectWidth(), ObjectHeight() };

	SDL_SetRenderDrawColor(m_rend, CLEAR_COLOR.r, CLEAR_COLOR.g, CLEAR_COLOR.b, CLEAR_COLOR.a);
	SDL_RenderFillRect(m_rend, &outline);

	DrawObject(outline.x, outline.y, m_cells[m_selected.x][m_selected.y]);
		
	m_selection = false;
}

void Grid::RemoveRanges(const TRanges& ranges)
{
	TRanges::const_iterator it = ranges.begin();

	int prevX = -1;
	bool addDelay = false;
	Uint32 maxAnimLen = 0;

	for (; it != ranges.end(); ++it)
	{
		const Range& r = *it;

		int len = r.y2 - r.y1 + 1;

		memmove(&m_cells[r.x][len], &m_cells[r.x][0], sizeof(m_cells[0][0]) * r.y1);

		if (r.y1)
		{
			const Uint32 animLen = len * AnimateSlide::STEP_DURATION;

			if (prevX == r.x)
				m_accumDelay += animLen;

			maxAnimLen = std::max(animLen, maxAnimLen);

			m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateSlide(*this, r.x, r.y1, r.y2, &m_cells[r.x][len], animLen, m_accumDelay)));
			addDelay = true;
		}

		prevX = r.x;
			
		for (int i = 0; i < len; ++i)
			m_cells[r.x][i] = RND_CELL;			
	}

	if (addDelay)
		m_accumDelay += maxAnimLen;
}

bool Grid::CanRemove(int x, int y)
{
	int xcount = 0;
	int ycount = 0;

	const int clr = m_cells[x][y];

	for (int i = x; i >= std::max(0, x - MIN_RANGE + 1); --i)
		if (m_cells[i][y] == clr) ++xcount; else break;

	for (int i = x; i < std::min(GRID_WIDTH, x + MIN_RANGE); ++i)
		if (m_cells[i][y] == clr) ++xcount; else break;

	if (xcount >= MIN_RANGE) return true;

	for (int i = y; i >= std::max(0, y - MIN_RANGE + 1); --i)
		if (m_cells[x][i] == clr) ++ycount; else break;

	for (int i = y; i < std::min(GRID_HEIGHT, y + MIN_RANGE); ++i)
		if (m_cells[x][i] == clr) ++ycount; else break;

	if (ycount >= MIN_RANGE) return true;

	return false;
}

void Grid::Randomize()
{
	srand((unsigned)time(NULL));

	bool addDelay = false;

	for (int x = 0; x < GRID_WIDTH; ++x)
	{
		for (int y = 0; y < GRID_HEIGHT; ++y)
		{
			if (m_cells[x][y] == RND_CELL)
			{
				m_score += 10;

				do
				{
					m_cells[x][y] = rand() % OBJ_COUNT;					
				}
				while (CanRemove(x, y));

				m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateAddition(*this, x, y, 1, 1, 0, m_cells[x][y], m_accumDelay)));
				addDelay = true;
			}
		}
	}

	if (addDelay)
		m_accumDelay += AnimateScaling::DURATION;
}

int Grid::GetRemovedRanges(TRanges& out)
{
	TRanges ranges;

	for (int x = 0; x < GRID_WIDTH; ++x)
	{
		int yStart = 0, y = 1;

		for (; y < GRID_HEIGHT; ++y)
		{
			if (m_cells[x][yStart] == m_cells[x][y])
				continue;
				
			if ((y - yStart) >= MIN_RANGE)
			{
				Range r = { x, yStart, y - 1 };
				ranges.push_back(r);
				m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateVertRemoval(*this, x, yStart, y - yStart, m_cells[x][yStart], m_accumDelay)));
			}
				
			yStart = y;				
		}

		if ((y - yStart) >= MIN_RANGE)
		{
			Range r = { x, yStart, y - 1 };
			ranges.push_back(r);
			m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateVertRemoval(*this, x, yStart, y - yStart, m_cells[x][yStart], m_accumDelay)));
		}
	}

	for (int y = 0; y < GRID_HEIGHT; ++y)
	{
		int xStart = 0, x = 1;

		for (; x < GRID_WIDTH; ++x)
		{
			if (m_cells[xStart][y] == m_cells[x][y])
				continue;
				
			if ((x - xStart) >= MIN_RANGE)
			{
				for (int i = xStart; i < x; ++i)
				{
					Range r = { i, y, y };
					ranges.push_back(r);						
				}

				m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateHorzRemoval(*this, xStart, y, x - xStart, m_cells[xStart][y], m_accumDelay)));
			}

			xStart = x;
		}

		if ((x - xStart) >= MIN_RANGE)
		{
			for (int i = xStart; i < x; ++i)
			{
				Range r = { i, y, y };
				ranges.push_back(r);
			}

			m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateHorzRemoval(*this, xStart, y, x - xStart, m_cells[xStart][y], m_accumDelay)));
		}
	}

	sort(ranges.begin(), ranges.end());

	TRanges::iterator it = ranges.begin();
	while (it != ranges.end() && it != (--ranges.end()))
	{
		Range& cur = *it;
		Range& next = *(++it);

		if (cur.x == next.x && cur.y2 >= next.y1)
		{
			cur.y1 = std::min(cur.y1, next.y1);
			cur.y2 = std::max(cur.y2, next.y2);
			it = ranges.erase(it);
			--it;
		}
	}

	out.swap(ranges);

	if (out.size())
		m_accumDelay += AnimateRemoval::DURATION;

	return out.size();
}

void Grid::DrawObject(int x, int y, int idx, double scale)
{
	m_objects.DrawTexture(m_rend, x, y, ObjectWidth(), ObjectHeight(), idx, scale);
}

void Grid::Move(const SDL_Rect& pos, bool redraw)
{
	m_pos = pos;

	if (redraw) Redraw();
}

void Grid::ClearRect(const SDL_Rect& rc)
{
	ClearRect(rc, CLEAR_COLOR);
}

void Grid::ClearRect(const SDL_Rect& rc, const SDL_Color& clr)
{
	SDL_SetRenderDrawColor(m_rend, clr.r, clr.g, clr.b, clr.a);
	SDL_RenderFillRect(m_rend, &rc);
}