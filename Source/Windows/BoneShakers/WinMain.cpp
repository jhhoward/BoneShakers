#include <SDL.h>
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include "Defines.h"
#include "Game.h"
#include "lodepng.h"

#define ZOOM_SCALE 5

SDL_Window* AppWindow;
SDL_Renderer* AppRenderer;
SDL_Surface* ScreenSurface;
SDL_Texture* ScreenTexture;

uint8_t InputMask = 0;

bool IsRecording = false;
int CurrentRecordingFrame = 0;

struct KeyMap
{
	SDL_Keycode key;
	uint8_t mask;
};

KeyMap KeyMappings[] =
{
	{ SDLK_LEFT, INPUT_LEFT },
	{ SDLK_RIGHT, INPUT_RIGHT },
	{ SDLK_UP, INPUT_UP },
	{ SDLK_DOWN, INPUT_DOWN },
	{ SDLK_z, INPUT_A },
	{ SDLK_x, INPUT_B },
};

void SetLED(uint8_t r, uint8_t g, uint8_t b)
{

}

void FillScreen(uint8_t colour)
{
	for (int y = 0; y < DISPLAY_HEIGHT; y++)
	{
		for (int x = 0; x < DISPLAY_WIDTH; x++)
		{
			PutPixel(x, y, colour);
		}
	}
}

void DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap,
	const uint8_t *mask, uint8_t frame, uint8_t mask_frame)
{
	uint8_t w = bitmap[0];
	uint8_t h = bitmap[1];

	bitmap += 2;

	for (int j = 0; j < h; j++)
	{
		for (int i = 0; i < w; i++)
		{
			/*int pixIndex = j * w + i;
			int block = pixIndex / 8;
			uint8_t bitmask = 1 << (pixIndex % 8);
			uint8_t pixels = bitmap[block];
			uint8_t maskPixels = mask[block];
			*/
			
			int blockX = i / 8;
			int blockY = j / 8;
			int blocksPerWidth = w / 8;
			int blockIndex = blockY * blocksPerWidth + blockX;
			uint8_t pixels = bitmap[blockIndex * 8 + i % 8];
			uint8_t maskPixels = mask[blockIndex * 8 + i % 8];
			uint8_t bitmask = 1 << (j % 8);
			
			if (maskPixels & bitmask)
			{
				if (x + i >= 0 && y + j >= 0)
				{
					if (pixels & bitmask)
					{
						PutPixel(x + i, y + j, 1);
					}
					else
					{
						PutPixel(x + i, y + j, 0);
					}
				}
			}
		}
	}

}

void PutPixel(uint8_t x, uint8_t y, uint8_t colour)
{
	if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT)
	{
		return;
	}

	SDL_Surface* surface = ScreenSurface;

	Uint32 col = colour ? SDL_MapRGBA(surface->format, 255, 255, 255, 255) : SDL_MapRGBA(surface->format, 0, 0, 0, 255);

	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	*(Uint32 *)p = col;
}

void DrawBitmap(const uint8_t* data, uint16_t x, uint16_t y, uint8_t w, uint8_t h)
{
	for (int j = 0; j < h; j++)
	{
		for (int i = 0; i < w; i++)
		{
			int blockX = i / 8;
			int blockY = j / 8;
			int blocksPerWidth = w / 8;
			int blockIndex = blockY * blocksPerWidth + blockX;
			uint8_t pixels = data[blockIndex * 8 + i % 8];
			uint8_t mask = 1 << (j % 8);
			if (x + i >= 0 && y + j >= 0)
			{
				if (pixels & mask)
				{
					PutPixel(x + i, y + j, 1);
				}
				else
				{
					PutPixel(x + i, y + j, 0);
				}
			}
		}
	}
}

void DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{
	DrawBitmap(bitmap + 2, x, y, bitmap[0], bitmap[1]);
}

void DrawSolidBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{
	DrawBitmap(bitmap + 2, x, y, bitmap[0], bitmap[1]);
}

uint8_t GetInput()
{
	return InputMask;
}


int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_CreateWindowAndRenderer(DISPLAY_WIDTH * ZOOM_SCALE, DISPLAY_HEIGHT * ZOOM_SCALE, SDL_WINDOW_RESIZABLE, &AppWindow, &AppRenderer);
	SDL_RenderSetLogicalSize(AppRenderer, DISPLAY_WIDTH, DISPLAY_HEIGHT);

	ScreenSurface = SDL_CreateRGBSurface(0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 32,
		0x000000ff,
		0x0000ff00,
		0x00ff0000,
		0xff000000
	);
	ScreenTexture = SDL_CreateTexture(AppRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, ScreenSurface->w, ScreenSurface->h);

	InitGame();
	
	bool running = true;
	int playRate = 1;

	while (running)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				running = false;
				break;
			case SDL_KEYDOWN:
				for (int n = 0; n < sizeof(KeyMappings); n++)
				{
					if (event.key.keysym.sym == KeyMappings[n].key)
					{
						InputMask |= KeyMappings[n].mask;
					}
				}
				switch (event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					running = false;
					break;
				case SDLK_TAB:
					playRate = 10;
					break;
				case SDLK_F12:
					{
						lodepng::encode(std::string("screenshot.png"), (unsigned char*)(ScreenSurface->pixels), ScreenSurface->w, ScreenSurface->h);
					}
					break;
				case SDLK_F11:
					IsRecording = !IsRecording;
					break;
				}
				break;
			case SDL_KEYUP:
				for (int n = 0; n < sizeof(KeyMappings); n++)
				{
					if (event.key.keysym.sym == KeyMappings[n].key)
					{
						InputMask &= ~KeyMappings[n].mask;
					}
				}
				if (event.key.keysym.sym == SDLK_TAB)
					playRate = 1;
				break;
			}
		}

		SDL_SetRenderDrawColor(AppRenderer, 206, 221, 231, 255);
		SDL_RenderClear(AppRenderer);

		//memset(ScreenSurface->pixels, 0, ScreenSurface->format->BytesPerPixel * ScreenSurface->w * ScreenSurface->h);

		for (int n = 0; n < playRate; n++)
		{
			TickGame();
		}

		if (IsRecording)
		{
			std::ostringstream filename;
			filename << "Frame";
			filename << std::setfill('0') << std::setw(5) << CurrentRecordingFrame << ".png";

			lodepng::encode(filename.str(), (unsigned char*)(ScreenSurface->pixels), ScreenSurface->w, ScreenSurface->h);
			CurrentRecordingFrame++;
		}

		SDL_UpdateTexture(ScreenTexture, NULL, ScreenSurface->pixels, ScreenSurface->pitch);
		SDL_Rect src, dest;
		src.x = src.y = dest.x = dest.y = 0;
		src.w = DISPLAY_WIDTH;
		src.h = DISPLAY_HEIGHT;
		dest.w = DISPLAY_WIDTH;
		dest.h = DISPLAY_HEIGHT;
		SDL_RenderCopy(AppRenderer, ScreenTexture, &src, &dest);
		SDL_RenderPresent(AppRenderer);

		SDL_Delay(1000 / 24);
	}

	return 0;
}
