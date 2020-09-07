/*
* ecs.c
*/


typedef enum {
	COMP_POSITION = 0,
	COMP_RENDERABLE,
	COMP_PHYSICAL,
	COMP_HEALTH,
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


/* World State */
#define MAX_GO 	1000
global_variable GameObject gameObjects[MAX_GO];
global_variable Position positionComps[MAX_GO];
global_variable Renderable renderableComps[MAX_GO];
global_variable Physical physicalComps[MAX_GO];

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


		default:
			assert(1 == 0);
	}

}

void destroyGameObject(GameObject *obj) {
	positionComps[obj->id].objectId = 0;
	renderableComps[obj->id].objectId = 0;
    physicalComps[obj->id].objectId = 0;
	// TODO: Clean up other components used by this object

	obj->id = 0;
}


void *getComponentForGameObject(GameObject *obj, 
								GameComponent comp) {
	return obj->components[comp];
}
