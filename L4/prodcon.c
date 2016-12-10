#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "buffer.h"
#include "prodcon-api.h"
#include <assert.h>
#include <time.h>

//struct for thread arguments

typedef struct{
	int start;
	int stop;
	int delay;
	int upper,lower;
	int id;
} thread_args_t;

//Function prototypes
void* consumer(void* args);
void* producer(void* args);
char* convert(char* string);
void* observe(void* args);

//mutex and cv
pthread_cond_t cv;
pthread_mutex_t mutex;

//global variables
Buffer buffer, inputBuffer;
time_t sec;
bool verbose = false;
pthread_barrier_t barr;

typedef struct
{
	int** access;
	int pt;
} synchronize;

//global observable
synchronize s;

int
main (int argc, char **argv)
{

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cv, NULL);

	int c; //argument
	char* input = NULL;
	char* output;
	
	int bufferRows = 20; //default
	int colsPerRows = 20; //default
	
	int consumerThreads = 1;
	int producerThreads = 1;
	
	int delay = 2000; // number of millisecs for the passvie sleep
	int upperBorder = 1; //default in sec
	int lowerBorder = 1; //default in sec
	int busyLoopFactor = 0; // default	
	
	srandom((unsigned int) sec);
	
	while ((c = getopt (argc, argv, "vhi:o:L:C:c:p:t:r:R:a:")) != -1)
		switch (c)
		  {
		  case 'v':
		  verbose = true;
			  break;
		  case 'h':
			printf("HELP\n"); // todo:
			printf ("Current Git HEAD commit number: \n");
			const char *gitversion = "git rev-parse HEAD";
			system (gitversion);
			  break;
		  case 'i':
			  input = optarg;
			  break;
		  case 'o':
			  output = optarg;
			  break;
		  case 'L':
			  bufferRows =  atoi(optarg);
			  break;
		  case 'C':
			  colsPerRows = atoi(optarg);
			  break;
		  case 'c':
			  consumerThreads = atoi(optarg);
			  break;
		  case 'p':
		      producerThreads =  atoi(optarg);
			  break;
		  case 't':
			  delay = atoi(optarg);
			  break;
		  case 'r':
			  lowerBorder = atoi(optarg);
			  break;
		  case 'R':
			  upperBorder = atoi(optarg);
			  break;
		  case 'a':
			  busyLoopFactor = atoi(optarg);
			  break;
		  case '?':
			  if (optopt == 'i')
				  fprintf (stderr,
					   "Option -%c requires an argument.\n",
					   optopt);
			  else if (optopt == 'o')
				  fprintf (stderr,
					   "Option -%c requires an argument.\n",
					   optopt);
			  else if (optopt == 'L')
				  fprintf (stderr,
					   "Option -%c requires an argument.\n",
					   optopt);
			  else if (optopt == 'C')
				  fprintf (stderr,
					   "Option -%c requires an argument.\n",
					   optopt);
			  else if (optopt == 'c')
				  fprintf (stderr,
					   "Option -%c requires an argument.\n",
					   optopt);
			  else if (optopt == 'p')
				  fprintf (stderr,
					   "Option -%c requires an argument.\n",
					   optopt);
			  else if (optopt == 't')
				  fprintf (stderr,
					   "Option -%c requires an argument.\n",
					   optopt);
			  else if (optopt == 'r')
				  fprintf (stderr,
					   "Option -%c requires an argument.\n",
					   optopt);
			  else if (optopt == 'R')
				  fprintf (stderr,
					   "Option -%c requires an argument.\n",
					   optopt);
			  else if (optopt == 'a')
				  fprintf (stderr,
					   "Option -%c requires an argument.\n",
					   optopt);

			  else if (isprint (optopt))
				  fprintf (stderr, "Unknown option `-%c'.\n",
					   optopt);
			  else
				  fprintf (stderr,
					   "Unknown option character `\\x%x'.\n",
					   optopt);
			  return 1;
		  default:
			  abort ();
		  }
		  
	initBuffer(&inputBuffer, bufferRows, colsPerRows);
	if(input != NULL)
	{
		printf("[READ FROM FILE]\n"); 
		readFile(&inputBuffer, input);
	}
	else
	{
		printf("[READ FROM STDIN]\n"); 
		readStdin(&inputBuffer);
	}
	
	pthread_t consumers[consumerThreads];
	pthread_t producers[producerThreads];
	thread_args_t argsProducer[producerThreads];
	thread_args_t argsConsumer[consumerThreads];
	
	pthread_barrier_init(&barr, NULL, consumerThreads);
	
	s.access = (int**) malloc(sizeof(int**) * producerThreads);
	assert(access != NULL);
	for(int i = 0; i < producerThreads; ++i)
	{
		s.access[i] = (int*) malloc(sizeof(int) * 2);
		assert(s.access[i] != NULL);
		
	}
	
	initBuffer(&buffer, bufferRows, colsPerRows);
	
	printf("%d Producer startet\n", producerThreads);
	for(int i = 0; i < producerThreads; ++i)
	{
		argsProducer[i].start = i * (inputBuffer.tail / producerThreads);
		argsProducer[i].stop = (i+1) * (inputBuffer.tail / producerThreads);
		argsProducer[i].delay = delay;
		argsProducer[i].id = i;
		pthread_create(&producers[i], NULL, (void *(*)(void *))producer, &argsProducer[i]);
	}
	
	printf("%d Consumer startet\n", consumerThreads);
	for(int i = 0; i < consumerThreads; ++i)
	{
		argsConsumer[i].id = i;
		argsConsumer[i].upper = upperBorder;
		argsConsumer[i].lower = lowerBorder;
		pthread_create(&consumers[i], NULL, (void *(*)(void *))consumer, &argsConsumer[i]);
	}
	
	//joins
	
	for(int i = 0; i < producerThreads; ++i)
	{
		pthread_join(producers[i], NULL);
	}
	
	for(int i = 0; i < consumerThreads; ++i)
	{
		pthread_join(consumers[i], NULL);
	}
	
	destroyBuffer(&buffer);
	destroyBuffer(&inputBuffer);
	
	
	//free access
	for(int i = 0; i < producerThreads; ++i)
		free(s.access[i]);
	free(s.access);
	return 0;
}

void* consumer(void* args)
{
	thread_args_t* arg = (thread_args_t*) args;
	
	pthread_barrier_wait(&barr);
	pthread_mutex_lock(&mutex);
	char* c = pop(&buffer);
	printf("C%d consumes %s\n",arg->id, c);
	
	time(&sec);
	int s = (random () % arg->upper) + arg->lower;
	sleep (s);

	//wait busy or passive
	
	printf("C%d reports (time) %s\n",arg->id, convert(c)); //needs id
	
	pthread_mutex_unlock(&mutex);
	pthread_exit(0);
}

void* producer(void* args)
{
	thread_args_t* arg = (thread_args_t*) args;
	for(int i = arg->start; i  < arg->stop; ++i)
	{
		pthread_mutex_lock(&mutex);
		//read from input
		char* input = pop(&inputBuffer);
		sleep(arg->delay/1000);
		add(&buffer, input);
		printf("P%d produces %s\n", arg->id, inputBuffer.queue[i]);
		pthread_mutex_unlock(&mutex);
		pthread_cond_signal(&cv);
	}
	pthread_exit(0);
}

char* convert(char* string)
{
	for(int i = 0; ; ++i)
	{
		string[i] = toupper(string[i]);
		if(string[i] == '\0')
			break;
	}
	return string;
}
