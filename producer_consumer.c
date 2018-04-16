#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>



struct buffer_item {
	int num;
	int wait_period;
};

struct buffer {
	struct buffer_item items[32];
	int size;
};

sem_t mutex, items, spaces;
struct buffer_item buffer[32];
struct buffer buff;

int rdrand32_step (uint32_t *rand) {
	unsigned char ok;
	__asm__ __volatile__ ("rdrand %0; setc %1"
			: "=r" (*rand), "=qm" (ok));
	return (int) ok;
}

// returns a positive randum number n such that min <= n <= max
// if no min/max is required, pass -1 for that value instead 
int get_rand_num(int min, int max) {
	unsigned int num;
	unsigned int eax;
	unsigned int ebx;
	unsigned int ecx;
	unsigned int edx;


	eax = 0x01;

	__asm__ __volatile__(
			"cpuid;"
			: "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
			: "a"(eax)
			);

	if (ecx & 0x40000000){
		//use rdrand
		rdrand32_step(&num);

	} else {
		// use mt
		num = genrand_int32();
	}
	if (max > 0) 
			num = num % (max - min);
	if (min > 0) 
		num += min;
	return num;
}
struct buffer_item get_item_from_buffer() {
	int buff_size, i;
	struct buffer_item extracted_item;
	sem_getvalue(&items, &buff_size);
	extracted_item = buff.items[0];
	if (buff.size > 1) {
		for (i = 1; i <= buff.size; i++) {
			// buffer[i - 1] = buffer[i];
			buff.items[i - 1] = buff.items[i];
		}
	}
	buff.size--;
	return extracted_item;

}

void add_item_to_buffer(struct buffer_item new_item) {
	// int buff_size;
	// sem_getvalue(&items, &buff_size);
	printf("Buff size: %d\n", buff.size);
	// buffer[buff_size] = new_item;
	buff.items[buff.size] = new_item;
	buff.size++;
}

struct buffer_item get_buffer_item() {
	struct buffer_item new_buffer_item;
	new_buffer_item.num = get_rand_num(-1, -1);
	new_buffer_item.wait_period = get_rand_num(2, 9);
	return new_buffer_item;
}

void* producer(void* args) {
	struct buffer_item new_item;
	int sem_val;
	printf("Producer\n");
	while(1) {
		sem_getvalue(&items, &sem_val);
		// get event
		new_item = get_buffer_item();
		// wait for mutex semaphore
		sem_wait(&spaces);
		sem_wait(&mutex);
		// add event to buffer
		printf("Adding item with num %d and wait %d\n", new_item.num, new_item.wait_period);
		fflush(stdout);
		//buffer[sem_val - 1] = new_item;
		add_item_to_buffer(new_item);
		// signal items semaphore
		sem_post(&items);
		// signal mutex semaphore
		sem_post(&mutex);
		sem_val = get_rand_num(2, 7);
		printf("Producer: sleeping for %d seconds\n", sem_val);
		sleep(sem_val);
	}
}

void* consumer(void* args) {
	struct buffer_item item;
	int sem_val;
	printf("Consumer\n");
	while(1) {
		// wait for items semaphore
		sem_wait(&items);
		// wait for mutex semaphore
		sem_wait(&mutex);
		// get event from buffer
		sem_getvalue(&items, &sem_val);
		item = get_item_from_buffer();
		// signal mutex semaphore
		sem_post(&mutex);
		sem_post(&spaces);
		// process event
		sleep(item.wait_period);
		printf("Val: %d, slept for %d seconds\n", item.num, item.wait_period);
		fflush(stdout);
	}

}

int main(int arc, char* argv[]) {
	// struct timezone tz;
	struct timeval tv;
	long seed;
	gettimeofday(&tv, NULL);
	seed = tv.tv_usec;
	printf("Seed: %ld\n", seed);
	pthread_t producer_thread;
	init_genrand(seed);
	buff.size = 0;
	sem_init(&mutex, 0, 1);	
	sem_init(&items, 0, 0);
	sem_init(&spaces, 0, 32);
	pthread_create(&producer_thread, NULL, producer, NULL);
	consumer(NULL);
	return 0;
}
