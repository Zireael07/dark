/*
 jsonscript.c 
*/


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

	while (leaf->next != NULL){
		leaf = leaf->next;
		leaf_val = leaf->value;
		ast_to_print(out, leaf_val);
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
