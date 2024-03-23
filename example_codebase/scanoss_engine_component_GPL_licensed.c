extern bool first_file;
int binary_scan(char * input)
{
	/* Get file MD5 */
	char * hexmd5 = strndup(input, MD5_LEN * 2);
	scanlog("Bin File md5 to be scanned: %s\n", hexmd5);
	uint8_t bin_md5[MD5_LEN];
	ldb_hex_to_bin(hexmd5, MD5_LEN * 2, bin_md5);
	free(hexmd5);



	uint8_t zero_md5[MD5_LEN] = {0xd4,0x1d,0x8c,0xd9,0x8f,0x00,0xb2,0x04,0xe9,0x80,0x09,0x98,0xec,0xf8,0x42,0x7e}; //empty string md5

	if (!memcmp(zero_md5,bin_md5, MD5_LEN)) //the md5 key of an empty string must be skipped.
		return -1;

	if (ldb_key_exists(oss_file, bin_md5))
	{
		scanlog("bin file md5 match\n");
		char  * file_name = field_n(3,input);
		int target_len = strchr(file_name,',') - file_name;
		char * target = strndup(file_name, target_len);
		scan_data_t * scan =  scan_data_init(target, 1, 1);
		free(target);
		memcpy(scan->md5, bin_md5, MD5_LEN);
		scan->match_type = MATCH_FILE;
		compile_matches(scan);

		if (scan->best_match)
		{
			scanlog("Match output starts\n");
			if (!quiet)
				output_matches_json(scan);

			scan_data_free(scan);
			return 0;
		}
		else
		{
			scanlog("No best match, scanning binary\n");
		}

		scan_data_free(scan);
	}

	binary_match_t result = {NULL, NULL, NULL};
	int sensibility = 1;
	while (sensibility < 100)
	{
		char * bfp = strdup(input);
		result = binary_scan_run(bfp, sensibility);
		free(bfp);
		if (!result.components)
			return -1;
		if (result.components->items > 1 && result.components->headp.lh_first->component->hits > 0)
			break;
		component_list_destroy(result.components);
		free(result.file);
		free(result.md5);
		sensibility++;
	};

	component_list_t * comp_list_sorted = calloc(1, sizeof(component_list_t));
	component_list_init(comp_list_sorted, 10);
	struct comp_entry *item = NULL;
	LIST_FOREACH(item, &result.components->headp, entries)
	{
		component_list_add(comp_list_sorted,item->component, sort_by_hits, false);
	}

	if (!quiet)
		if (!first_file)
			printf(",");
	first_file = false;
	item = NULL;
	printf("\"%s\":{\"hash\":\"%s\",\"id\":\"bin_snippet\",\"matched\":[", result.file, result.md5);
	LIST_FOREACH(item, &comp_list_sorted->headp, entries)
	{
		printf("{\"purl\":\"%s\", \"hits\": %d}",item->component->purls[0], item->component->hits);
		if (item->entries.le_next)
			printf(",");
	}
	printf("]}");
	component_list_destroy(result.components);
	free(result.file);
	free(result.md5);
	free(comp_list_sorted);

	return 0;

}