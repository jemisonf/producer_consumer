default: producer_consumer.c
	gcc -Wall -lpthread producer_consumer.c mt19937ar.c -o producer_consumer
