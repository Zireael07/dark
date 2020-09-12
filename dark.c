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

#include "ecs.c"
#include "map.c"
#include "fov.c"
#include "screens.c"

struct context {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *screenTexture;
	UIScreen *activeScreen;
    bool should_quit;
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


bool canMove(Position pos) {
	bool moveAllowed = true;

	if ((pos.x >= 0) && (pos.x < NUM_COLS) && (pos.y >= 0) && (pos.y < NUM_ROWS)) {
		//check map
		if (map[pos.x][pos.y] == tile_wall) {
			moveAllowed = false;
		}

		//check for blocking
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

void health_check_death(GameObject *go) {
	Health *h = (Health *)getComponentForGameObject(go, COMP_HEALTH);
	if (h->currentHP <= 0) {
		// Death!
		if (go == player) {
			char *msg = NULL;
			sasprintf(msg, "You have died.");
			add_message(msg, 0xCC0000FF);

			// TODO: Enter endgame flow

		} else {
			char *msg = NULL;
			sasprintf(msg, "You killed the [MONSTER].");
			add_message(msg, 0xCC0000FF);

			//remove from ECS
			destroyGameObject(go);
		}
	}
}

void combatAttack(GameObject *attacker, GameObject *defender) {
	Combat *att = (Combat *)getComponentForGameObject(attacker, COMP_COMBAT);
	Combat *def = (Combat *)getComponentForGameObject(defender, COMP_COMBAT);
	Health *defHealth = (Health *)getComponentForGameObject(defender, COMP_HEALTH);

	Name *name_att = (Name *)getComponentForGameObject(attacker, COMP_NAME);
	Name *name_def = (Name *)getComponentForGameObject(defender, COMP_NAME);

	i32 damage = att->attack;
	defHealth->currentHP -= damage;

	//printf("%s attacks %s\n", name_att->name, name_def->name);
	char *msg = NULL;
	sasprintf(msg, "%s attacks %s for %i damage!", name_att->name, name_def->name, damage);
	add_message(msg, 0xCC0000FF);

	health_check_death(defender);
}

void onPlayerMoved(GameObject *player) {
	//clear and regenerate Dijkstra map
	if (targetMap != NULL) {
		free(targetMap);
	}
	Position *playerPos = (Position *)getComponentForGameObject(player, COMP_POSITION);
	generate_Dijkstra_map(playerPos->x, playerPos->y);

	//NPCs get their turn
	for (u32 i = 1; i < MAX_GO; i++) {
		NPC npc = NPCComps[i];
		if ((npc.objectId > 0)) {
			//printf("%s growls!\n", nameComps[i].name);

			Position *p = (Position *)getComponentForGameObject(&gameObjects[i], COMP_POSITION);
			Position newPos = {.objectId = p->objectId, .x = p->x, .y = p->y};

			// If the player can see the monster, the monster can see the player
			if (fovMap[p->x][p->y] > 0) {
				// Determine if we're currently in combat range of the player
				if (targetMap[p->x][p->y] == 1) {
					//printf("%s", "Monster attacks!\n");
					// Combat range - so attack the player
					combatAttack(&gameObjects[npc.objectId], player);
				} else {
					// Evaluate all cardinal direction cells and pick randomly between optimal moves 
					Position moves[4];
					i32 moveCount = 0;
					i32 currTargetValue = targetMap[p->x][p->y];
					if (targetMap[p->x - 1][p->y] < currTargetValue) {
						Position np = newPos;
						np.x -= 1;	
						moves[moveCount] = np;					
						moveCount += 1;
					}
					if (targetMap[p->x][p->y - 1] < currTargetValue) { 
						Position np = newPos;
						np.y -= 1;						
						moves[moveCount] = np;					
						moveCount += 1;
					}
					if (targetMap[p->x + 1][p->y] < currTargetValue) { 
						Position np = newPos;
						np.x += 1;						
						moves[moveCount] = np;					
						moveCount += 1;
					}
					if (targetMap[p->x][p->y + 1] < currTargetValue) { 
						Position np = newPos;
						np.y += 1;						
						moves[moveCount] = np;					
						moveCount += 1;
					}

					u32 moveIdx = rand() % moveCount;
					newPos = moves[moveIdx];			

					// Test to see if the new position can be moved to
					if (canMove(newPos)) {
						addComponentToGameObject(&gameObjects[i], COMP_POSITION, &newPos);
					}
				}
			}

		}
	}
}

/* What it says on the tin */
void main_loop(void *context) {
	struct context *ctx = (struct context *)context;
	SDL_Event event;
	
	bool recalculateFOV = false;

	while (SDL_PollEvent(&event) != 0) {

		if (event.type == SDL_QUIT) {
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#endif
			ctx->should_quit = true;
			break;
		}

		SDL_Keycode key = event.key.keysym.sym;
		Position *playerPos = (Position *)getComponentForGameObject(player, COMP_POSITION);

        if (key == SDLK_UP) {
				Position newPos = {playerPos->objectId, playerPos->x, playerPos->y - 1};
			 	if (canMove(newPos)) { 
					addComponentToGameObject(player, COMP_POSITION, &newPos);
					recalculateFOV = true;
					onPlayerMoved(player);
				} else {
					//check for blocking NPCs
					GameObject *blockerObj = NULL;
					for (u32 i = 1; i < MAX_GO; i++) {
						Position p = positionComps[i];
						if ((p.objectId > 0) && (p.x == newPos.x) && (p.y == newPos.y)) {
							if (healthComps[i].currentHP > 0) {
								blockerObj = (GameObject *) &gameObjects[i];
								//printf("Blocker found!\n");
								break;
							}
						}
					}

					if (blockerObj != NULL) {
						//printf("We have a blocker!\n");
						combatAttack(player, blockerObj);
						onPlayerMoved(player);
					}
				}
			}
        if (key == SDLK_DOWN) { 
				Position newPos = {playerPos->objectId, playerPos->x, playerPos->y + 1};
				if (canMove(newPos)) { 
					addComponentToGameObject(player, COMP_POSITION, &newPos);
					recalculateFOV = true;
					onPlayerMoved(player);
				} else {
					//check for blocking NPCs
					GameObject *blockerObj = NULL;
					for (u32 i = 1; i < MAX_GO; i++) {
						Position p = positionComps[i];
						if ((p.objectId > 0) && (p.x == newPos.x) && (p.y == newPos.y)) {
							if (healthComps[i].currentHP > 0) {
								blockerObj = (GameObject *) &gameObjects[i];
								//printf("Blocker found!\n");
								break;
							}
						}
					}

					if (blockerObj != NULL) {
						//printf("We have a blocker!\n");
						combatAttack(player, blockerObj);
						onPlayerMoved(player);
					}
				}
			}
        if (key == SDLK_LEFT) { 
				Position newPos = {playerPos->objectId, playerPos->x - 1, playerPos->y};
				if (canMove(newPos)) { 
					addComponentToGameObject(player, COMP_POSITION, &newPos);
					recalculateFOV = true;
					onPlayerMoved(player);
				} else {
					//check for blocking NPCs
					GameObject *blockerObj = NULL;
					for (u32 i = 1; i < MAX_GO; i++) {
						Position p = positionComps[i];
						if ((p.objectId > 0) && (p.x == newPos.x) && (p.y == newPos.y)) {
							if (healthComps[i].currentHP > 0) {
								blockerObj = (GameObject *) &gameObjects[i];
								//printf("Blocker found!\n");
								break;
							}
						}
					}

					if (blockerObj != NULL) {
						//printf("We have a blocker!\n");
						combatAttack(player, blockerObj);
						onPlayerMoved(player);
					}
				}
			}
        if (key == SDLK_RIGHT) { 
				Position newPos = {playerPos->objectId, playerPos->x + 1, playerPos->y};
				if (canMove(newPos)) { 
					addComponentToGameObject(player, COMP_POSITION, &newPos);
					recalculateFOV = true;
					onPlayerMoved(player);
				} else {
					//check for blocking NPCs
					GameObject *blockerObj = NULL;
					for (u32 i = 1; i < MAX_GO; i++) {
						Position p = positionComps[i];
						if ((p.objectId > 0) && (p.x == newPos.x) && (p.y == newPos.y)) {
							if (healthComps[i].currentHP > 0) {
								blockerObj = (GameObject *) &gameObjects[i];
								//printf("Blocker found!\n");
								break;
							}
						}
					}

					if (blockerObj != NULL) {
						//printf("We have a blocker!\n");
						combatAttack(player, blockerObj);
						onPlayerMoved(player);
					}
				}
			}

	}

	if (recalculateFOV) {
		Position *pos = (Position *)getComponentForGameObject(player, COMP_POSITION);
		fov_calculate(pos->x, pos->y, fovMap);
		recalculateFOV = false;
	}

	// TODO: Determine active screen and render it
	render_screen(ctx->renderer, ctx->screenTexture, ctx->activeScreen);
	//render_screen(ctx->renderer, ctx->screen, ctx->console);
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

	SDL_Texture *screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	// Initialize UI state (screens, view stack, etc)
	UIScreen *activeScreen = NULL;
	
	activeScreen = screens_setup();


	player = createGameObject();
	Position pos = {player->id, 10, 10};
	addComponentToGameObject(player, COMP_POSITION, &pos);
	Renderable rnd = {player->id, '@', 0x00FFFFFF, 0x000000FF};
	addComponentToGameObject(player, COMP_RENDERABLE, &rnd);
	Physical phys = {player->id, true, true};
	addComponentToGameObject(player, COMP_PHYSICAL, &phys);
	Name name = {.objectId = player->id};
	name.name = "Player"; // we can't initialize strings in C!
	//printf("Player name: %s\n", name.name);
	addComponentToGameObject(player, COMP_NAME, &name);
	Health hlth = {.objectId = player->id, .currentHP = 20, .maxHP = 20, .recoveryRate = 1};
	addComponentToGameObject(player, COMP_HEALTH, &hlth);
	Combat com = {.objectId = player->id, .attack = 2, .defense = 2};
	addComponentToGameObject(player, COMP_COMBAT, &com);

	
	Point pt = level_get_open_point();
	add_NPC(pt.x, pt.y, 't', 0xFF0000FF, 8, 1, 1);
	pt = level_get_open_point();
	add_NPC(pt.x, pt.y, 't', 0xFF0000FF, 8, 1, 1);


	generate_map();

	Position *playerPos = (Position *)getComponentForGameObject(player, COMP_POSITION);
	fov_calculate(playerPos->x, playerPos->y, fovMap);
	//Dijkstra map
	generate_Dijkstra_map(playerPos->x, playerPos->y);

	struct context ctx = {.window = window, .screenTexture = screenTexture, .renderer = renderer, .activeScreen = activeScreen, .should_quit = false};

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