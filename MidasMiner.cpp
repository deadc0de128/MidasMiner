#include "Animations.h"
#include "Objects.h"
#include "Grid.h"

#include <SDL.h>
#include <SDL_image.h>

static const char WINDOW_CAPTION[] = "Midas Miner";
static const SDL_Color CLEAR_COLOR = {   0,   0,   0, SDL_ALPHA_OPAQUE };
static const Uint32 GAME_LEN = 60000; // 60 sec

void ClearWindow(SDL_Renderer* rend)
{
	SDL_SetRenderDrawColor(rend, CLEAR_COLOR.r, CLEAR_COLOR.g, CLEAR_COLOR.b, CLEAR_COLOR.a);
	SDL_RenderClear(rend);
}

void GetGridRect(SDL_Window* win, SDL_Rect* gridPos)
{
	SDL_zero(*gridPos);
	SDL_GetWindowSize(win, &gridPos->w, &gridPos->h);

	if (gridPos->w > gridPos->h)
	{
		gridPos->x = gridPos->x + (gridPos->w - gridPos->h) / 2;
		gridPos->w = gridPos->h;
	}
	else if (gridPos->h > gridPos->w)
	{
		gridPos->y = gridPos->y + (gridPos->h - gridPos->w) / 2;
		gridPos->h = gridPos->w;
	}
}

static Uint32 END_GAME_EVENT;

static Uint32 TimerCallback(Uint32 interval, void *param)
{
	SDL_Event event;
	SDL_zero(event);
	event.type = END_GAME_EVENT;
	SDL_PushEvent(&event);
	return interval;
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
	if (SDL_CreateWindowAndRenderer(GRID_WIDTH * OBJ_WIDTH, GRID_HEIGHT * OBJ_HEIGHT, SDL_WINDOW_RESIZABLE, &win, &rend))
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

	ClearWindow(rend);

	Objects objects;
	if (!objects.Load(rend))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, WINDOW_CAPTION, IMG_GetError(), NULL);
		return -1;
	}

	Animations anim;

	SDL_Rect gridPos = { 0, 0, 0, 0 };
	GetGridRect(win, &gridPos);

/*	int cells[GRID_WIDTH][GRID_HEIGHT] = { { 0, 1, 2, 3, 4, 0, 1, 2 },
										   { 4, 3, 0, 1, 0, 4, 3, 0 },
										   { 0, 1, 2, 3, 4, 0, 1, 2 },
										   { 4, 3, 0, 1, 0, 4, 3, 0 },
										   { 0, 1, 2, 3, 4, 0, 1, 2 },
										   { 4, 3, 0, 1, 0, 4, 3, 0 },
										   { 0, 1, 2, 3, 4, 0, 1, 2 },
										   { 4, 3, 0, 1, 0, 4, 3, 0 } };*/

/*	int cells[GRID_WIDTH][GRID_HEIGHT] = { { 0, 1, 2, 3, 4, 0, 1, 2 },
										   { 4, 3, 0, 1, 0, 4, 3, 0 },
										   { 0, 1, 2, 3, 4, 0, 1, 2 },
										   { 4, 3, 2, 2, 3, 2, 3, 0 },
										   { 0, 1, 2, 3, 4, 0, 1, 2 },
										   { 4, 3, 0, 1, 0, 4, 3, 0 },
										   { 0, 1, 2, 3, 4, 0, 1, 2 },
										   { 4, 3, 0, 1, 0, 4, 3, 0 } };

	int cells[GRID_WIDTH][GRID_HEIGHT] = { { 0, 1, 2, 3, 4, 0, 1, 2 },
										   { 4, 3, 0, 1, 0, 4, 3, 0 },
										   { 0, 1, 2, 3, 4, 0, 1, 2 },
										   { 4, 3, 3, 2, 3, 2, 3, 0 },
										   { 0, 1, 2, 3, 4, 0, 1, 2 },
										   { 4, 3, 0, 1, 0, 4, 3, 0 },
										   { 0, 1, 2, 3, 4, 0, 1, 2 },
										   { 4, 3, 0, 1, 0, 4, 3, 0 } };*/

	Grid grid(rend, objects, anim, gridPos);
	grid.Redraw();

	SDL_RenderPresent(rend);

	END_GAME_EVENT = SDL_RegisterEvents(1);
	const SDL_TimerID idTimer = SDL_AddTimer(GAME_LEN, TimerCallback, 0);

	for (;;)
	{
		SDL_Event event;
		bool haveEvent = (SDL_WaitEventTimeout(&event, 33) != 0);

		if (haveEvent && event.type == SDL_QUIT)
			break;

		if (anim.Active())
		{
			ClearWindow(rend);
			grid.RedrawOld();
			anim.Draw(rend);

			if (!anim.Active())
				grid.Redraw();

			SDL_RenderPresent(rend);
		}

		if (!haveEvent) continue;

		if (event.type == END_GAME_EVENT)
		{
			char buf[256];
			sprintf(buf, "Time is up. You got: %i", grid.GetScore());
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, WINDOW_CAPTION, buf, win);
			grid.NewGame();
		}
		else if (!anim.Active() && event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
		{
			SDL_Point cell;
			const bool insideGrid = grid.CellFromMouseCoord(event.button.x, event.button.y, cell);

			if (insideGrid)
			{
				ClearWindow(rend);

				if (grid.HasSelection())
				{
					grid.Swap(cell.x, cell.y);
				}
				else
				{
					grid.Select(cell.x, cell.y);
				}

				if (!anim.Active())
					SDL_RenderPresent(rend);
			}
		}
		else if (event.type == SDL_WINDOWEVENT)
		{
			if (event.window.event == SDL_WINDOWEVENT_EXPOSED ||
				event.window.event == SDL_WINDOWEVENT_RESIZED)
			{
				if (anim.Active()) anim.Cancel();
				ClearWindow(rend);
				SDL_Rect gridPos;
				GetGridRect(win, &gridPos);
				grid.Move(gridPos);
				SDL_RenderPresent(rend);
			}
		}
	}

	SDL_RemoveTimer(idTimer);
	SDL_FreeSurface(icon);
	SDL_DestroyRenderer(rend);
	SDL_DestroyWindow(win);

	return 0;
}
