/* Map Management */
// Defines are per file only?
#define MAP_WIDTH	80
#define MAP_HEIGHT	40
global_variable int map[MAP_WIDTH][MAP_HEIGHT]; //it's an int because we use enums (= ints) for tiles

typedef struct {
	u32 x, y;
} Point;

// Tile types; these are used on the map
typedef enum
{
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