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

Rect_Area largestRectangleArea(int* heights, int heightsSize){
    // if(heightsSize==0)
    //     return 0;
    stack *s=create_stack(heightsSize);
    int max_area=0;
    int area=0;
    int i=0;
    int top=0;

    int w = 0;

    //dummy
    Rect_Area rect_a = {0,0,0,0,0}; 

    for(i=0;i<heightsSize;){
        if(is_empty(s) || heights[peek(s)]<=heights[i])
            push(s, i++);
        else{
            //it's not higher, pop from stack
            top=pop(s);
            if(is_empty(s)) {
                w = i;
                area=heights[top]*w;
            }
            else {
                w = (i-peek(s)-1);
                area=heights[top]*w;
            }
                
            if(area>max_area && area > 0) {
                max_area=area;
                //this is bottom-up, rect is top-down
                //C99 compound literals
                rect_a = (Rect_Area){i-w,0, w, heights[top], area};
            }
                
        }
    }

    while(!is_empty(s)){
        top=pop(s);

        if(is_empty(s)) {
            w = i;
            area=heights[top]*w;
        }
        else {
            w = (i-peek(s)-1);
            area=heights[top]*w;
        }

        if(area>max_area && area > 0) {
            max_area=area;
            //this is bottom-up, rect is top-down
            //C99 compound literals
            rect_a = (Rect_Area){i-w,0, w, heights[top], area};
        }
           
    }
    free(s);

    //return max_area;
    return rect_a;
}

Rect_Area maximalRectangle(){
    int max_area=0;

    int* dp=(int*)(calloc(sizeof(int),MAP_WIDTH));

    Rect_Area res = {0,0,0,0,0};

    for(int y=0; y<MAP_HEIGHT; y++){
        for(int x=0; x<MAP_WIDTH; x++){
            // note: the values here are our histogram heights
            dp[x] = floor_map[y][x];

            // if(floor_map[y][x]==0)
            //     dp[x]=0;
            // else
            //     dp[x]=floor_map[y][x];
        }
        Rect_Area rec = largestRectangleArea(dp, MAP_WIDTH);

        if(rec.area>max_area)
            max_area=rec.area;
            rec.y = y-rec.h+1; // rect is top-down
            res = rec; //assign to result
    }
    free(dp);


    printf("Rect: x: %d, y: %d, w: %d h: %d area: %d\n", res.x, res.y, res.w, res.h, res.area);
    return res;
    //return max_area;
}