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

global_variable Point mousePos = {0,0};

#include "colors.c"

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
					fgColor = color_light_gray;
				}
				if (fovMap[x][y] > 0) {
					fgColor = color_white;
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

	//mouse highlight test
	//should use console->cellWidth and console->cellHeight, but that blows up occasionally?
	PT_ConsolePutCharAt(console, '#', mousePos.x/16, mousePos.y/16, 0x00000000, color_solarized_green); //solarized green bg

	//debug Dijkstra map
	//debug_draw_Dijkstra(console);
}

internal void statsRender(PT_Console *console) {

	PT_Rect rect = {0, 0, STATS_WIDTH, STATS_HEIGHT};
	UI_DrawRect(console, &rect, 0x222222FF, 0, 0xFF990099); //light gray

	//menu button
	char *menu_label = String_Create("Inventory");
	 // Check if the button should be hot
	 //all calculations only within this console
    PT_Rect pixelRect = { 
        rect.x = ((STATS_WIDTH/2)-5) * console->cellWidth,
        rect.y = 0 * console->cellHeight,
        rect.w = 9 * console->cellWidth,
        rect.h = 1 * console->cellHeight
    }; 
    if (UI_PointInRect(mousePos.x, mousePos.y-MAP_HEIGHT*16, &pixelRect)) {
		PT_ConsolePutStringAt(console, menu_label, (STATS_WIDTH/2)-5, 0, color_cyan, 0x586e75FF);
	}
	else {
		PT_ConsolePutStringAt(console, menu_label, (STATS_WIDTH/2)-5, 0, color_cyan, 0x00000000);
	}
	String_Destroy(menu_label);

	// HP health bar
	Health *playerHealth = getComponentForGameObject(player, COMP_HEALTH);
	PT_ConsolePutCharAt(console, 'H', 0, 1, color_cyan, 0x00000000); //brown
	PT_ConsolePutCharAt(console, 'P', 1, 1, color_cyan, 0x00000000);
	i32 leftX = 3;
	i32 barWidth = 16;

	i32 healthCount = ceil(((float)playerHealth->currentHP / (float)playerHealth->maxHP) * barWidth);
	for (i32 x = 0; x < barWidth; x++) {
		if (x < healthCount) {
			//PT_ConsolePutCharAt(console, '#', leftX + x, 41, 0x009900FF, 0x00000000);	//green	
			PT_ConsolePutCharAt(console, 176, leftX + x, 1, color_cyan, 0x00000000); //one of the dotted/shaded rectangles
			//note that pt_console.c allows layering characters!
			PT_ConsolePutCharAt(console, 3, leftX + x, 1, color_cyan, 0x00000000);	//heart
		} else {
			PT_ConsolePutCharAt(console, 176, leftX + x, 1, color_gray, 0x00000000);		
		}
	}

	//draw player position
	Position *playerPos = (Position *)getComponentForGameObject(player, COMP_POSITION);
	char *msg = String_Create("X: %d Y: %d", playerPos->x, playerPos->y);
	PT_ConsolePutStringAt(console, msg, 0, 3, color_cyan, 0x00000000);
	String_Destroy(msg);
	//debug mouse pos
	char *mouse = String_Create("Mouse X: %d Y: %d", mousePos.x/16, mousePos.y/16);
	PT_ConsolePutStringAt(console, mouse, 0, 4, color_cyan, 0x00000000);
	String_Destroy(mouse);

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
	UI_DrawRect(console, &rect, 0x222222FF, 1, color_cyan);

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
				PT_ConsolePutStringAt(console, itemText, 4, yIdx, color_white, 0x80000099);
			} else {
				PT_ConsolePutStringAt(console, itemText, 4, yIdx, color_gray, 0x00000000);
			}
			yIdx += 1;
			lineIdx += 1;
			
		}
	}
	//track length of inventory
	inventoryLen = lineIdx-1;

	// Render additional information at bottom of view
	char *instructions = String_Create("[Up/Down] to select item");
	char *i2 = String_Create("[Spc or left click] to (un)equip");
	char *i3 = String_Create("[D or right click] to drop");
	PT_ConsolePutStringAt(console, instructions, 5, 25, color_gray, 0x00000000);
	PT_ConsolePutStringAt(console, i2, 5, 26, color_gray, 0x00000000);
	PT_ConsolePutStringAt(console, i3, 5, 27, color_gray, 0x00000000);

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

void _on_save_dummy(){}

internal void
handle_event_in_game(UIScreen *activeScreen, SDL_Event event) 
{
	//If mouse event happened
    if( event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP )
    {
        //Get mouse position
        //int x, y;
        SDL_GetMouseState( &(mousePos.x), &(mousePos.y) );
	}
	if ( event.type == SDL_MOUSEBUTTONDOWN) {
		//left click to move
		if (event.button.button == SDL_BUTTON_LEFT){
			//handle GUI
			//inventory
			if (inventoryView != NULL) {
				// Check if mouse is on button
				PT_Rect pixelRect = { 
					(16 * INVENTORY_LEFT), (16 * (INVENTORY_TOP + 4)), (16 * INVENTORY_WIDTH), (16 * 1)	}; 
				//printf("rect: %d %d w: %d h: %d\n", pixelRect.x, pixelRect.y, pixelRect.w, pixelRect.h);
				if (UI_PointInRect(mousePos.x, mousePos.y, &pixelRect)) {
					printf("Clicked on inventory entry 1\n");
					if (highlightedIdx < 1) { highlightedIdx = 1; }
					//equivalent of 'e' keybind
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

				return;
			}


			// Check if mouse is on button
			PT_Rect pixelRect = { 
				((STATS_WIDTH/2)-5) * 16,
				0,
				9 * 16,
				1 * 16
			}; 
			//printf("rect: %d %d w: %d h: %d\n", pixelRect.x, pixelRect.y, pixelRect.w, pixelRect.h);
			if (UI_PointInRect(mousePos.x, mousePos.y-MAP_HEIGHT*16, &pixelRect)) {
				//printf("Clicked on menu\n");
				//equivalent of 'i' keybind
				if (inventoryView == NULL) {
					show_inventory_overlay(activeScreen);				
				} else {
					hide_inventory_overlay(activeScreen);
				}
			}

			Position *playerPos = (Position *)getComponentForGameObject(player, COMP_POSITION);
			float distance = getDistance(playerPos->x, playerPos->y, mousePos.x/16, mousePos.y/16);
			if (distance < 2){
				int dx = mousePos.x/16-playerPos->x;
				int dy = mousePos.y/16-playerPos->y;
				//usual move code
				PlayerMove(playerPos, dx,dy);
			}
		}
		// right click is the equivalent of 'g' keypress (for now)
		if (event.button.button == SDL_BUTTON_RIGHT) {
			//inventory
			if (inventoryView != NULL) {
				// Check if mouse is on button
				PT_Rect pixelRect = { 
					(16 * INVENTORY_LEFT), (16 * (INVENTORY_TOP + 4)), (16 * INVENTORY_WIDTH), (16 * 1)	}; 
				//printf("rect: %d %d w: %d h: %d\n", pixelRect.x, pixelRect.y, pixelRect.w, pixelRect.h);
				if (UI_PointInRect(mousePos.x, mousePos.y, &pixelRect)) {
					printf("Right clicked on inventory entry 1\n");
					//if (highlightedIdx < 1) { highlightedIdx = 1; }
					//equivalent of 'd' keybind
					if (selIdx > 0) {
						Name *name = NULL;
						GameObject go = gameObjects[selIdx];
						name = (Name *)getComponentForGameObject(&go, COMP_NAME);
						char *msg = String_Create("Player dropped %s", name->name);
						add_message(msg, 0xFFFFFFFF);
						String_Destroy(msg);
						item_drop(&go);
						//close inventory
						hide_inventory_overlay(activeScreen);
					}
				} else {
					//close inventory
					hide_inventory_overlay(activeScreen);
				}

				return;
			}

			GameObject *itemObj = NULL;
			Position *playerPos = (Position *)getComponentForGameObject(player, COMP_POSITION);
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

	}


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
				PlayerMove(playerPos, 0, -1);
			}
		}
        if (key == SDLK_DOWN) { 
			if (inventoryView != NULL) {
				// Handle for inventory view
				highlightedIdx += 1;
				if (highlightedIdx > inventoryLen) { highlightedIdx = inventoryLen; }
			} else {
				// Handle for main in-game view
				PlayerMove(playerPos, 0,1);
			}
			
		}
        if (key == SDLK_LEFT) { 
			PlayerMove(playerPos, -1, 0);
		}
        if (key == SDLK_RIGHT) { 
			PlayerMove(playerPos, 1,0);
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
		if (key == SDLK_e || key == SDLK_SPACE) {
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
		if (key == SDLK_d) {
			if (inventoryView != NULL) {
				if (selIdx > 0) {
					Name *name = NULL;
					GameObject go = gameObjects[selIdx];
					name = (Name *)getComponentForGameObject(&go, COMP_NAME);
					char *msg = String_Create("Player dropped %s", name->name);
					add_message(msg, 0xFFFFFFFF);
					String_Destroy(msg);
					item_drop(&go);
					//close inventory
					hide_inventory_overlay(activeScreen);
				}
			}
		}
		//make Esc context-sensitive
		if (key == SDLK_ESCAPE) {
			if (inventoryView != NULL) {
				hide_inventory_overlay(activeScreen);
			} else {
		 		should_quit = true;
			}
		}
		//save
		if (key == SDLK_s) {
			void (*ptr)() = &_on_save_dummy;
			game_save(ptr);
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
