/*
 * Operating Systems   (2INCO)   Practical Assignment
 * Threaded Application
 *
 * Bogdan Floris (0935036)
 * Razvan Tomescu (0942059)
 *
 * Grading:
 * Students who hand in clean code that fully satisfies the minimum requirements will get an 8.
 * ”Extra” steps can lead to higher marks because we want students to take the initiative.
 * Extra steps can be, for example, in the form of measurements added to your code, a formal
 * analysis of deadlock freeness etc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>     // for usleep()
#include <time.h>       // for time()
#include <math.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include "prime.h"

typedef unsigned long long  MY_TYPE; //definition of MY_TYPE

MY_TYPE counter; //initialize thread counter
//bit definitions
#define BITMASK(n)          (((unsigned long long) 1) << (n))

// check if bit n in v is set
#define BIT_IS_SET(v,n)     (((v) & BITMASK(n)) == BITMASK(n))

// set bit n in v
#define BIT_SET(v,n)        ((v) =  (v) |  BITMASK(n))

// clear bit n in v
#define BIT_CLEAR(v,n)      ((v) =  (v) & ~BITMASK(n))

// declare a mutex, and it is initialized as well
static pthread_mutex_t      mutex1[NROF_SIEVE + 65]  ; //first mutez array
static pthread_mutex_t      mutex2          = PTHREAD_MUTEX_INITIALIZER; //mutex for condition
static pthread_cond_t       cond            = PTHREAD_COND_INITIALIZER;
static pthread_cond_t       cond2           = PTHREAD_COND_INITIALIZER;
static void rsleep (int t);

static void* my_threads(void * arg)
{
  MY_TYPE * argi;
  MY_TYPE   i;
  argi = (MY_TYPE *) arg;     // proper casting before dereferencing (could also be done in one statement)
  i = *argi;              // get the integer value of the pointer
  free (arg);

  MY_TYPE j;
  for(j = 2; j < NROF_SIEVE/i + 1; j++) // go through the sieve
  {
    // get the exact number
    MY_TYPE  z = i * j;
    MY_TYPE  x1 = z/64;
    MY_TYPE  y1 = z%64;
    pthread_mutex_lock (&mutex1[z]); //lock mutex for this buffer in which the number is in so the other buffers are accesible
    // if is already prime then brake. This is an optimization to make the algorithm faster, so it doesn't check items that where already crossed
    if(BIT_IS_SET(buffer[i/64],i%64) == false){

      pthread_mutex_unlock (&mutex1[z]); // unlock mutex for this buffer
      rsleep(100);
      break;

    }

    BIT_CLEAR(buffer[x1],y1); //cross the number
    pthread_mutex_unlock (&mutex1[z]); // unlock mutex for this buffer
    rsleep(100);
  }

  pthread_mutex_lock (&mutex2); //lock mutex to update counter
  counter--;

  if(counter == 0){
    pthread_cond_signal(&cond2); //signal that all threads are finished
  }

  pthread_mutex_unlock (&mutex2);
  pthread_cond_signal(&cond); // signal that threads can be created again

  pthread_exit(0); //exit thread

}

int main (void)
{
    // TODO: start threads generate all primes between 2 and NROF_SIEVE and output the results
    // (see thread_malloc_free_test() and thread_mutex_test() how to use threads and mutexes,
    //  see bit_test() how to manipulate bits in a large integer)
    MY_TYPE n = sqrt(NROF_SIEVE);
    MY_TYPE i;

    for(i = 0; i <= NROF_SIEVE +63; i++){
        pthread_mutex_init(&mutex1[i], NULL); //initialize all the mutexes for every buffer
    }
    //initialize numbers in buffers
    for(i = 0; i <= NROF_SIEVE/64; i++)
    {
      buffer[i] = ~0;
    }

    pthread_t   my_thread;

    for(i = 2; i <= n; i++)
    {
      MY_TYPE x = i/64;
      MY_TYPE y = i%64;

      if(BIT_IS_SET(buffer[x], y))
      {
        pthread_mutex_lock(&mutex2); // lock mutex to check counter
        while(counter >= NROF_THREADS)
        {

          pthread_cond_wait(&cond, &mutex2); //wait for a thread to exit

        }
        pthread_mutex_unlock(&mutex2); //unlock mutex

        MY_TYPE* parameter;
        parameter = malloc (sizeof (MY_TYPE));
        *parameter = i;
        pthread_mutex_lock (&mutex2); //lock mutex to update counter
        counter++;
        pthread_mutex_unlock (&mutex2); //unlock

        pthread_create (&my_thread, NULL, my_threads, parameter); //create thread
      }
    }
    //lock to check counter
    pthread_mutex_lock(&mutex2);
    while(counter != 0)
    {
      pthread_cond_wait(&cond2, &mutex2);//wait for all threads to finish
    }
    pthread_mutex_unlock(&mutex2);//unlock
    //print prime numbers
    for(i = 2; i <= NROF_SIEVE; i++)
    {
      MY_TYPE  x = i/64;
      MY_TYPE  y = i%64;
      if(BIT_IS_SET(buffer[x], y))
      {
        printf("%lld\n", i);
      }
    }


    return (0);
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void rsleep (int t)
{
    static bool first_call = true;

    if (first_call == true)
    {
        srandom (time (NULL) % getpid ());
        first_call = false;
    }
    usleep (random() % t);
}
