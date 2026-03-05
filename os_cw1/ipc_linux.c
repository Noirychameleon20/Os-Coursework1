/* ============================================================================
 *  IPC — SHARED MEMORY | Online Examination System
 *  ST5068CEM | Platforms and Operating Systems
 *
 *  Compile & Run
 *  Linux  : gcc -o ipc_shm ipc_sharedmem.c && ./ipc_shm
 *  Windows: gcc -o ipc_shm.exe ipc_sharedmem.c && .\ipc_shm.exe
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #include <windows.h>
  static double get_time_s(void) {
      LARGE_INTEGER f, c;
      QueryPerformanceFrequency(&f); QueryPerformanceCounter(&c);
      return (double)c.QuadPart / f.QuadPart;
  }
#else
  #include <sys/ipc.h>
  #include <sys/shm.h>
  static double get_time_s(void) {
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      return ts.tv_sec + ts.tv_nsec / 1e9;
  }
#endif

#define N 9

typedef struct {
    int   id;
    int   priority;   /* 1=CRITICAL 2=HIGH 3=NORMAL 4=LOW 5=IDLE */
    float arrival;
    float burst;
    float start;
    float finish;
    float wt;
    float tat;
    int   done;
} Task;

typedef struct {
    Task tasks[N];
    int  context_switches;
    int  ready;
} SharedExam;

/* ── Priority scheduling ────────────────────────────────── */
static void schedule(SharedExam *shm)
{
    float time = 0.0f;
    int   done = 0;

    while (done < N) {
        int best = -1, bp = 99;
        for (int i = 0; i < N; i++)
            if (!shm->tasks[i].done && shm->tasks[i].arrival <= time
                && shm->tasks[i].priority < bp)
                { bp = shm->tasks[i].priority; best = i; }

        if (best == -1) { time += 0.5f; continue; }

        shm->tasks[best].start  = time;
        shm->tasks[best].finish = time + shm->tasks[best].burst;
        shm->tasks[best].wt     = shm->tasks[best].start  - shm->tasks[best].arrival;
        shm->tasks[best].tat    = shm->tasks[best].finish - shm->tasks[best].arrival;
        shm->tasks[best].done   = 1;
        time = shm->tasks[best].finish;
        shm->context_switches++;
        done++;
    }
}

/* ── Main ───────────────────────────────────────────────── */
int main(void)
{
#ifdef _WIN32
    SharedExam *shm = (SharedExam *)calloc(1, sizeof(SharedExam));
#else
    int shmid = shmget(0x2025, sizeof(SharedExam), IPC_CREAT | 0666);
    SharedExam *shm = (SharedExam *)shmat(shmid, NULL, 0);
#endif

    /* ── Exam tasks written into shared memory ── */
    Task init[N] = {
        {1,3,3.0f, 3.0f}, {2,3,1.0f, 7.0f}, {3,5,9.0f,10.0f},
        {4,1,4.0f, 3.0f}, {5,2,8.0f, 2.0f}, {6,2,1.0f, 5.0f},
        {7,4,8.0f, 3.0f}, {8,3,6.0f, 6.0f}, {9,2,4.0f,10.0f},
    };
    for (int i = 0; i < N; i++) { shm->tasks[i] = init[i]; shm->tasks[i].done = 0; }
    shm->context_switches = 0;

    double t0        = get_time_s();
    schedule(shm);
    double exec_time = get_time_s() - t0;
    double db_lat    = exec_time * 33.0;
    shm->ready       = 1;

    /* ── Table ── */
    printf("\nOnline Examination System - Priority Scheduling IPC System\n\n");
    printf("| %-4s| %-5s| %-5s| %-5s| %-7s| %-7s| %-7s| %-7s|\n",
           "ID","Prio","AT","BT","Start","Finish","WT","TAT");
    printf("|-----|------|------|------|--------|--------|--------|--------|\n");
    for (int i = 0; i < N; i++) {
        Task *t = &shm->tasks[i];
        printf("| %02d  | %-5d| %-5.1f| %-5.1f| %-7.2f| %-7.2f| %-7.2f| %-7.2f|\n",
               t->id, t->priority, t->arrival, t->burst,
               t->start, t->finish, t->wt, t->tat);
    }
    printf("|-----|------|------|------|--------|--------|--------|--------|\n");

    /* ── Metrics ── */
    float tw=0,tt=0,tb=0,mxw=0,mnw=999,mxt=0,mnt=999,lf=0;
    for (int i=0;i<N;i++){
        tw+=shm->tasks[i].wt; tt+=shm->tasks[i].tat; tb+=shm->tasks[i].burst;
        if(shm->tasks[i].wt >mxw)mxw=shm->tasks[i].wt;
        if(shm->tasks[i].wt <mnw)mnw=shm->tasks[i].wt;
        if(shm->tasks[i].tat>mxt)mxt=shm->tasks[i].tat;
        if(shm->tasks[i].tat<mnt)mnt=shm->tasks[i].tat;
        if(shm->tasks[i].finish>lf)lf=shm->tasks[i].finish;
    }

    printf("\nSystem Performance Metrics:\n");
    printf("%-28s: %.2f units\n",     "Average Waiting Time",    tw/N);
    printf("%-28s: %.2f units\n",     "Average Turnaround Time", tt/N);
    printf("%-28s: %.2f units\n",     "Average Response Time",   tw/N);
    printf("%-28s: %.2f units\n",     "Maximum Waiting Time",    mxw);
    printf("%-28s: %.2f units\n",     "Minimum Waiting Time",    mnw);
    printf("%-28s: %.2f units\n",     "Maximum Turnaround Time", mxt);
    printf("%-28s: %.2f units\n",     "Minimum Turnaround Time", mnt);
    printf("%-28s: %.4f tasks/unit\n","System Throughput",       (float)N/lf);
    printf("%-28s: %.0f%%\n",         "Server CPU Utilization",  tb/lf*100);

    printf("\nLatency Metrics (Shared Memory Log):\n");
    printf("%-28s: %f units\n", "DB Latency (Hardware Calc)", db_lat);
    printf("%-28s: %d\n",       "Context Switches",  shm->context_switches);
    printf("%-28s: %f units\n", "Total Latency Overhead", db_lat + shm->context_switches*0.5);

    printf("\nReal-Time IPC Metrics:\n");
    printf("%-28s: %f seconds\n","IPC Execution Time",   exec_time);
    printf("%-28s: %.2f units\n","Avg Processing Delay", tw/N);
    printf("%-28s: %.2f units\n","Total System Delay",   tw);
    printf("\n");

#ifdef _WIN32
    free(shm);
#else
    shmdt(shm); shmctl(shmid, IPC_RMID, NULL);
#endif
    return 0;
}