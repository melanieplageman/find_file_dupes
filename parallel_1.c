#include <errno.h>
#include <stdbool.h>
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx/string.h"
#include "mx/vector.h"

FILE *open_file(char *name, char *mode) {
	FILE *file = fopen(name, mode);
	if (file != NULL)
		return file;
	fprintf(stderr, "fopen() failed on %s: %s\n", name, strerror(errno));
	exit(1);
}

bool is_same_file(char *name_1, char *name_2) {
	FILE *file_1 = open_file(name_1, "r");
	FILE *file_2 = open_file(name_2, "r");
	unsigned char buffer_1[2048], buffer_2[2048];

	do {
		size_t size_1 = fread(buffer_1, 1, 2048, file_1);
		size_t size_2 = fread(buffer_2, 1, 2048, file_2);
		if (size_1 != size_2 || memcmp(buffer_1, buffer_2, size_1) != 0) {
			fclose(file_1);
			fclose(file_2);
			return false;
		}
	} while (!feof(file_1) || !feof(file_2));

	bool result = feof(file_1) == feof(file_2);
	fclose(file_1);
	fclose(file_2);
	return result;
}

typedef struct _read_params
{
	size_t start_idx;
	size_t end_idx;
	char **names;
} read_params;

void *do_work(void *work_params)
{
	read_params *params = (read_params *) work_params;

	for (size_t i = params->start_idx; i < params->end_idx; i++)
	{
		mx_string_t name_1 = params->names[i];
		for (size_t j = 0; j < mx_vector_length(params->names); j++)
		{
			mx_string_t name_2 = params->names[j];
			if (strcmp(name_1, name_2) == 0)
				continue;
			if (!is_same_file(name_1, name_2))
				continue;

			printf("%s, %s\n", name_1, name_2);
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	DIR *dirp = opendir("random_data");

	mx_string_t *names = mx_vector_create(sizeof(mx_string_t));

	struct dirent *dirent;
	while ((dirent = readdir(dirp)) != NULL) {
		if (dirent->d_type != DT_REG)
			continue;
		mx_string_t name_1 = mx_string_create(NULL, 0);
		name_1 = mx_string_catf(name_1, "random_data/%s", dirent->d_name);
		names = mx_vector_append(names, &name_1);
	}
	closedir(dirp);

	size_t names_length = mx_vector_length(names);

	read_params *params_1 = malloc(sizeof(read_params));
	params_1->start_idx = 0;
	params_1->end_idx = names_length / 2;
	params_1->names = names;

	pthread_t thread_1;
	pthread_create(&thread_1, NULL, &do_work, params_1);

	read_params *params_2 = malloc(sizeof(read_params));
	params_2->start_idx = names_length / 2;
	params_2->end_idx = names_length;
	params_2->names = names;

	pthread_t thread_2;
	pthread_create(&thread_2, NULL, &do_work, params_2);

	pthread_join(thread_1, NULL);
	pthread_join(thread_2, NULL);

	return 0;
}
