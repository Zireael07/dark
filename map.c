/* Map Management */
// Defines are per file only?
#define MAP_WIDTH	80
#define MAP_HEIGHT	40
global_variable int map[MAP_WIDTH][MAP_HEIGHT]; //it's an int because we use enums (= ints) for tiles
global_variable i32 (*targetMap)[MAP_HEIGHT] = NULL;

typedef struct {
	u32 x, y;
} Point;

// Tile types; these are used on the map
typedef enum
{
  tile_error,
  tile_floor,
  tile_wall,
} tile_t;

/*
  Returns the tile at (X,Y).
*/
int get_tile(int x, int y)
{
  if (y < 0 || y >= MAP_HEIGHT || x < 0 || x >= MAP_WIDTH)
    return tile_wall;
  
  return map[x][y];
}

/*
  Sets the tile at (X,Y) to TILE. Checks for out of bounds, so there's
  no risk writing outside the map.
*/
void set_tile(int x, int y, int tile)
{
  if (y < 0 || y > MAP_HEIGHT || x < 0 || x > MAP_WIDTH)
    return;
  
  map[x][y] = tile;
  //printf("Tile at x %i, y %i: %i", x, y, map[x][y]);

  return;
}

Point level_get_open_point() {
	// Return a random position within the level that is open
	for (;;) {
		u32 x = rand() % MAP_WIDTH;
		u32 y = rand() % MAP_HEIGHT;
		if (map[x][y] == tile_floor) {
			return (Point) {x, y};
		}
	}
}

void generate_map() {
	int x;
	int y;

	// Make a wall all around the edge and fill the rest with floor tiles.
  	for (x = 0; x < MAP_WIDTH; x++)
  	{
     	for (y = 0; y < MAP_HEIGHT; y++)	
    	{
			if (y == 0 || x == 0 || y == MAP_HEIGHT - 1 || x == MAP_WIDTH - 1) {
				set_tile(x, y, tile_wall);
			}
			else {
				set_tile(x, y, tile_floor);
			}
		}
  	}
}

bool is_wall(i32 x, i32 y) {
	return get_tile(x,y) == tile_wall;
}

void generate_Dijkstra_map(i32 targetX, i32 targetY) {
	i32 (* dmap)[MAP_HEIGHT] = malloc(MAP_WIDTH * MAP_HEIGHT * sizeof(i32));
	i32 UNSET = 9999;

	for (i32 x = 0; x < MAP_WIDTH; x++) {
		for (i32 y = 0; y < MAP_HEIGHT; y++) {
			dmap[x][y] = UNSET;
		}
	}

	// Set our target point(s)
	dmap[targetX][targetY] = 0;

	// Calculate our target map
	bool changesMade = true;
	while (changesMade) {
		changesMade = false;

		for (i32 x = 0; x < MAP_WIDTH; x++) {
			for (i32 y = 0; y < MAP_HEIGHT; y++) {
				i32 currCellValue = dmap[x][y];
				if (currCellValue != UNSET) {
					// Check cells around this one and update them if warranted
					if ((!is_wall(x+1, y)) && (dmap[x+1][y] > currCellValue + 1)) { 
						dmap[x+1][y] = currCellValue + 1;
						changesMade = true;
					}
					if ((!is_wall(x-1, y)) && (dmap[x-1][y] > currCellValue + 1)) { 
						dmap[x-1][y] = currCellValue + 1;
						changesMade = true;
					}
					if ((!is_wall(x, y-1)) && (dmap[x][y-1] > currCellValue + 1)) { 
						dmap[x][y-1] = currCellValue + 1;
						changesMade = true;
					}
					if ((!is_wall(x, y+1)) && (dmap[x][y+1] > currCellValue + 1)) { 
						dmap[x][y+1] = currCellValue + 1;
						changesMade = true;
					}
				}
			}
		}
	}

	targetMap = dmap;
}