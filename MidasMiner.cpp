#include <SDL.h>
#include <SDL_image.h>

#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <list>
#include <memory>
#include <ctime>

static const char WINDOW_CAPTION[] = "Midas Miner";

static const int GRID_WIDTH  = 8;
static const int GRID_HEIGHT = 8;

static const int OBJ_WIDTH = 40;
static const int OBJ_HEIGHT = 40;
static const int OBJ_COUNT = 5;

static const char* OBJ_NAMES[OBJ_COUNT] = { "Blue.png", "Green.png", "Purple.png", "Red.png", "Yellow.png" };
static const SDL_Point OBJ_SIZES[OBJ_COUNT] = { { 35, 36 }, { 35, 35 }, { 35, 35 }, { 34, 36 }, { 38, 37 } };

static const int RND_CELL = -1;
static const int MIN_RANGE = 3;

class Image
{
public:
	Image() : m_text(0)
	{
	}

	bool Load(SDL_Renderer* rend, const char* name)
	{
		m_text = IMG_LoadTexture(rend, name);

		return m_text != 0;
	}

	~Image()
	{
		SDL_DestroyTexture(m_text);
	}

	SDL_Texture* Texture() { assert(m_text); return m_text; }

private:
	SDL_Texture* m_text;
};

class Objects
{
public:
	bool Load(SDL_Renderer* rend)
	{
		for (int i = 0; i < OBJ_COUNT; ++i)
		{
			if (!m_images[i].Load(rend, OBJ_NAMES[i]))
				return false;
		}

		return true;
	}

	int Count() { return OBJ_COUNT; }

	void DrawTexture(SDL_Renderer* rend, int x, int y, int idx, double scale = 1)
	{
		const int obj_width  = int(Size(idx).x * scale);
		const int obj_height = int(Size(idx).y * scale);
		const int x_adj = (OBJ_WIDTH  - obj_width)  / 2;
		const int y_adj = (OBJ_HEIGHT - obj_height) / 2;
		const SDL_Rect dest = { x + x_adj, y + y_adj, obj_width, obj_height };
		SDL_RenderCopy(rend, Texture(idx), NULL,  &dest);
	}

private:
	SDL_Texture* Texture(int idx) { assert(idx < OBJ_COUNT); return m_images[idx].Texture(); }
	const SDL_Point& Size(int idx)  { assert(idx < OBJ_COUNT); return OBJ_SIZES[idx]; }

	Image m_images[OBJ_COUNT];
};

class Animation
{
public:
	virtual ~Animation() {}
	virtual bool Draw(SDL_Renderer* rend) = 0;
};

class Animations
{
public:
	~Animations()
	{
		m_animEvent = SDL_RegisterEvents(1);
		Cancel();
	}

	void AddAnimation(std::auto_ptr<Animation> anim)
	{
		m_animations.push_back(anim.release());
		SDL_Event event;
		SDL_zero(event);
		event.type = m_animEvent;
		SDL_PushEvent(&event);
	}

	bool Active() const { return !m_animations.empty(); }

	bool Draw(SDL_Renderer* rend)
	{
		std::list<Animation*>::iterator it = m_animations.begin();

		while (it != m_animations.end())
		{
			if ((*it)->Draw(rend))
			{
				delete *it;
				it = m_animations.erase(it);
			}
			else
			{
				++it;
			}
		}

		SDL_RenderPresent(rend);

		return m_animations.empty();
	}

	void Cancel()
	{
		std::list<Animation*>::iterator it = m_animations.begin();
		
		while (it != m_animations.end())
		{
			delete *it;
			it = m_animations.erase(it);
		}
	}

private:
	std::list<Animation*> m_animations;
	Uint32 m_animEvent;
};

class AnimateSwap : public Animation
{
public:
	AnimateSwap(Objects& objects, int x1, int y1, int clr1, int x2, int y2, int clr2, bool wrong)
		: m_objects(objects)
		, m_clr1(clr1)
		, m_clr2(clr2)
		, m_wrong(wrong)
		, m_swapped(false)
	{
		m_rc.x = std::min(x1, x2) * OBJ_WIDTH;
		m_rc.y = std::min(y1, y2) * OBJ_HEIGHT;
		m_rc.w = (abs(x1 - x2) + 1) * OBJ_WIDTH;
		m_rc.h = (abs(y1 - y2) + 1) * OBJ_HEIGHT;

		if (x1 > x2 || y1 > y2) std::swap(m_clr1, m_clr2);

		m_animStartTS = SDL_GetTicks();
	}

	virtual bool Draw(SDL_Renderer* rend)
	{
		const Uint32 ts = SDL_GetTicks();

		const double durMult = m_wrong ? 2.0 : 1.0;

		double pos = (ts - m_animStartTS) / (DURATION / durMult);
		
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

		SDL_SetRenderDrawColor(rend, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(rend, &m_rc);

		m_objects.DrawTexture(rend, m_rc.x + int(pos * (m_rc.w - OBJ_WIDTH) + 0.5), m_rc.y + int(pos * (m_rc.h - OBJ_HEIGHT) + 0.5), m_clr1);
		m_objects.DrawTexture(rend, m_rc.x + m_rc.w - OBJ_WIDTH - int(pos * (m_rc.w - OBJ_WIDTH) + 0.5), m_rc.y + m_rc.h - OBJ_HEIGHT - int(pos * (m_rc.h - OBJ_HEIGHT) + 0.5), m_clr2);

		return ret;
	}

	static const Uint32 DURATION;

private:
	Objects& m_objects;
	int m_clr1, m_clr2;
	bool m_wrong;

	Uint32 m_animStartTS;
	SDL_Rect m_rc;
	bool m_swapped;	
};

const Uint32 AnimateSwap::DURATION = 500;

class AnimateScaling : public Animation
{
public:
	AnimateScaling(Objects& objects, int x, int y, int count, int xMult, int yMult, int clr, Uint32 delay, bool scaleDown)
		: m_objects(objects)
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

	virtual bool Draw(SDL_Renderer* rend)
	{
		const Uint32 ts = SDL_GetTicks();

		int x = m_x * OBJ_WIDTH;
		int y = m_y * OBJ_HEIGHT;

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

		SDL_Rect rc = { x, y, OBJ_WIDTH + m_xMult * (m_count - 1) * OBJ_WIDTH, OBJ_HEIGHT + m_yMult * (m_count - 1) * OBJ_HEIGHT };
		SDL_SetRenderDrawColor(rend, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(rend, &rc);

		for (int i = 0; i < m_count; i++, x += (m_xMult * OBJ_WIDTH), y += (m_yMult * OBJ_HEIGHT))
			m_objects.DrawTexture(rend, x, y, m_clr, scale);

		return ret;
	}

	static const Uint32 DURATION;

private:
	Objects& m_objects;
	int m_x, m_y;	
	int m_count;
	int m_xMult, m_yMult;
	int m_clr;
	bool m_scaleDown;
	Uint32 m_animStartTS;
	Uint32 m_delay;
};

const Uint32 AnimateScaling::DURATION = 500;

class AnimateRemoval :  public AnimateScaling
{
public:
	AnimateRemoval(Objects& objects, int x, int y, int count, int xMult, int yMult, int clr, Uint32 delay)
		: AnimateScaling(objects, x, y, count, xMult, yMult, clr, delay, true)
	{
	}
};

class AnimateAddition :  public AnimateScaling
{
public:
	AnimateAddition(Objects& objects, int x, int y, int count, int xMult, int yMult, int clr, Uint32 delay)
		: AnimateScaling(objects, x, y, count, xMult, yMult, clr, delay, false)
	{
	}
};

class AnimateHorzRemoval : public AnimateRemoval
{
public:
	AnimateHorzRemoval(Objects& objects, int x, int y, int count, int clr, Uint32 delay)
		: AnimateRemoval(objects, x, y, count, 1, 0, clr, delay)
	{
	}
};

class AnimateVertRemoval : public AnimateRemoval
{
public:
	AnimateVertRemoval(Objects& objects, int x, int y, int count, int clr, Uint32 delay)
		: AnimateRemoval(objects, x, y, count, 0, 1, clr, delay)
	{
	}
};

class AnimateSlide : public Animation
{
public:
	AnimateSlide(Objects& objects, int x, int y1, int y2, int* column, Uint32 duration, Uint32 delay)
		: m_objects(objects)
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
		
	bool Draw(SDL_Renderer* rend)
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

		int y = (m_y1 - m_length) * OBJ_HEIGHT;

		SDL_Rect rc = { m_x * OBJ_WIDTH, y, OBJ_WIDTH, OBJ_HEIGHT * (m_length + m_y2 - m_y1 + 1)  };
		SDL_SetRenderDrawColor(rend, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(rend, &rc);

		const int shift = int(pc * (m_y2 - m_y1 + 1) * OBJ_HEIGHT);
		y += shift;

		for (int i = 0; i < m_length; ++i, y += OBJ_HEIGHT)
		{
			m_objects.DrawTexture(rend, m_x * OBJ_WIDTH, y, m_column[i]);
		}

		return ret;
	}

	static Uint32 STEP_DURATION;

private:
	Objects& m_objects;
	int m_x;
	int m_y1;
	int m_y2;	
	int m_length;
	int m_column[GRID_HEIGHT];
	Uint32 m_animStartTS;
	Uint32 m_duration;
	Uint32 m_delay;
};

Uint32 AnimateSlide::STEP_DURATION = 150; 

class Grid
{
public:
	Grid(Objects& obj, Animations& anim)
		: m_objects(obj)
		, m_animations(anim)
		, m_selection(false)
		, m_accumDelay(0)
	{		
		for (int x = 0; x < GRID_WIDTH; ++x)
			for (int y = 0; y < GRID_HEIGHT; ++y)
				m_cells[x][y] = RND_CELL;

		Randomize();
	}

	void Select(int x1, int y1)
	{
		m_selected.x = x1;
		m_selected.y = y1;
		m_selection = true;
	}

	bool HasSelection()
	{
		return m_selection;
	}

	bool Swap(int x, int y)
	{
		if (!m_selection) return false;

		m_selection = false;

		int dx = abs(m_selected.x - x);
		int dy = abs(m_selected.y - y);

		if ((dx + dy) > 1)
			return false;

		int& clr1 = m_cells[m_selected.x][m_selected.y];
		int& clr2 = m_cells[x][y];

		if (clr1 == clr2)
		{
			m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateSwap(m_objects, m_selected.x, m_selected.y, clr1, x, y, clr2, true)));
			return false;
		}

		std::swap(clr1, clr2);

		m_accumDelay = AnimateSwap::DURATION;

		TRanges ranges;
		GetCollapsedRanges(ranges);		

		if (ranges.empty())
		{
			std::swap(clr1, clr2);
			m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateSwap(m_objects, m_selected.x, m_selected.y, clr1, x, y, clr2, true)));
			return false;
		}

		m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateSwap(m_objects, m_selected.x, m_selected.y, clr2, x, y, clr1, false)));		

		do
		{
			RemoveRanges(ranges);			
			Randomize();			
		}
		while (GetCollapsedRanges(ranges));		

		return true;
	}

	void Draw(SDL_Renderer* rend, int x = 0, int y = 0)
	{
		const int width = GRID_WIDTH * OBJ_WIDTH;
		const int height = GRID_HEIGHT * OBJ_HEIGHT;

		const SDL_Rect boundary = { 0, 0, width, height };

		SDL_SetRenderDrawColor(rend, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(rend, &boundary);

		for (int x = 0; x < GRID_WIDTH; ++x)
			for (int y = 0; y < GRID_HEIGHT; ++y)			
			{
				m_objects.DrawTexture(rend, x * OBJ_WIDTH, y * OBJ_HEIGHT, m_cells[x][y]);
			}

		if (m_selection)
		{
			const SDL_Rect outline = { m_selected.x * OBJ_WIDTH, m_selected.y * OBJ_HEIGHT, OBJ_WIDTH, OBJ_HEIGHT };
			SDL_SetRenderDrawColor(rend, 255, 255, 255, SDL_ALPHA_OPAQUE);
			SDL_RenderDrawRect(rend, &outline);
		}

		SDL_RenderPresent(rend);
	}

private:
	struct Range
	{
		int x;
		int y1;
		int y2;

		bool operator<(const Range& r) const
		{
			return (x < r.x) || (x == r.x && y1 < r.y1);
		}
	};

	typedef std::vector<Range> TRanges;

	void RemoveRanges(const TRanges& ranges)
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

				m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateSlide(m_objects, r.x, r.y1, r.y2, &m_cells[r.x][len], animLen, m_accumDelay)));
				addDelay = true;
			}

			prevX = r.x;
			
			for (int i = 0; i < len; ++i)
				m_cells[r.x][i] = RND_CELL;			
		}

		if (addDelay)
			m_accumDelay += maxAnimLen;
	}

	bool CanCollapse(int x, int y)
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

	void Randomize()
	{
		srand((unsigned)time(NULL));

		bool addDelay = false;

		for (int x = 0; x < GRID_WIDTH; ++x)
			for (int y = 0; y < GRID_HEIGHT; ++y)
				if (m_cells[x][y] == RND_CELL)
				{
					do
					{
						m_cells[x][y] = rand() % OBJ_COUNT;
					}
					while (CanCollapse(x, y));

					m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateAddition(m_objects, x, y, 1, 1, 0, m_cells[x][y], m_accumDelay)));
					addDelay = true;
				}

		if (addDelay)
			m_accumDelay += AnimateScaling::DURATION;
	}

	int GetCollapsedRanges(TRanges& out)
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
					m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateVertRemoval(m_objects, x, yStart, y - yStart, m_cells[x][yStart], m_accumDelay)));
				}
				
				yStart = y;				
			}

			if ((y - yStart) >= MIN_RANGE)
			{
				Range r = { x, yStart, y - 1 };
				ranges.push_back(r);
				m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateVertRemoval(m_objects, x, yStart, y - yStart, m_cells[x][yStart], m_accumDelay)));
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

					m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateHorzRemoval(m_objects, xStart, y, x - xStart, m_cells[xStart][y], m_accumDelay)));
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

				m_animations.AddAnimation(std::auto_ptr<Animation>(new AnimateHorzRemoval(m_objects, xStart, y, x - xStart, m_cells[xStart][y], m_accumDelay)));
			}
		}

		sort(ranges.begin(), ranges.end());

		TRanges::iterator it = ranges.begin();
		while (it != ranges.end() && it != (--ranges.end()))
		{
			Range& cur = *it;
			Range& next = *(++it);

			if (cur.x == next.x)
			{
				if (cur.y2 >= next.y1)
				{
					cur.y2 = next.y2;
					it = ranges.erase(it);
				}
			}
		}

		out.swap(ranges);

		if (out.size())
			m_accumDelay += AnimateRemoval::DURATION;

		return out.size();
	}

	Objects& m_objects;
	Animations& m_animations;
	int m_cells[GRID_WIDTH][GRID_HEIGHT];
	SDL_Point m_selected;
	bool m_selection;
	Uint32 m_accumDelay;
};

SDL_Point CellFromMouseCoord(int x, int y)
{
	SDL_Point ret = { x / OBJ_WIDTH, y / OBJ_HEIGHT };
	return ret;
}

int main(int argc, char* argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, WINDOW_CAPTION, SDL_GetError(), NULL);
		return -1;
	}

	atexit(SDL_Quit);

	if (!IMG_Init(IMG_INIT_PNG))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, WINDOW_CAPTION, IMG_GetError(), NULL);
		return -1;
	}

	atexit(IMG_Quit);	

	SDL_Window* win;
	SDL_Renderer* rend;
	if (SDL_CreateWindowAndRenderer(GRID_WIDTH * OBJ_WIDTH, GRID_HEIGHT * OBJ_HEIGHT, SDL_WINDOW_SHOWN, &win, &rend))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, WINDOW_CAPTION, SDL_GetError(), NULL);
		return -1;
	}

	SDL_SetWindowTitle(win, WINDOW_CAPTION);

	SDL_Surface* icon = IMG_Load("Purple.png");

	if (!icon)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, WINDOW_CAPTION, IMG_GetError(), NULL);
		return -1;
	}

	SDL_SetWindowIcon(win, icon);
	
	SDL_SetRenderDrawColor(rend, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(rend);
	SDL_RenderPresent(rend);

	Objects objects;
	if (!objects.Load(rend))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, WINDOW_CAPTION, IMG_GetError(), NULL);
		return -1;
	}

	Animations anim;

	Grid grid(objects, anim);
	grid.Draw(rend);

	for (;;)
	{
		SDL_Event event;
		bool haveEvent = (SDL_WaitEventTimeout(&event, 33) != 0);

		if (haveEvent && event.type == SDL_QUIT)
			break;

		if (anim.Active())
		{
			if (anim.Draw(rend))
				grid.Draw(rend);
			else
				continue;
		}

		if (!haveEvent) continue;

		if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
		{
			const SDL_Point cell = CellFromMouseCoord(event.button.x, event.button.y);
			
			if (grid.HasSelection())
			{
				grid.Swap(cell.x, cell.y);				
			}
			else
			{
				grid.Select(cell.x, cell.y);
				grid.Draw(rend);
			}
		}

		if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_EXPOSED)
		{
			if (anim.Active()) anim.Cancel();
			grid.Draw(rend);
		}
	}

	SDL_FreeSurface(icon);
	SDL_DestroyRenderer(rend);
	SDL_DestroyWindow(win);

	return 0;
}
