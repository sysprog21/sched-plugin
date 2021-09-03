#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void test_pr(void)
{
    FILE *fp = fopen("/proc/process_sched_add", "w");
    fprintf(fp, "%d", getpid());
    fclose(fp);

    while (1) {
        printf("PID: %d\n", getpid());
        sleep(1);
    }
}

int main()
{
    test_pr();
    return 0;
}
