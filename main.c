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

typedef struct _pair_t
{
	bool is_done;
	mx_string_t name_1;
	mx_string_t name_2; 
} pair_t;

pair_t *queue;
size_t queue_head = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t results_mutex = PTHREAD_MUTEX_INITIALIZER;
size_t results_count = 0;
pthread_cond_t results_cond = PTHREAD_COND_INITIALIZER;

void enqueue_pair(pair_t *p) {
	if (pthread_mutex_lock(&mutex) != 0)
		abort();

	queue = mx_vector_append(queue, p);
	
	if (mx_vector_length(queue) == 1)
		pthread_cond_signal(&cond);

	pthread_mutex_unlock(&mutex);
}

void dequeue_pair(pair_t *p) {
	if (pthread_mutex_lock(&mutex) != 0)
		abort();

	if (mx_vector_length(queue) == 0)
		pthread_cond_wait(&cond, &mutex);

	pair_t dequeued = queue[queue_head];
	queue_head++;
	pthread_mutex_unlock(&mutex);
	*p = dequeued;
}

void *thread_main(void *data)
{
	while (true) {
		pair_t p;
		dequeue_pair(&p);
		if (p.is_done)
			return NULL;

		if (strcmp(p.name_1, p.name_2) == 0) {
			pthread_mutex_lock(&results_mutex);
			results_count++;
			pthread_cond_signal(&results_cond);
			pthread_mutex_unlock(&results_mutex);
			continue;
		}
		if (!is_same_file(p.name_1, p.name_2)) {
			pthread_mutex_lock(&results_mutex);
			results_count++;
			pthread_cond_signal(&results_cond);
			pthread_mutex_unlock(&results_mutex);
			continue;
		}

		printf("%s, %s\n", p.name_1, p.name_2);

		pthread_mutex_lock(&results_mutex);
		results_count++;
		pthread_cond_signal(&results_cond);
		pthread_mutex_unlock(&results_mutex);
	}

	return NULL;
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

	queue = mx_vector_create(sizeof(pair_t));

	pthread_t thread_1;
	pthread_create(&thread_1, NULL, &thread_main, NULL);

	pthread_t thread_2;
	pthread_create(&thread_2, NULL, &thread_main, NULL);

	pthread_t thread_3;
	pthread_create(&thread_3, NULL, &thread_main, NULL);

	/* pthread_t thread_4; */
	/* pthread_create(&thread_4, NULL, &thread_main, NULL); */

	names_length = 200;

	for (size_t i = 0; i < names_length; i++)
	{
		for (size_t j = 0; j < names_length; j++)
		{
			pair_t p = { .name_1 = names[i], .name_2 = names[j], .is_done = false };
			enqueue_pair(&p);
		}
	}

	pair_t p = { .is_done = true };
	enqueue_pair(&p);
	enqueue_pair(&p);
	enqueue_pair(&p);
	/* enqueue_pair(&p); */

	pthread_join(thread_1, NULL);
	pthread_join(thread_2, NULL);
	pthread_join(thread_3, NULL);
	/* pthread_join(thread_4, NULL); */

	/* while (true) { */
	/* 	pthread_mutex_lock(&results_mutex); */
	/* 	if (results_count == names_length * names_length) */
	/* 		return 0; */
	/* 	pthread_cond_wait(&results_cond, &results_mutex); */
	/* 	pthread_mutex_unlock(&results_mutex); */
	/* } */

	return 1;
}
