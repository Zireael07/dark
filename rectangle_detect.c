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

//based on https://leetcode.com/problems/maximal-rectangle/discuss/673436/C-solution-using-maximum-rectangle-area-in-a-histogram-of-every-row

typedef struct stack{
    int top;
    int capacity;
    int* array;
}stack;

bool is_full(stack *s){
    return s->top==s->capacity-1;
}
bool is_empty(stack *s){
    return s->top==-1;
}

stack* create_stack(int capacity){
    stack* s=(stack*)(malloc(sizeof(stack)));
    s->capacity=capacity;
    s->top=-1;
    s->array=(int*)(calloc(sizeof(int), capacity));

    return s;
}

void push(stack *s, int index){
    if(is_full(s))
        return;
    s->array[++(s->top)]=index;
}

int pop(stack *s){
    if(is_empty(s))
        return -1;
    return s->array[(s->top)--];

}
int peek(stack *s){
    if(is_empty(s))
        return -1;
    return s->array[s->top];
}

int largestRectangleArea(int* heights, int heightsSize){
    if(heightsSize==0)
        return 0;
    stack *s=create_stack(heightsSize);
    int max_area=0;
    int area=0;
    int i=0;
    int top=0;

    for(i=0;i<heightsSize;){
        if(is_empty(s) || heights[peek(s)]<=heights[i])
            push(s, i++);
        else{
            top=pop(s);
            if(is_empty(s))
                area=heights[top]*i;
            else
                area=heights[top]*(i-peek(s)-1);
            if(area>max_area)
                max_area=area;
        }
    }

    while(!is_empty(s)){
        top=pop(s);

        if(is_empty(s))
            area=heights[top]*i;
        else
            area=heights[top]*(i-peek(s)-1);

        if(area>max_area)
            max_area=area; 
    }
    free(s);
    return max_area;
}

int maximalRectangle(){
    int area=0, max_area=0;

    int* dp=(int*)(calloc(sizeof(int),MAP_HEIGHT));


    for(int x=0; x<MAP_WIDTH; x++){
        for(int y=0; y<MAP_HEIGHT; y++){
            // note: the values here are our histogram heights
            if(floor_map[y][x]==0)
                dp[y]=0;
            else
                dp[y]+=floor_map[y][x]-0;
        }
        area=largestRectangleArea(dp, MAP_HEIGHT);

        if(area>max_area)
            max_area=area;
    }
    free(dp);
    return max_area;
}