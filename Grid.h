#pragma once

#include <SDL_rect.h>
#include <vector>

class Objects;
class Animations;
struct SDL_Renderer;

const int GRID_WIDTH  = 8;
const int GRID_HEIGHT = 8;
const int RND_CELL = -1;

class Grid
{
public:
	Grid(SDL_Renderer* rend, Objects& obj, Animations& anim, const SDL_Rect& pos);
	Grid(SDL_Renderer* rend, Objects& obj, Animations& anim, const SDL_Rect& pos, int cells[GRID_WIDTH][GRID_HEIGHT]);

	void NewGame();
	int GetScore() { return m_score; }

	bool CellFromMouseCoord(int x, int y, SDL_Point& pt);
	void Select(int x, int y);
	bool HasSelection()	{ return m_selection; }
	bool Swap(int x, int y);

	void RedrawOld();
	void Redraw();

	int ObjectX(int cellX) { return m_pos.x + cellX * ObjectWidth(); }
	int ObjectY(int cellY) { return m_pos.y + cellY * ObjectHeight(); }
	int ObjectWidth() {	return m_pos.w / GRID_WIDTH; }
	int ObjectHeight() { return m_pos.h / GRID_HEIGHT; }

	void DrawObject(int x, int y, int idx, double scale = 1);

	void Move(const SDL_Rect& pos, bool redraw = true);

	void ClearRect(const SDL_Rect& rc);
	void ClearRect(const SDL_Rect& rc, const SDL_Color& clr);

private:
	void DrawSelection();
	void ClearSelection();	

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

	void RemoveRanges(const TRanges& ranges);
	bool CanRemove(int x, int y);
	void Randomize();
	int GetRemovedRanges(TRanges& out);

	SDL_Renderer* m_rend;
	Objects& m_objects;
	Animations& m_animations;
	SDL_Rect m_pos;
	int m_oldCells[GRID_WIDTH][GRID_HEIGHT];
	int m_cells[GRID_WIDTH][GRID_HEIGHT];
	SDL_Point m_selected;
	bool m_selection;
	Uint32 m_accumDelay;
	int m_score;
};