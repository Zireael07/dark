#include <SDL2/SDL.h>

#define SCREEN_WIDTH	1280
#define SCREEN_HEIGHT	720

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t		u8;
typedef uint32_t	u32;
typedef uint64_t	u64;
typedef int32_t		i32;
typedef int64_t		i64;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

struct context {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *screen;
    bool should_quit;
};

// looks like functions have to be defined before use in C
void render_screen(SDL_Renderer *renderer, SDL_Texture *screen) {

	u32 *pixels = calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u32));

	SDL_UpdateTexture(screen, NULL, pixels, SCREEN_WIDTH * sizeof(u32));
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, screen, NULL, NULL);
	SDL_RenderPresent(renderer);
}

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

	}

	render_screen(ctx->renderer, ctx->screen);
}

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

	struct context ctx = {.window = window, .screen = screen, .renderer = renderer, .should_quit = false};

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