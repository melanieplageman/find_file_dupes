#include <errno.h>
#include <stdbool.h>
#include <dirent.h>
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

	for (size_t i = 0; i < mx_vector_length(names); i++) {
		mx_string_t name_1 = names[i];

		for (size_t j = 0; j < mx_vector_length(names); j++) {
			if (i == j) continue;

			mx_string_t name_2 = names[j];

			if (!is_same_file(name_1, name_2))
				continue;

			printf("%s, %s\n", name_1, name_2);
		}
	}

	return 0;
}
