/*
 Main game state
*/

global_variable	bool recalculateFOV = false;

// Necessary forward-declares
//internal void fov_calculate(u32 heroX, u32 heroY, u32 fovMap[][MAP_HEIGHT]);
void add_message(char *msg, u32 color);


/* Low-level game stuff */

//combat
void health_check_death(GameObject *go) {
	Health *h = (Health *)getComponentForGameObject(go, COMP_HEALTH);
	if (h->currentHP <= 0) {
		// Death!
		if (go == player) {
			char *msg = String_Create("You have died.");
			add_message(msg, 0xCC0000FF);
			String_Destroy(msg);

			// TODO: Enter endgame flow

		} else {
			char *msg = String_Create("You killed the [MONSTER].");
			add_message(msg, 0xCC0000FF);
			String_Destroy(msg);

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

	//for player only
	if (attacker == player) {
		Equipment *eq = NULL;
		CombatBonus *combonus = NULL;
		Name *name = NULL;
		//loop over all equipped items
		for (u32 i = 1; i < MAX_GO; i++) {
			GameObject go = gameObjects[i];
			eq = (Equipment *)getComponentForGameObject(&go, COMP_EQUIP);
			combonus = (CombatBonus *)getComponentForGameObject(&go, COMP_COMBAT_BONUS);
			name = (Name *)getComponentForGameObject(&go, COMP_NAME);
			if (eq != NULL && eq->isEquipped) {
				// C is funny about strings, again
				if (strcmp(eq->slot, "hand") == 0) {
					damage += combonus->attack;
					char *msg = String_Create("applying bonus from %s +%i, total %i damage!", name->name, combonus->attack, damage);
					add_message(msg, 0xCC0000FF);
					String_Destroy(msg);
				}
			}
				
		}
	}

	defHealth->currentHP -= damage;

	//printf("%s attacks %s\n", name_att->name, name_def->name);
	char *msg = String_Create("%s attacks %s for %i damage!", name_att->name, name_def->name, damage);
	add_message(msg, 0xCC0000FF);
	String_Destroy(msg);

	health_check_death(defender);
}


//movement
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

GameObject * getItemAtPos(u8 x, u8 y) {
	GameObject *itemObj = NULL;
	Equipment *eq = NULL;
	for (u32 i = 1; i < MAX_GO; i++) {
		Position p = positionComps[i];
		if ((p.objectId > 0) && (p.x == x) && (p.y == y)) {
			GameObject go = gameObjects[i];
			eq = (Equipment *)getComponentForGameObject(&go, COMP_EQUIP);
			if (eq != NULL && !backpackComps[i].objectId > 0) {
				itemObj = (GameObject *) &gameObjects[i];
				//printf("Item found!\n");
				break;
			}
		}
	}
	return itemObj;
}

void item_toggle_equip(GameObject *item) {
	if (item == NULL) { return; }

	Equipment *eq = (Equipment *)getComponentForGameObject(item, COMP_EQUIP);
	if (eq != NULL) {
		eq->isEquipped = !eq->isEquipped;

		if (eq->isEquipped) {
			// Apply any effects of equipping that item?
			//TODO: Unequip any other items in the same slot
		}
	}
}

void item_drop(GameObject *item) {
	if (item == NULL) { return; }

	// Determine player position
	Position *playerPos = (Position *)getComponentForGameObject(player, COMP_POSITION);

	// Assigning it the player position
	Position pos = {.objectId = item->id, .x = playerPos->x, .y = playerPos->y};
	addComponentToGameObject(item, COMP_POSITION, &pos);

	// Unequip the item if equipped
	Equipment *eq = (Equipment *)getComponentForGameObject(item, COMP_EQUIP);
	if (eq->isEquipped) {
		item_toggle_equip(item);
	}

	// Drop the item by removing the backpack component!
	addComponentToGameObject(item, COMP_INBACKPACK, NULL);

	// Display a message to the player
	Name *name = (Name *)getComponentForGameObject(item, COMP_NAME);
	char *msg = String_Create("Player dropped the %s.", name->name);
	add_message(msg, 0x990000ff);
	String_Destroy(msg);

	
}


/* High-level game routines */
internal void
game_new()
{
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


	generate_map();
	
	Point pt = level_get_open_point();
	add_NPC(pt.x, pt.y, 't', 0xFF0000FF, 8, 1, 1);
	pt = level_get_open_point();
	add_NPC(pt.x, pt.y, 't', 0xFF0000FF, 8, 1, 1);

	pt = level_get_open_point();
	add_item(pt.x, pt.y, "combat knife", '/', 0xFFFF00FF, "hand", 2);

	Position *playerPos = (Position *)getComponentForGameObject(player, COMP_POSITION);
	fov_calculate(playerPos->x, playerPos->y, fovMap);
	//Dijkstra map
	generate_Dijkstra_map(playerPos->x, playerPos->y);
}

internal void
game_update() 
{
	if (recalculateFOV) {
		Position *pos = (Position *)getComponentForGameObject(player, COMP_POSITION);
		fov_calculate(pos->x, pos->y, fovMap);
		recalculateFOV = false;
	}
}

void game_save(void (*ptr)()) {	
	printf("Saving game...\n");
	//save to file
	FILE *fp;

	#ifdef __EMSCRIPTEN__
		fp = fopen("/save/world_save.txt", "w+");
		if (fp) {
			printf("Writing to file...\n");
		}
	#else:
		fp = fopen("./world_save.txt", "w+");
	#endif

	for (u32 i = 1; i < MAX_GO; i++) {
		GameObject go = gameObjects[i];
		// only serialize what can change
		Position *pos = (Position *)getComponentForGameObject(&go, COMP_POSITION);
		Health *h = (Health *)getComponentForGameObject(&go, COMP_HEALTH);
		InBackpack *back = (InBackpack *)getComponentForGameObject(&go, COMP_INBACKPACK);

		if (pos != NULL){
			//debug
			printf("[%d]. Pos: x %d y %d\n", i, pos->x, pos->y);
			fprintf(fp, "[%d]. Pos: x %d y %d\n", i, pos->x, pos->y);
		}

		if (h != NULL && h->currentHP > 0) {
			//debug
			printf("[%d]. Current: %d max %d\n", i, h->currentHP, h->maxHP);
			fprintf(fp, "[%d]. Current: %d max %d\n", i, h->currentHP, h->maxHP);
		}

		if (back != NULL) {
			//debug
			printf("[%d]. Backpack.\n", i);
			fprintf(fp, "[%d]. Backpack.\n", i);
		}
		
	}

	fclose(fp);

	#ifdef __EMSCRIPTEN__
	// Don't forget to sync to make sure you store it to IndexedDB
    EM_ASM(
        FS.syncfs(function (err) {
            // Error
			if (!err) {
				console.log("Game saved successfully.")
			}
        });
    );
	#endif

	#ifdef __EMSCRIPTEN__
		fp = fopen("/save/map_save", "wb");
		if (fp) {
			printf("Writing to map file...\n");
		}
	#else
		//dump map to mapfile (NOTE, this one is binary!)
		fp = fopen("./map_save", "wb");
	#endif
	fwrite(seenMap, sizeof(int), MAP_HEIGHT*MAP_WIDTH, fp);

	fclose(fp);

	#ifdef __EMSCRIPTEN__
	// Don't forget to sync to make sure you store it to IndexedDB
    EM_ASM(
        FS.syncfs(function (err) {
            // Error
			if (!err) {
				console.log("Game saved successfully.")
			}
        });
    );
	#endif

	//callback/hook
	(*ptr)();   //calling the callback function

	//now mark the flag that tells the game to quit
	//should_quit = true;
}

void game_load() {
	FILE *fp;
   	char buffer[255];

	#ifdef __EMSCRIPTEN__
   		fp = fopen("/save/world_save.txt", "r");
	#else
		fp = fopen("./world_save.txt", "r");
	#endif

	//handle case where no save file exists
	if (fp == NULL) {
		printf("No save file found...\n");
		return;
	} else {
		//debug
		printf("Loading game...\n");
	}

	// Loop through each line of the file
	while (fgets(buffer, 255, fp) != NULL) {
		//debug
		printf("Read: %s", buffer);
		// if line schema fits what we expect for entities
		if (buffer[0] == '[') {
			buffer[255-1] = '\0';	// Ensure that our buffer string is null-terminated
			// this only works for single digits!
			//int id = buffer[1] - '0';
			//printf("Id: %d\n", id);

			int id;
			int x_pos;
			int y_pos;
			int r = sscanf(buffer, "[%d]. Pos: x %d y %d", &id, &x_pos, &y_pos);
			if (r == 3) {
				printf("Read pos from file: x %d y %d\n", x_pos, y_pos);
				GameObject* go = &gameObjects[id];
				Position pos = {go->id, x_pos, y_pos};
				addComponentToGameObject(go, COMP_POSITION, &pos);
			}

			i32 currHP;
			i32 maxHP;
			r = sscanf(buffer, "[%d]. Current: %d max %d", &id, &currHP, &maxHP);
			if (r == 3) {
				printf("Read hp from file: curr %d max %d\n", currHP, maxHP);
				GameObject* go = &gameObjects[id];
				Health hlth = {.objectId = go->id, .currentHP = currHP, .maxHP = maxHP, .recoveryRate = 1};
				addComponentToGameObject(go, COMP_HEALTH, &hlth);
			}

			r = sscanf(buffer, "[%d]. Backpack", &id);
			if (r == 1 && buffer[5] == 'B') {
				printf("Read backpack from file\n");
				GameObject* go = &gameObjects[id];
				InBackpack back_comp = {.objectId = go->id};
				addComponentToGameObject(go, COMP_INBACKPACK, &back_comp);
			}
		}
	}

	//load the map data
	#ifdef __EMSCRIPTEN__
		fp = fopen("/save/map_save", "rb");
	#else
		fp = fopen("./map_save", "rb");
	#endif
	fread(seenMap, sizeof(int), MAP_HEIGHT*MAP_WIDTH, fp);

	//force FOV refresh
	recalculateFOV = true;
}