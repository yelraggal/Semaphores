#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define bufferSize 5

int counter=0;

//semaphore for counter threads to writing messages 
sem_t sem;
//semaphore for mutual exclusion for critical section
sem_t sSem;
//semaphore for empty spaces available in buffer
sem_t eSem;
//semaphore for spaces taken from buffer 
sem_t nSem;

int queue_array[bufferSize];
int tail = -1;
int head = -1;


int enqueue(int element) {
	//check queue is full 
	if ((head == tail + 1) || (head == 0 && tail == bufferSize - 1)){
		return -1;
  	}
  	else {
		if (head == -1){ 
			head = 0;
		}
		tail = (tail + 1) % bufferSize;
		queue_array[tail] = element;
	}
	return tail;
}

int dequeue() {
	int element;
	//check queue is empty
	if (head == -1) {
    		return -1;
  	} else {
		element = queue_array[head];
		if (head == tail) {
			tail = -1;
			head = -1;
			return 0;
		} 
		else {
			head = (head + 1) % bufferSize;
			if(head == 0){
				return bufferSize-1;
			}
			return head-1;			
		}
	}
}


void * collectorFunction(void * args){
	int random;
	int index1;
	int semTryWait;
	while(1){
		random = (rand()%20)+10;
		//wait until buffer has available spaces
		semTryWait=sem_trywait(&nSem);
		if(semTryWait != 0){
			printf("Collector thread:nothing is in the buffer\n");
			sem_wait(&nSem);
		}
		sem_wait(&sSem);
		//dequeue from buffer
		index1 = dequeue();
		printf("Collector thread: reading from buffer at position %d\n",index1);
		sem_post(&sSem);
		//signal to monitor(producer) that value in buffer taken and can produce another one
		sem_post(&eSem);
		sleep(random);
	}
}

void * monitorFunction(void * args){
	int random;
	int temp;
	int index;
	int semTryWait1;
	while(1){
		random = (rand()%20)+10;
		printf("Monitor thread: waiting to read counter\n");
		//waiting to read value of counter and initialize it to 0
		sem_wait(&sem);
		printf("Monitor thread: reading a count value of %d\n",counter);
		temp = counter;
		counter = 0;
		sem_post(&sem);
		//wait until empty space available in buffer 
		semTryWait1=sem_trywait(&eSem);
		if(semTryWait1 != 0){
			printf("Monitor thread: Buffer full!!\n");
			sem_wait(&eSem);
		}
		sem_wait(&sSem);
		//enqueue counter value in buffer
		index = enqueue(temp);
		printf("Monitor thread: writing to buffer at position %d\n",index);
		sem_post(&sSem);
		//signal to collector(consumer) for value in buffer to consume 
		sem_post(&nSem);
		sleep(random);
	}
}
void * counterFunction(void * args){
	int random;
	while(1){
		random = (rand()%10)+1;
		printf("Counter thread %ld:received a message\n",pthread_self());
		printf("Counter thread %ld:waiting to write\n",pthread_self());
		sem_wait(&sem);
		counter++;
		printf("Counter thread %ld:now adding to counter, counter value %d \n",pthread_self(),counter);
		sem_post(&sem);
		sleep(random);
	}
}

int main(){
	int n,i;
	printf("Enter n number of threads counter:");
	scanf("%d",&n);
	//initialize threads for collector, monitor, and n for counter 
	pthread_t collector;
	pthread_t monitor;
	pthread_t mcounter[n];
	//semaphores initializations
	//2nd argument is 0 specifies that semaphore is shared between threads 
	//3rd argument specifies the initialized value for semaphore
	sem_init(&sem,0,1);
	sem_init(&eSem,0,bufferSize);
	sem_init(&sSem,0,1);
	sem_init(&nSem,0,0);
	pthread_create(&collector,NULL,&collectorFunction,NULL);
	pthread_create(&monitor,NULL,&monitorFunction,NULL);
	for(i=0;i<n;i++){
		pthread_create(&mcounter[i],NULL,&counterFunction,NULL);
	}
	pthread_join(collector,NULL);
	pthread_join(monitor,NULL);
	for(i=0;i<n;i++){
		pthread_join(mcounter[i],NULL);
	}
	return 0;
}
