
typedef struct {
    char* name;
	char* glyph;
	bool blocks;
	int hp;
	int def;
	int pow;
	int r;
	int g;
	int b;
} JSON_data;

global_variable JSON_data loadedNPCs[200];

JSON_data *create_JSON_data() {
	JSON_data *jd = NULL;
	//find first empty spot
	for (u32 i = 0; i < 200; i++) {
		if (loadedNPCs[i].hp == NULL) {
			jd = &loadedNPCs[i];
			break;
		}
	}
	assert(jd != NULL);
	return jd;
}

void JSON_parse_npc(struct json_object_element_s* npc){
	JSON_data *npc_data = create_JSON_data();

	struct json_string_s* npc_name = npc->name;
	printf("Parsing npc: %s\n", npc_name->string);
	struct json_object_s* data = json_value_as_object(npc->value);
	struct json_object_element_s* name = data->start;
	struct json_string_s* inner_name = json_value_as_string(name->value);
	//printf("Inner: %s\n", inner_name->string);
	npc_data->name = inner_name->string;

	struct json_object_element_s* renderable = name->next;
	//struct json_string_s* render_name = renderable->name;
	//printf("Name: %s\n", render_name->string);
	struct json_object_s* render = json_value_as_object(renderable->value);
	struct json_object_element_s* glyph = render->start;
	struct json_string_s* glyph_value = json_value_as_string(glyph->value);
	//printf("Glyph: %s\n", glyph_value->string);
	npc_data->glyph = glyph_value->string;
	struct json_object_element_s* fg_o = glyph->next;
	//printf("Fg: %s\n", fg_o->name->string);
	struct json_array_s* fg = json_value_as_array(fg_o->value);
	struct json_array_element_s* fg_el = fg->start;
	struct json_number_s* fg_r = json_value_as_number(fg_el->value);
	struct json_number_s* fg_g = json_value_as_number(fg_el->next->value);
	struct json_number_s* fg_b = json_value_as_number(fg_el->next->next->value);
	printf("Rgb: %s %s %s\n", fg_r->number, fg_g->number, fg_b->number);
	npc_data->r = atoi(fg_r->number);
	npc_data->g = atoi(fg_g->number);
	npc_data->b = atoi(fg_b->number);
	struct json_object_element_s* blocks = renderable->next;
	struct json_string_s* blocks_value = json_value_is_true(blocks->value); // returns non-zero if true else 0
	//printf("Blocks: %d\n", blocks_value);
	npc_data->blocks = (blocks_value != 0 ? true : false);
	struct json_object_element_s* stats_block = blocks->next;
	struct json_object_s* stats = json_value_as_object(stats_block->value);
	struct json_object_element_s* hp = stats->start;
	struct json_number_s* hp_value = json_value_as_number(hp->value);
	//printf("HP: %s\n", hp_value->number);
	npc_data->hp = atoi(hp_value->number);
	struct json_object_element_s* defense = hp->next;
	struct json_number_s* def_val = json_value_as_number(defense->value);
	//printf("Def: %s\n", def_val->number);
	npc_data->def = atoi(def_val->number);
	struct json_object_element_s* pow = defense->next;
	struct json_number_s* pow_val = json_value_as_number(pow->value);
	//printf("Pow: %s\n", pow_val->number);
	npc_data->pow = atoi(pow_val->number);
	//debug
	printf("NPC data: name %s glyph %s hp %d def %d pow %d\n", npc_data->name, npc_data->glyph, npc_data->hp, npc_data->def, npc_data->pow);

	//after stats, some entries are optional
	struct json_object_element_s* next = stats_block->next;
	if (strcmp(next->name->string, "faction") == 0) {
		struct json_string_s* fact_val = json_value_as_string(next->value);
		printf("Faction: %s\n", fact_val->string);
	}
	else {
		struct json_object_element_s* eq = next;
		struct json_array_s* equip_list = json_value_as_array(eq->value);
		struct json_array_element_s* item = equip_list->start;
		struct json_string_s* item_str = json_value_as_string(item->value);
		printf("Item: %s\n", item_str->string);
		while (item->next != NULL){
			item = item->next;
			item_str = json_value_as_string(item->value);
			printf("Item: %s\n", item_str->string);
		}
	}
	if (next->next != NULL){
		if (strcmp(next->next->name->string, "equipment") == 0){
			struct json_object_element_s* eq = next->next;
			struct json_array_s* equip_list = json_value_as_array(eq->value);
			struct json_array_element_s* item = equip_list->start;
			struct json_string_s* item_str = json_value_as_string(item->value);
			printf("Item: %s\n", item_str->string);
			while (item->next != NULL){
				item = item->next;
				item_str = json_value_as_string(item->value);
				printf("Item: %s\n", item_str->string);
			}
		}
	}

}


void JSON_load(){
	//open file (https://stackoverflow.com/questions/3747086/reading-the-whole-text-file-into-a-char-array-in-c)
	
	FILE *fp;
	long lSize;
	char *buffer;

	fp = fopen ( "assets/npcs.json" , "rb" );
	if( !fp ) { printf("Could not open JSON"); }

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

	struct json_object_s* object = (struct json_object_s*)root->payload;

	//get the first NPC and parse it
	struct json_object_element_s* npc = object->start;
	JSON_parse_npc(npc);
	
	//get all others
	while (npc->next != NULL){
		npc = npc->next;
		JSON_parse_npc(npc);

	}

	fclose(fp);
	free(buffer);
	/* Don't forget to free the one allocation! */
	free(root);

	//debug
	JSON_data *npc_data = &loadedNPCs[0];
	printf("NPC data after load: name %s glyph %s hp %d def %d pow %d\n", npc_data->name, npc_data->glyph, npc_data->hp, npc_data->def, npc_data->pow);
}
