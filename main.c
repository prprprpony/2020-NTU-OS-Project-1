#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <time.h>
#include <assert.h>
#include <sched.h>
#define SYS_PRINTK 335
#define SYS_GETNS 336
#define RR_UNIT 500
typedef enum { FIFO, RR, SJF, PSJF } Policy;

void run_unit(int n)
{
    for (int j = 0; j < n; ++j) {
        volatile unsigned long i;
        for (i = 0; i < 1000000UL; i++); 
    }
}
typedef struct
{
    char name[32];
    int R;
    int T;
    int in; // input order
    pid_t pid;
    struct timespec st;
} proc;
int cmp(const void * p1, const void * p2)
{
   const proc * a = p1;
   const proc * b = p2;
   if (a->R < b->R)
       return -1;
   if (a->R > b->R)
       return 1;
   return a->in < b->in ? -1 : 1;
}
int sprintf_timespec(char * s, struct timespec ts)
{
    return sprintf(s, "%lld.%09ld", (long long)ts.tv_sec, ts.tv_nsec);
}
struct sched_param param;
int maxpri, minpri;
void setpri(pid_t pid, int pri)
{
    param.sched_priority = pri;
    if (pri == 0) // block
        sched_setscheduler(pid, SCHED_IDLE, &param);
    else
        sched_setscheduler(pid, SCHED_FIFO, &param);
}
proc * a;
int N;
proc * b;
int len, cnt;
void new_proc(proc * ai)
{
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork failed\n");
        exit(-1);
    }
    if (pid == 0) {
        char strT[10];
        sprintf(strT, "%d", ai->T);
        execlp("./main", "main", ai->name, strT, NULL);
    }
    //getnstimeofday(&st);
    printf("%s %d\n", ai->name, pid);
    assert(syscall(SYS_GETNS, &ai->st) == 0);
    ai->pid = pid;
    setpri(pid, minpri);
    b[len++] = *ai;
}
void rot1(proc * arr, int n)
{
    if (n <= 0)
        return;
    proc tmp = arr[0];
    for (int i = 1; i < n; ++i)
        arr[i - 1] = arr[i];
    arr[n - 1] = tmp;
}
void update_queue(Policy policy) // move next proc to execute to b[0]
{
    if (len && b[0].T <= 0) {
        cnt = 0;
        waitpid(b[0].pid, NULL, 0);

        struct timespec st = b[0].st, ft;
        //getnstimeofday(&ft);
        assert(syscall(SYS_GETNS, &ft) == 0);
        char msg[256];
        char * s = msg;
        s += sprintf(s, "[Project1] %d", b[0].pid);
        *s++ = ' ';
        s += sprintf_timespec(s, st);
        *s++ = ' ';
        s += sprintf_timespec(s, ft);
        // fprintf(stderr, "%s\n", msg);
        assert(syscall(SYS_PRINTK, msg, (int)strlen(msg) + 1) == 0);
        double delta = (ft.tv_sec - st.tv_sec) + (ft.tv_nsec - st.tv_nsec) / 1e9;
        fprintf(stderr, "[DEBUG] name = %s, delta = %f\n", b[0].name, delta);

        rot1(b, len);
        --len;
    }
    if (len <= 0)
        return;
    pid_t old = b[0].pid;
    if (policy == FIFO) {
    } else if (policy == RR && cnt == RR_UNIT) {
        rot1(b, len); 
        cnt = 0;
    } else if (policy == PSJF || (cnt == 0 && policy == SJF)) {
        for (int i = 1; i < len; ++i)
            if (b[i].T < b[0].T) {
                proc tmp = b[0];
                b[0] = b[i];
                b[i] = tmp;
            }
    }
    if (b[0].pid != old) {
        setpri(old, minpri);
        setpri(b[0].pid, maxpri);
    }
}
void mainloop(Policy policy)
{
    int i = 0;
    for (int t = 0; i < N || len; t++) {
        while (i < N && a[i].R == t)
            new_proc(&a[i++]);
        update_queue(policy);
        if (len) {
            ++cnt;
            --b[0].T;
        }
        run_unit(1);
        update_queue(policy);
    }
}
int main(int argc, char * argv[])
{
    // set cpu to zero
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    assert(sched_setaffinity(getpid(), sizeof(mask), &mask) == 0);

    if (argc == 3) { // child process
        const char * name = argv[1];
        int T;
        sscanf(argv[2], "%d", &T);
        run_unit(T);
        return 0;
    }
    maxpri = sched_get_priority_max(SCHED_FIFO); // 99
    //minpri = sched_get_priority_min(SCHED_FIFO); // 1
    minpri = 0;
    fprintf(stderr, "[DEBUG] minpri = %d\n", minpri); 
    fprintf(stderr, "[DEBUG] maxpri = %d\n", maxpri); 
    setpri(getpid(), maxpri);
    char S[10];
    scanf("%s%d", S, &N);
    fprintf(stderr, "[DEBUG] %s %d\n", S, N); 
    a = malloc(sizeof(proc) * N);
    memset(a, 0, sizeof(proc) * N);
    b = malloc(sizeof(proc) * N);
    len = 0;
    for (int i = 0; i < N; ++i) {
        scanf("%s%d%d", a[i].name, &a[i].R, &a[i].T);
        a[i].in = i;
    }
    qsort(a, N, sizeof(proc), cmp);
    for (int i = 0; i < N; ++i)
        fprintf(stderr, "[DEBUG] %s %d %d\n", a[i].name, a[i].R, a[i].T);
    if (!strcmp(S, "FIFO"))
        mainloop(FIFO);
    if (!strcmp(S, "RR"))
        mainloop(RR);
    if (!strcmp(S, "SJF"))
        mainloop(SJF);
    if (!strcmp(S, "PSJF"))
        mainloop(PSJF);
    free(a);
    free(b);
}
