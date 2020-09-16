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

#ifdef __EMSCRIPTEN__
#define HAVE_ASPRINTF //Emscripten has their own asprintf built-in!
#include <emscripten.h>
#endif

global_variable should_quit = false;

// our own stuff starts here
#include "utils.c"
#include "list.c"

/* Message Log */
typedef struct {
	char *msg;
	u32 fgColor;
} Message;

global_variable List *messageLog = NULL;

#include "pt_console.c"
#include "pt_ui.c"
#include "String.c"

#include "ecs.c"
#include "map.c"
#include "fov.c"
#include "game.c"
#include "screens.c"

struct context {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *screenTexture;
	UIScreen *activeScreen;
    //bool should_quit; //moved to global variable to be able to access it from within a screen
};

#include "stdio.h"

/* Message */
void add_message(char *msg, u32 color) {
	Message *m = malloc(sizeof(Message));
	if (msg != NULL) {
		//see comment in ecs.c
		m->msg = malloc(strlen(msg) +1);
		strcpy(m->msg, msg);		
	} else {
		m->msg = "";
	}
	m->fgColor = color;

	// Add message to log
	if (messageLog == NULL) {
		messageLog = list_new(NULL);
	}
	list_insert_after(messageLog, list_tail(messageLog), m);

	// If our log has exceeded 20 messages, cull the older messages
	if (list_size(messageLog) > 20) {
		list_remove(messageLog, NULL);  // Remove the oldest message
	}

	// DEBUG
	printf("%s\n", msg);
	printf("Message Log Size: %d\n", list_size(messageLog));
	// DEBUG
}


// looks like functions have to be defined before use in C
// this is the meat of our rendering
void render_screen(SDL_Renderer *renderer, SDL_Texture *screenTexture, UIScreen *screen) {

	// Render views from back to front for the current screen
	ListElement *e = list_head(screen->views);
	while (e != NULL) {
		UIView *v = (UIView *)list_data(e);
		PT_ConsoleClear(v->console);
		v->render(v->console);
		SDL_UpdateTexture(screenTexture, v->pixelRect, v->console->pixels, v->pixelRect->w * sizeof(u32));
		e = list_next(e);
	}

	//SDL_UpdateTexture(screenTexture, NULL, screen->console->pixels, SCREEN_WIDTH * sizeof(u32));
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

//Savegame callback/hook (https://www.tutorialspoint.com/callbacks-in-c)
void _on_save_exit() {
	//now mark the flag that tells the game to quit
	should_quit = true;
}


/* What it says on the tin */
void main_loop(void *context) {
	struct context *ctx = (struct context *)context;
	SDL_Event event;
	
	while (SDL_PollEvent(&event) != 0) {

		if (event.type == SDL_QUIT) {
			//save game on quit
			void (*ptr)() = &_on_save_exit; 
			game_save(ptr);
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#endif
			//should_quit = true;
			break;
		}

		// Handle "global" keypresses (those not handled on a screen-by-screen basis)
		// if (event.type == SDL_KEYDOWN) {
		// 	SDL_Keycode key = event.key.keysym.sym;
		// 	if (key == SDLK_ESCAPE) {
		// 		ctx->should_quit = true;
		// 		//break;
		// 	}
		// }
		// Send the event to the currently active screen for handling
		ctx->activeScreen->handle_event(ctx->activeScreen, event);
	}

	game_update();

	// Render the active screen
	render_screen(ctx->renderer, ctx->screenTexture, ctx->activeScreen);
}

//test exporting
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
void test_export() {
	printf("Test export!");
	emscripten_resume_main_loop();
}
#endif

/* Initialization here */
int main() {

	#ifdef __EMSCRIPTEN__
	/* Setup Emscripten file system!!! */
	// EM_ASM is a macro to call in-line JavaScript code.
	//EM_JS doesn't inline
    EM_ASM({
        // Make a directory other than '/'
		// https://github.com/emscripten-core/emscripten/issues/2040 - root cannot be mounted as anything else than MEMFS
        FS.mkdir('/save');
        // Then mount with IDBFS type
        FS.mount(IDBFS, {}, '/save');
	});


	emscripten_pause_main_loop(); // Will need to wait for FS sync.
    EM_ASM({
	    // Then sync
        FS.syncfs(true, function (err) {
            // Error
			if (!err) {
				console.log("Successfully mounted and synced the filesystem");
				//FIXME: any ccall here freezes the browser
				//call the C function
				//ccall('game_load', 'v');
				ccall('test_export', null, []);
			}
        });
	});
	#endif

	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window *window = SDL_CreateWindow("Dark Caverns",
		SDL_WINDOWPOS_UNDEFINED, 
		SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		0);

	SDL_Renderer *renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_SOFTWARE);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

	SDL_Texture *screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	// Initialize UI state (screens, view stack, etc)
	UIScreen *activeScreen = NULL;
	
	activeScreen = screens_setup();

	game_new();

	#ifndef __EMSCRIPTEN__
		//load game if any
		game_load();
	#endif

	struct context ctx = {.window = window, .screenTexture = screenTexture, .renderer = renderer, .activeScreen = activeScreen};

/* Main loop handling */
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(main_loop, &ctx, -1, 1);
#else

	//bool done = false;
	while (!should_quit) {
		main_loop(&ctx);
	}
# endif

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}