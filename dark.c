/*
dark.c - main Dark Twilight file 
*/

#include "assert.h"
#include <SDL2/SDL.h>

#define SCREEN_WIDTH	1280
#define SCREEN_HEIGHT	720

#define NUM_COLS		80
#define NUM_ROWS 		45

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t		u8;
typedef uint32_t	u32;
typedef uint64_t	u64;
typedef int32_t		i32;
typedef int64_t		i64;

#define internal static
#define local_persist static
#define global_variable static


#include "pt_console.c"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// our own stuff starts here
#include "ecs.c"

struct context {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *screen;
	PT_Console *console;
	GameObject *player; //hack for now!
    bool should_quit;
};

// looks like functions have to be defined before use in C
void render_screen(SDL_Renderer *renderer, SDL_Texture *screen, PT_Console *console) {

	u32 *pixels = calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u32));
	PT_ConsoleClear(console);

	//PT_ConsolePutCharAt(console, '@', player.pos_x, player.pos_y, 0xFFFFFFFF, 0x000000FF);
	for (u32 i = 1; i < MAX_GO; i++) {
		if (renderableComps[i].objectId > 0) {
			Position *p = (Position *)getComponentForGameObject(&gameObjects[i], COMP_POSITION);
			PT_ConsolePutCharAt(console, renderableComps[i].glyph, p->x, p->y, 
								renderableComps[i].fgColor, renderableComps[i].bgColor);
		}
	}

	SDL_UpdateTexture(screen, NULL, console->pixels, SCREEN_WIDTH * sizeof(u32));
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, screen, NULL, NULL);
	SDL_RenderPresent(renderer);
}

bool canMove(Position pos) {
	bool moveAllowed = true;

	if ((pos.x >= 0) && (pos.x < NUM_COLS) && (pos.y >= 0) && (pos.y < NUM_ROWS)) {
		for (u32 i = 1; i < MAX_GO; i++) {
			Position p = positionComps[i];
			if ((p.objectId > 0) && (p.x == pos.x) && (p.y == pos.y)) {
				if (physicalComps[i].blocksMovement == true) {
					moveAllowed = false;
				}
			}
		}

	} else {
		moveAllowed = false;
	}

	return moveAllowed;
}

/* What it says on the tin */
void main_loop(void *context) {
	struct context *ctx = (struct context *)context;
	SDL_Event event;
		
	while (SDL_PollEvent(&event) != 0) {

		if (event.type == SDL_QUIT) {
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#endif
			ctx->should_quit = true;
			break;
		}

		SDL_Keycode key = event.key.keysym.sym;
		Position *playerPos = (Position *)getComponentForGameObject(ctx->player, COMP_POSITION);

        if (key == SDLK_UP) {
				Position newPos = {playerPos->objectId, playerPos->x, playerPos->y - 1};
			 	if (canMove(newPos)) { addComponentToGameObject(ctx->player, COMP_POSITION, &newPos); } 
			}
        if (key == SDLK_DOWN) { 
				Position newPos = {playerPos->objectId, playerPos->x, playerPos->y + 1};
				if (canMove(newPos)) { addComponentToGameObject(ctx->player, COMP_POSITION, &newPos); } 
			}
        if (key == SDLK_LEFT) { 
				Position newPos = {playerPos->objectId, playerPos->x - 1, playerPos->y};
				if (canMove(newPos)) { addComponentToGameObject(ctx->player, COMP_POSITION, &newPos); } 
			}
        if (key == SDLK_RIGHT) { 
				Position newPos = {playerPos->objectId, playerPos->x + 1, playerPos->y};
				if (canMove(newPos)) { addComponentToGameObject(ctx->player, COMP_POSITION, &newPos); } 
			}

	}

	render_screen(ctx->renderer, ctx->screen, ctx->console);
}

/* Initialization here */
int main() {

	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window *window = SDL_CreateWindow("Dark Caverns",
		SDL_WINDOWPOS_UNDEFINED, 
		SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		0);

	SDL_Renderer *renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_SOFTWARE);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

	SDL_Texture *screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	PT_Console *console = PT_ConsoleInit(SCREEN_WIDTH, SCREEN_HEIGHT, 
										 NUM_ROWS, NUM_COLS);

	PT_ConsoleSetBitmapFont(console, "assets/terminal16x16.png", 0, 16, 16);

	GameObject *player = createGameObject();
	Position pos = {player->id, 10, 10};
	addComponentToGameObject(player, COMP_POSITION, &pos);
	Renderable rnd = {player->id, '@', 0x00FFFFFF, 0x000000FF};
	addComponentToGameObject(player, COMP_RENDERABLE, &rnd);
	Physical phys = {player->id, true, true};
	addComponentToGameObject(player, COMP_PHYSICAL, &phys);

	//TODO: do I want walls to be Game Objects?
	GameObject *wall = createGameObject();
	Position wallPos = {wall->id, 30, 25};
	addComponentToGameObject(wall, COMP_POSITION, &wallPos);
	Renderable wallRnd = {wall->id, '#', 0xFFFFFFFF, 0x000000FF};
	addComponentToGameObject(wall, COMP_RENDERABLE, &wallRnd);
	Physical wallPhys = {wall->id, true, true};
	addComponentToGameObject(wall, COMP_PHYSICAL, &wallPhys);

	struct context ctx = {.window = window, .screen = screen, .renderer = renderer, .console = console, .player = player, .should_quit = false};

/* Main loop handling */
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(main_loop, &ctx, -1, 1);
#else

	bool done = false;
	while (!ctx.should_quit) {
		main_loop(&ctx);
	}
# endif

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}