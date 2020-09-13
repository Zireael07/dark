/*
* ecs.c
*/


typedef enum {
	COMP_POSITION = 0,
	COMP_RENDERABLE,
	COMP_PHYSICAL, // do we block move?
	COMP_NPC,
	COMP_NAME,
	COMP_HEALTH,
	COMP_COMBAT,
	COMP_EQUIP,
	COMP_COMBAT_BONUS,
	COMP_INBACKPACK,
	COMP_MOVEMENT,

	/* Define other components above here */
	COMPONENT_COUNT
} GameComponent;


/* Entity */
typedef struct {
	u32 id;
	void *components[COMPONENT_COUNT];
} GameObject;


/* Components */
typedef struct {
	u32 objectId;
	u8 x, y;	
} Position;

typedef struct {
	u32 objectId;
	asciiChar glyph;
	u32 fgColor;
	u32 bgColor;
} Renderable;

typedef struct {
	u32 objectId;
	bool blocksMovement;
	bool blocksSight;
} Physical;

typedef struct {
	u32 objectId;
	const char* name; //no string type in C!
} Name;

//C doesn't support empty structs
typedef struct {
	u32 objectId;
} NPC;

typedef struct {
	i32 objectId;
	i32 currentHP;
	i32 maxHP;
	i32 recoveryRate;		// HP recovered per tick.
} Health;

typedef struct {
	i32 objectId;
	i32 attack;				// attack = damage inflicted per hit
	i32 defense;			// defense = damage absorbed before HP is affected
} Combat;

typedef struct {
	i32 objectId;
	i32 quantity;
	i32 weight;
	char *slot;
	bool isEquipped;
} Equipment;

typedef struct {
	i32 objectId;
	i32 attack;				// attack = damage inflicted per hit
	i32 defense;			// defense = damage absorbed before HP is affected
} CombatBonus;

typedef struct {
	u32 objectId;
} InBackpack;

/* World State */
#define MAX_GO 	1000
global_variable GameObject *player = NULL;
global_variable GameObject gameObjects[MAX_GO];
global_variable Position positionComps[MAX_GO];
global_variable Renderable renderableComps[MAX_GO];
global_variable Physical physicalComps[MAX_GO];
global_variable NPC NPCComps[MAX_GO];
global_variable Name nameComps[MAX_GO];
global_variable Health healthComps[MAX_GO];
global_variable Combat combatComps[MAX_GO];
global_variable Equipment equipComps[MAX_GO];
global_variable CombatBonus combonusComps[MAX_GO];
global_variable InBackpack backpackComps[MAX_GO];

/* Game Object Management */
GameObject *createGameObject() {
	// Find the next available object space
	GameObject *go = NULL;
	for (u32 i = 1; i < MAX_GO; i++) {
		if (gameObjects[i].id == 0) {
			go = &gameObjects[i];
			go->id = i;
			break;
		}
	}

	assert(go != NULL);

	for (u32 i = 0; i < COMPONENT_COUNT; i++) {
		go->components[i] = NULL;
	}

	return go;
}

void addComponentToGameObject(GameObject *obj, 
							  GameComponent comp,
							  void *compData) {
	assert(obj->id != -1);

	switch (comp) {
		case COMP_POSITION: {
            Position *pos = &positionComps[obj->id];
			Position *posData = (Position *)compData;
			pos->objectId = obj->id;
			pos->x = posData->x;
			pos->y = posData->y;

			obj->components[comp] = pos;

			break;
        }	

		case COMP_RENDERABLE: {
            Renderable *rnd = &renderableComps[obj->id];
			Renderable *rndData = (Renderable *)compData;
			rnd->objectId = obj->id;
			rnd->glyph = rndData->glyph;
			rnd->fgColor = rndData->fgColor;
			rnd->bgColor = rndData->bgColor;

			obj->components[comp] = rnd;

			break;
        }

        case COMP_PHYSICAL: {
			Physical *phys = &physicalComps[obj->id];
			Physical *physData = (Physical *)compData;
			phys->objectId = obj->id;
			phys->blocksSight = physData->blocksSight;
			phys->blocksMovement = physData->blocksMovement;

			obj->components[comp] = phys;

			break;
        }

		case COMP_NAME: {
			Name *name = &nameComps[obj->id];
			Name *nameData = (Name *)compData;
			name->objectId = obj->id;
			//name->name = nameData->name; //this doesn't work in C!
			//workaround for the above
			if (nameData->name != NULL) {
					//strlen() does NOT include the null termination
					//character, so always malloc() the reported length + 1!!
					name->name = calloc(strlen(nameData->name) + 1, sizeof(char));
					strcpy(name->name, nameData->name);
				}

			obj->components[comp] = name;

			break;
		}

		case COMP_NPC: {
			NPC *npc = &NPCComps[obj->id];
			NPC *npcData = (NPC *)compData;
			npc->objectId = obj->id;

			obj->components[comp] = npc;
			break;
		}

		case COMP_HEALTH: {
			Health *hlth = &healthComps[obj->id];
			Health *hlthData = (Health *)compData;
			hlth->objectId = obj->id;
			hlth->currentHP = hlthData->currentHP;
			hlth->maxHP = hlthData->maxHP;
			hlth->recoveryRate = hlthData->recoveryRate;

			obj->components[comp] = hlth;
			break;
		}

		case COMP_COMBAT: {
			Combat *com = &combatComps[obj->id];
			Combat *combatData = (Combat *)compData;
			com->objectId = obj->id;
			com->attack = combatData->attack;
			com->defense = combatData->defense;

			obj->components[comp] = com;
			break;
		}

		case COMP_EQUIP: {
			Equipment *eq = &equipComps[obj->id];
			Equipment *eqData = (Equipment *)compData;
			eq->objectId = obj->id;
			eq->quantity = eqData->quantity;
			eq->weight = eqData->weight;
			if (eqData->slot != NULL) {
				eq->slot = malloc(strlen(eqData->slot) + 1);
				strcpy(eq->slot, eqData->slot);
			}
			eq->isEquipped = eqData->isEquipped;

			obj->components[comp] = eq;
			break;
		}

		case COMP_COMBAT_BONUS: {
			CombatBonus *combon = &combonusComps[obj->id];
			CombatBonus *combatbonusData = (CombatBonus *)compData;
			combon->objectId = obj->id;
			combon->attack = combatbonusData->attack;
			combon->defense = combatbonusData->defense;

			obj->components[comp] = combon;
			break;
		}

		case COMP_INBACKPACK: {
			if (compData != NULL) {
				InBackpack *back = &backpackComps[obj->id];
				InBackpack *backData = (InBackpack *)compData;
				back->objectId = obj->id;

				obj->components[comp] = back;
			} else {
				// Clear component 
				InBackpack *back = obj->components[COMP_INBACKPACK];
				if (back != NULL) {
					obj->components[COMP_INBACKPACK] = NULL;
					backpackComps[obj->id].objectId = 0;
				}
			}
			break;
		}

		default:
			assert(1 == 0);
	}

}

void destroyGameObject(GameObject *obj) {
	positionComps[obj->id].objectId = 0;
	renderableComps[obj->id].objectId = 0;
    physicalComps[obj->id].objectId = 0;
	nameComps[obj->id].objectId = 0;
	NPCComps[obj->id].objectId = 0;
	healthComps[obj->id].objectId = 0;
	combatComps[obj->id].objectId = 0;
	equipComps[obj->id].objectId = 0;
	combonusComps[obj->id].objectId = 0;
	backpackComps[obj->id].objectId = 0;
	// TODO: Clean up other components used by this object

	obj->id = 0;
}


void *getComponentForGameObject(GameObject *obj, 
								GameComponent comp) {
	return obj->components[comp];
}

//helpers to avoid problems assigning our structs
void add_NPC(u8 x, u8 y, asciiChar glyph, u32 fgColor, i32 maxHP, i32 attack, i32 defense) {
	GameObject *npc = createGameObject();
	Position pos = {.objectId = npc->id, .x = x, .y = y};
	printf("NPC: x: %d y: %d\n", x, y);
	addComponentToGameObject(npc, COMP_POSITION, &pos);
	Renderable rnd = {.objectId = npc->id, .glyph = glyph, .fgColor = fgColor, .bgColor = 0x00000000};
	addComponentToGameObject(npc, COMP_RENDERABLE, &rnd);
	Physical phys = {.objectId = npc->id, .blocksMovement = true, .blocksSight = false};
	addComponentToGameObject(npc, COMP_PHYSICAL, &phys);
	Name name = {.objectId = npc->id};
	name.name = "Thug"; // we can't initialize strings in C!
	//printf("%s", name.name);
	addComponentToGameObject(npc, COMP_NAME, &name);
	NPC npc_comp = {.objectId = npc->id};
	addComponentToGameObject(npc, COMP_NPC, &npc_comp);
	Health hlth = {.objectId = npc->id, .currentHP = maxHP, .maxHP = maxHP, .recoveryRate = 0};
	addComponentToGameObject(npc, COMP_HEALTH, &hlth);
	Combat com = {.objectId = npc->id, .attack = attack, .defense = defense};
	addComponentToGameObject(npc, COMP_COMBAT, &com);
}

void add_item(u8 x, u8 y, char *name, asciiChar glyph, u32 fgColor, char *slot,
	i32 attack) {
	GameObject *item = createGameObject();
	Position pos = {.objectId = item->id, .x = x, .y = y};
	printf("x: %d y: %d\n", x, y);
	addComponentToGameObject(item, COMP_POSITION, &pos);
	Renderable rnd = {.objectId = item->id, .glyph = glyph, .fgColor = fgColor, .bgColor = 0x00000000};
	addComponentToGameObject(item, COMP_RENDERABLE, &rnd);
	Physical phys = {.objectId = item->id, .blocksMovement = false, .blocksSight = false};
	addComponentToGameObject(item, COMP_PHYSICAL, &phys);
	Name nam = {.objectId = item->id};
	nam.name = name; // we can't initialize strings in C!
	printf("item: %s", nam.name);
	addComponentToGameObject(item, COMP_NAME, &nam);
	Equipment eq = {.objectId = item->id, .quantity = 1, .weight = 0, .slot = slot};
	addComponentToGameObject(item, COMP_EQUIP, &eq);
	CombatBonus combonus = {.objectId = item->id, .attack = attack, .defense = 0};
	addComponentToGameObject(item, COMP_COMBAT_BONUS, &combonus);
}