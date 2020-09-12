#define STATS_WIDTH		20
#define STATS_HEIGHT 	5

#define LOG_WIDTH		58
#define LOG_HEIGHT		5

//Rendering
void draw_map(PT_Console *console){
	int x;
	int y;
	int tile; 
	asciiChar glyph;
	u32 fgColor;

	for (x = 0; x < MAP_WIDTH; x++)
  	{
     	for (y = 0; y < MAP_HEIGHT; y++)	
    	{
			if (fovMap[x][y] > 0 || seenMap[x][y] > 0) {
				tile = map[x][y];
				//tile = get_tile(x,y);

				switch (tile)
				{
					case tile_floor:
					{
						glyph = '.';
						break;
					}
						
					case tile_wall:
					{
						glyph = '#';
						break;
					}
					default:
					{
						glyph = 'x';
					}
				}

				if (seenMap[x][y] > 0) {
					fgColor = 0x7F7F7FFF;
				}
				if (fovMap[x][y] > 0) {
					fgColor = 0xFFFFFFFF;
				}
				
				

				PT_ConsolePutCharAt(console, glyph, x, y, fgColor, 0x000000FF);
			}
		}
	}
}

void debug_draw_Dijkstra(PT_Console *console){
	int x;
	int y;
	
	for (x = 0; x < MAP_WIDTH; x++)
  	{
     	for (y = 0; y < MAP_HEIGHT; y++)	
    	{
			if (fovMap[x][y] > 0 || seenMap[x][y] > 0) {
				i32 val = targetMap[x][y] % 10;
				asciiChar ch = 48 + val;
				PT_ConsolePutCharAt(console, ch, x, y, 0xFFFFFFFF, 0x000000FF);
			}
		}
	}
}

internal void gameRender(PT_Console *console){
	//PT_ConsolePutCharAt(console, '@', player.pos_x, player.pos_y, 0xFFFFFFFF, 0x000000FF);
	draw_map(console);

	for (u32 i = 1; i < MAX_GO; i++) {
		if (renderableComps[i].objectId > 0) {
			Position *p = (Position *)getComponentForGameObject(&gameObjects[i], COMP_POSITION);
			if (fovMap[p->x][p->y] > 0) {
				PT_ConsolePutCharAt(console, renderableComps[i].glyph, p->x, p->y, 
								renderableComps[i].fgColor, renderableComps[i].bgColor);
			}
		}
	}

	//debug Dijkstra map
	//debug_draw_Dijkstra(console);
}

internal void statsRender(PT_Console *console) {

	PT_Rect rect = {0, 0, STATS_WIDTH, STATS_HEIGHT};
	UI_DrawRect(console, &rect, 0x222222FF, 0, 0xFF990099); //light gray

	// HP health bar
	Health *playerHealth = getComponentForGameObject(player, COMP_HEALTH);
	PT_ConsolePutCharAt(console, 'H', 0, 1, 0xFF990099, 0x00000000); //brown
	PT_ConsolePutCharAt(console, 'P', 1, 1, 0xFF990099, 0x00000000);
	i32 leftX = 3;
	i32 barWidth = 16;

	i32 healthCount = ceil(((float)playerHealth->currentHP / (float)playerHealth->maxHP) * barWidth);
	for (i32 x = 0; x < barWidth; x++) {
		if (x < healthCount) {
			//PT_ConsolePutCharAt(console, '#', leftX + x, 41, 0x009900FF, 0x00000000);	//green	
			PT_ConsolePutCharAt(console, 176, leftX + x, 1, 0x009900FF, 0x00000000); //one of the dotted/shaded rectangles
			//note that pt_console.c allows layering characters!
			PT_ConsolePutCharAt(console, 3, leftX + x, 1, 0x009900FF, 0x00000000);	//heart
		} else {
			PT_ConsolePutCharAt(console, 176, leftX + x, 1, 0xFF990099, 0x00000000);		
		}
	}

}

internal void messageLogRender(PT_Console *console) {
	// some fancy background color
	PT_Rect rect = {0, 0, LOG_WIDTH, LOG_HEIGHT};
	UI_DrawRect(console, &rect, 0x111111FF, 0, 0xFF990099);


	if (messageLog == NULL) { return; }

	// Get the last 5 messages from the log
	ListElement *e = list_tail(messageLog);
	i32 msgCount = list_size(messageLog);
	u32 row = 4;
	u32 col = 0;

	if (msgCount < 5) {
		row -= (5 - msgCount);
	} else {
		msgCount = 5;
	}

	for (i32 i = 0; i < msgCount; i++) {
		if (e != NULL) {
			Message *m = (Message *)list_data(e);
			PT_Rect rect = {.x = col, .y = row, .w = LOG_WIDTH, .h = 1};
			PT_ConsolePutStringInRect(console, m->msg, rect, false, m->fgColor, 0x00000000);
			e = list_prev(e);			
			row -= 1;
		}
	}
}


// Event Handling --

internal void
handle_event_in_game(UIScreen *activeScreen, SDL_Event event) 
{
	if (event.type == SDL_KEYDOWN) {
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
}

//setup
internal UIScreen *
screens_setup() {
    List *igViews = list_new(NULL);

	PT_Rect mapRect = {0, 0, (16 * MAP_WIDTH), (16 * MAP_HEIGHT)};
	UIView *mapView = view_new(mapRect, MAP_WIDTH, MAP_HEIGHT, 
							   "assets/terminal16x16.png", 0, gameRender);
	list_insert_after(igViews, NULL, mapView);

	PT_Rect statsRect = {0, (16 * MAP_HEIGHT), (16 * STATS_WIDTH), (16 * STATS_HEIGHT)};
	UIView *statsView = view_new(statsRect, STATS_WIDTH, STATS_HEIGHT,
								 "assets/terminal16x16.png", 0, statsRender);
	list_insert_after(igViews, NULL, statsView);

	PT_Rect logRect = {(16 * 22), (16 * MAP_HEIGHT), (16 * LOG_WIDTH), (16 * LOG_HEIGHT)};
	UIView *logView = view_new(logRect, LOG_WIDTH, LOG_HEIGHT,
							   "assets/terminal16x16.png", 0, messageLogRender);
	list_insert_after(igViews, NULL, logView);

	UIScreen *inGameScreen = malloc(sizeof(UIScreen));
	inGameScreen->views = igViews;

    inGameScreen->activeView = mapView;
	inGameScreen->handle_event = handle_event_in_game;
    return inGameScreen;
}