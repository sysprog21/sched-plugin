#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#define N_THREADS 2

void *test_pthread(void *ptr)
{
#ifdef SYS_gettid
    pid_t tid = syscall(SYS_gettid);
#else
#error "SYS_gettid unavailable on this system"
#endif
    FILE *fp = fopen("/proc/process_sched_add", "w");
    fprintf(fp, "%d", tid);
    fclose(fp);

    while (1) {
        printf("TID: %d\n", tid);
        sleep(1);
    }
    pthread_exit(NULL);
}

int main()
{
    pthread_t threads[N_THREADS];
    for (long t = 0; t < N_THREADS; t++) {
        printf("In main: creating thread %ld\n", t);
        int rc = pthread_create(&threads[t], NULL, test_pthread, NULL);
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    /* Last thing that main() should do */
    pthread_exit(NULL);
    return 0;
}
