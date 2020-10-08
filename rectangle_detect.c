global_variable int (*columns_map)[MAP_HEIGHT] = NULL;
global_variable int (*floor_map)[MAP_WIDTH] = NULL;

//step one of finding biggest area of floor in matrix
void NumUnbrokenFloors_columns() {
	
    //init
    int (* num_columns)[MAP_HEIGHT] = malloc(MAP_WIDTH * MAP_HEIGHT * sizeof(int));

	//actual values
	for (int x = 0; x < MAP_WIDTH; x++) {
		for (int y = 0; y < MAP_HEIGHT; y++) {	

            int north[2];
			north[0] = x + 0;
			north[1] = y -1;
            int add = y == 0 ? 0 : num_columns[north[0]][north[1]];
        
            //ternary: condition ? true : false
            num_columns[x][y] = !is_wall(x,y) ? 1 + add : 0;
		}
	}

	columns_map = num_columns;
}

//transpose the floor_map
// based on https://stackoverflow.com/questions/46534547/transposing-2d-array-in-c
void GetUnbrokenFloors() {
	//init
    int (* f_map)[MAP_WIDTH] = malloc(MAP_WIDTH * MAP_HEIGHT * sizeof(int));


	// transpose matrix
    for (int x = 0; x < MAP_WIDTH; x++)
        for (int y = 0; y < MAP_HEIGHT; y++)
            f_map[y][x] = columns_map[x][y];

	floor_map = f_map;
}