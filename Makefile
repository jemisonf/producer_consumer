default: producer_consumer.c
	gcc -std=c99 -Wall -pthread producer_consumer.c mt19937ar.c -o producer_consumer
