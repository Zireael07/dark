/*
 jsonscript.c 
*/

/* Function definitions (because functions have to be defined before use in C) */
int JLisp_eval(struct json_value_s* value);

//debugging
void ast_to_print(char* out, struct json_value_s* leaf_value){
	//output depending on type
	if (leaf_value->type == json_type_string) {
		struct json_string_s* leaf_str = json_value_as_string(leaf_value);
		char *str = String_Append(out, " %s", leaf_str->string);
		strcpy(out, str);
		//printf(out);
		//printf("Leaf: %s\n", leaf_str->string);
	}
	if (leaf_value->type == json_type_number) {
		struct json_number_s* leaf_num = json_value_as_number(leaf_value);
		char *str = String_Append(out, " %s", leaf_num->number);
		strcpy(out, str);
		//printf("Leaf:%s\n", leaf_num->number); //ASCII representation of number
	}
}

char * JLisp_read() {
	FILE *fp;
	long lSize;
	char *buffer;

	fp = fopen ( "assets/scripts/test.json" , "rb" );
	if( !fp ) { printf("Could not open JSON source"); }

	fseek( fp , 0L , SEEK_END);
	lSize = ftell( fp );
	rewind( fp );

	/* allocate memory for entire content */
	buffer = calloc( 1, lSize+1 );
	if( !buffer ) fclose(fp),printf("memory alloc fails");

	/* copy the file into the buffer */
	if( 1!=fread( buffer , lSize, 1 , fp) )
	fclose(fp),free(buffer),printf("entire read fails");

	//parse
	struct json_value_s* root = json_parse(buffer, strlen(buffer));

	struct json_array_s* list = (struct json_array_s*)root->payload;
	struct json_array_element_s* leaf = list->start;
	struct json_value_s* leaf_val = leaf->value;
	//this will hold the whole string
    //char out[255]
	char* out = String_Create(" ");
	//the first entry
	ast_to_print(out, leaf_val);
	
	assert(root->type == json_type_array);
	JLisp_eval(root);

	while (leaf->next != NULL){
		leaf = leaf->next;
		leaf_val = leaf->value;
		ast_to_print(out, leaf_val);
		//JLisp_eval(leaf_val);
	}

	//print out the whole out
	//printf("%s\n", out);

	//cleanup
	fclose(fp);
	free(buffer);
	/* Don't forget to free the one allocation! */
	free(root);

    return out;
}

void JLisp_print(char* exp) {
    printf("%s\n", exp);
}

// sample functions exposed to scripting
int Add(int a, int b, int c) 
{ return a + b + c; }
int Subtract(int a, int b) 
{ return a - b; }
int Multiply(int a, int b) 
{ return a * b; }
int Divide(int a, int b) 
{ return a / b; }

//based on https://stackoverflow.com/questions/60808663/point-to-functions-with-different-arguments-using-the-same-pointer
typedef struct
{
    char* name;
	int num_args;
    //void (*func)(int);
	int (*func)();
} command_info;
//lookup table aka dispatch table aka environment
command_info command_table[] = {{"+", 3, &Add}, {"-", 2, &Subtract}, {"*", 2, &Multiply}, {"/", 2, &Divide} };


int JLisp_eval(struct json_value_s* value){
	if (value->type == json_type_null) {
		printf("Error, value is NULL\n");
	}
	if (value->type == json_type_array) {
		printf("Value is array\n");
		struct json_array_s* list = json_value_as_array(value);
		
		//operator is always the first
		struct json_array_element_s* leaf = list->start;
		struct json_value_s* leaf_val = leaf->value;
		struct json_string_s* leaf_str = json_value_as_string(leaf_val);
		char *str = leaf_str->string;
		printf("Op: %s\n", str);
		//remaining children are parameters
		int args[20];
		int cnt = 0;
		while (leaf->next != NULL){
			leaf = leaf->next;
			leaf_val = leaf->value;
			if (leaf_val->type == json_type_number) {
				struct json_number_s* leaf_num = json_value_as_number(leaf_val);
				int i = atoi(leaf_num->number);
				args[cnt] = i;
				cnt += 1;
			}
		}
		printf("Args: %d %d %d\n", args[0], args[1], args[2]);

		//TODO: get rid of that loop by making command_table a hashmap
		int i;
		for(i = 0; i < 4; i++) {
			if (strcmp(str, command_table[i].name) == 0) {
				//check number of parameters
				if (command_table[i].num_args == cnt) {
					int res = -1;
					//call it
					if (cnt == 2){
						res = command_table[i].func(args[0], args[1]);
					}
					
					if (cnt == 3){
						res = command_table[i].func(args[0], args[1], args[2]);
					}
					printf("Match! %s\n", command_table[i].name);
					printf("Result: %d\n", res);
				}
				else {
					printf("Wrong number of parameters given for command %s", command_table[i].name);
				}
				break; //stop the loop
			}
		}

	}


	if (value->type == json_type_string) {
		struct json_string_s* leaf_str = json_value_as_string(value);
		char *str = leaf_str->string;
		printf("Leaf: %s\n", str);
		//look it up in the command table
		int i;
		int res = -1;
		//TODO: get rid of that loop by making command_table a hashmap
		for(i = 0; i < 4; i++) {
			if (strcmp(str, command_table[i].name) == 0) {
				//call it
				res = command_table[i].func();
				printf("Match! %s\n", command_table[i].name);
				printf("Result: %d\n", res);
				break; //stop the loop
				
			} else {
				//debug
				printf("No match: %s\n", command_table[i].name);
			}
		}
		//it wasn't found
		if (res == -1) {
			// 		printf("Function %s not found in command table", str);
			//return NULL;
		}

		//return res;
	}
	if (value->type == json_type_number) {
		struct json_number_s* leaf_num = json_value_as_number(value);
		int i = atoi(leaf_num->number);
		printf("Eval num: %d\n", i);
	}
}
