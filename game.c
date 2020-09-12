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

	
	Point pt = level_get_open_point();
	add_NPC(pt.x, pt.y, 't', 0xFF0000FF, 8, 1, 1);
	pt = level_get_open_point();
	add_NPC(pt.x, pt.y, 't', 0xFF0000FF, 8, 1, 1);

	pt = level_get_open_point();
	add_item(pt.x, pt.y, "combat knife", '/', 0xFFFF00FF, "hand", 2);

	generate_map();

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