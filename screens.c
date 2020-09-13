#define STATS_WIDTH		20
#define STATS_HEIGHT 	5

#define LOG_WIDTH		58
#define LOG_HEIGHT		5

#define INVENTORY_LEFT		20
#define INVENTORY_TOP		7
#define INVENTORY_WIDTH		40
#define INVENTORY_HEIGHT	30
global_variable UIView *inventoryView = NULL;
global_variable i32 highlightedIdx = 0;
global_variable i32 selIdx = 0;
global_variable i32 inventoryLen = 1;

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
		if (renderableComps[i].objectId > 0 && !backpackComps[i].objectId > 0) {
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

	//draw player position
	Position *playerPos = (Position *)getComponentForGameObject(player, COMP_POSITION);
	char *msg = String_Create("X: %d Y: %d", playerPos->x, playerPos->y);
	PT_ConsolePutStringAt(console, msg, 0, 3, 0x009900FF, 0x00000000);
	String_Destroy(msg);
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

internal void 
render_inventory_view(PT_Console *console) 
{
	PT_Rect rect = {0, 0, INVENTORY_WIDTH, INVENTORY_HEIGHT};
	UI_DrawRect(console, &rect, 0x222222FF, 1, 0xFF990099);

	//Render list of carried items
	i32 yIdx = 4;
	i32 lineIdx = 1; //start at 1 because GameObject indices start at 1
	Equipment *eq = NULL;
	Name *name = NULL;
	for (u32 i = 1; i < MAX_GO; i++) {
		GameObject go = gameObjects[i];
		eq = (Equipment *)getComponentForGameObject(&go, COMP_EQUIP);
		name = (Name *)getComponentForGameObject(&go, COMP_NAME);
			if (eq != NULL && backpackComps[i].objectId > 0) {
			char *equipped = (eq->isEquipped) ? "*" : ".";
			char *slotStr = String_Create("[%s]", eq->slot);
			char *itemText = String_Create("%s %-10s %-8s", equipped, name->name, slotStr);
			//draw highlight
			if (lineIdx == highlightedIdx) {
				selIdx = i;
				PT_ConsolePutStringAt(console, itemText, 4, yIdx, 0xffffffff, 0x80000099);
			} else {
				PT_ConsolePutStringAt(console, itemText, 4, yIdx, 0x666666ff, 0x00000000);
			}
			yIdx += 1;
			lineIdx += 1;
			
		}
	}
	//track length of inventory
	inventoryLen = lineIdx-1;

}

// Screen Functions

internal void 
hide_inventory_overlay(UIScreen *screen) 
{
	if (inventoryView != NULL) {
		list_remove_element_with_data(screen->views, inventoryView);
		view_destroy(inventoryView);
		inventoryView = NULL;
	}
}

internal void 
show_inventory_overlay(UIScreen *screen) 
{
	if (inventoryView == NULL) {
		PT_Rect overlayRect = {(16 * INVENTORY_LEFT), (16 * INVENTORY_TOP), (16 * INVENTORY_WIDTH), (16 * INVENTORY_HEIGHT)};
		inventoryView = view_new(overlayRect, INVENTORY_WIDTH, INVENTORY_HEIGHT, 
								   "assets/terminal16x16.png", 0, render_inventory_view);
		list_insert_after(screen->views, list_tail(screen->views), inventoryView);
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
			if (inventoryView != NULL) {
				// Handle for inventory view
				highlightedIdx -= 1;
				if (highlightedIdx < 1) { highlightedIdx = 1; }
			} else {
				// Handle for main in-game screen
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
		}
        if (key == SDLK_DOWN) { 
			if (inventoryView != NULL) {
				// Handle for inventory view
				highlightedIdx += 1;
				if (highlightedIdx > inventoryLen) { highlightedIdx = inventoryLen; }
			} else {
				// Handle for main in-game view
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
		//other actions
		if (key == SDLK_g) {
			GameObject *itemObj = NULL;
			itemObj = getItemAtPos(playerPos->x, playerPos->y);

			if (itemObj != NULL) {
				//pick it up
				InBackpack back_comp = {.objectId = itemObj->id};
				addComponentToGameObject(itemObj, COMP_INBACKPACK, &back_comp);
				Name *name_comp = (Name *)getComponentForGameObject(itemObj, COMP_NAME);
				char *msg = String_Create("You picked up the %s.", name_comp->name);
				add_message(msg, 0x009900ff);
				String_Destroy(msg);
			}
			else {
				char *msg = String_Create("No items to pick up here!");
				add_message(msg, 0xFFFFFFFF);
				String_Destroy(msg);
			}
		}

		if (key == SDLK_i) {
			if (inventoryView == NULL) {
				show_inventory_overlay(activeScreen);				
			} else {
				hide_inventory_overlay(activeScreen);
			}
		}
		if (key == SDLK_e) {
			if (inventoryView != NULL) {
				if (selIdx > 0) {
					Name *name = NULL;
					GameObject go = gameObjects[selIdx];
					name = (Name *)getComponentForGameObject(&go, COMP_NAME);
					char *msg = String_Create("Player equipped %s", name->name);
					add_message(msg, 0xFFFFFFFF);
					String_Destroy(msg);
					item_toggle_equip(&go);
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