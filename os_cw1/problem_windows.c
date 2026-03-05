/* ============================================================================
 *  FILE 1 — THE PROBLEMS (No Synchronization)
 *  Online Examination System — ST5068CEM
 *
 *  Shows 3 classic problems WITHOUT any synchronization:
 *  1. Producer-Consumer  — students submit answers, race condition on buffer
 *  2. Reader-Writer      — examiner updates paper while students read
 *  3. Dining Philosophers— exam markers deadlock on shared resources
 *
 *  Compile & Run
 *  Linux  : gcc -o problem problem.c -lpthread && ./problem
 *  Windows: gcc -o problem.exe problem.c && .\problem.exe
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #include <windows.h>
  #include <process.h>
  #define THREAD_T          HANDLE
  #define sleep_ms(ms)      Sleep(ms)
  #define thread_create(t,fn,arg) (t)=(HANDLE)_beginthreadex(NULL,0,fn,arg,0,NULL)
  #define thread_join(t)    WaitForSingleObject(t,INFINITE)
  #define THREAD_RET        unsigned __stdcall
  static double get_time_s(void) {
      LARGE_INTEGER f,c; QueryPerformanceFrequency(&f); QueryPerformanceCounter(&c);
      return (double)c.QuadPart/f.QuadPart;
  }
#else
  #include <pthread.h>
  #include <unistd.h>
  #define THREAD_T          pthread_t
  #define sleep_ms(ms)      usleep((ms)*1000)
  #define thread_create(t,fn,arg) pthread_create(&(t),NULL,(void*(*)(void*))fn,arg)
  #define thread_join(t)    pthread_join(t,NULL)
  #define THREAD_RET        void*
  static double get_time_s(void) {
      struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts);
      return ts.tv_sec+ts.tv_nsec/1e9;
  }
#endif

/* ══════════════════════════════════════════════════════════
 *  PROBLEM 1 — PRODUCER-CONSUMER
 *  Race condition: students write to buffer without locking.
 *  count variable corrupted — answers can be lost or doubled.
 * ══════════════════════════════════════════════════════════ */
#define BUF_SIZE 5
#define NUM_ANS  9

static int buf[BUF_SIZE], buf_in=0, buf_out=0, buf_count=0;
static int p_submitted=0, p_graded=0;
static int race_events=0;

/* Metrics for table */
static float pc_wt[NUM_ANS], pc_tat[NUM_ANS];
static int   pc_ids[NUM_ANS], pc_idx=0;
static float pc_submit_time[NUM_ANS], pc_grade_time[NUM_ANS];
static float pc_start_time;

THREAD_RET producer_prob(void *arg)
{
    int id = *(int *)arg;
    for (int q=1; q<=3 && p_submitted<NUM_ANS; q++) {
        sleep_ms(60*id);
        float t = (float)(get_time_s()-pc_start_time)*1000;
        /* NO LOCK — race condition */
        if (buf_count < BUF_SIZE) {
            buf[buf_in] = id*10+q;
            buf_in = (buf_in+1)%BUF_SIZE;
            buf_count++;
            p_submitted++;
            pc_submit_time[pc_idx < NUM_ANS ? pc_idx : 0] = t;
        } else {
            race_events++;  /* buffer full — answer lost */
        }
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

THREAD_RET consumer_prob(void *arg)
{
    (void)arg;
    while (p_graded < NUM_ANS) {
        sleep_ms(100);
        float t = (float)(get_time_s()-pc_start_time)*1000;
        if (buf_count > 0) {
            int ans = buf[buf_out];
            buf_out = (buf_out+1)%BUF_SIZE;
            buf_count--;
            int i = p_graded < NUM_ANS ? p_graded : NUM_ANS-1;
            pc_ids[i]        = ans;
            pc_grade_time[i] = t;
            pc_wt[i]         = t - pc_submit_time[i];
            pc_tat[i]        = t;
            p_graded++;
        }
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static void run_pc_problem(void)
{
    printf("\n============================================================\n");
    printf("  PROBLEM 1: Producer-Consumer (No Synchronization)\n");
    printf("  ExamSystem — 3 Students submit answers, 1 Grading Server\n");
    printf("============================================================\n");
    printf("  ⚠ No mutex/semaphore — race condition on shared buffer\n\n");

    pc_start_time = (float)get_time_s();
    THREAD_T students[3], grader;
    int ids[3]={1,2,3};
    thread_create(grader,      consumer_prob, NULL);
    thread_create(students[0], producer_prob, &ids[0]);
    thread_create(students[1], producer_prob, &ids[1]);
    thread_create(students[2], producer_prob, &ids[2]);
    thread_join(students[0]); thread_join(students[1]);
    thread_join(students[2]); thread_join(grader);

    printf("| %-6s| %-10s| %-10s| %-10s| %-10s|\n",
           "AnsID","Submit(ms)","Grade(ms)","WT(ms)","TAT(ms)");
    printf("|-------|-----------|-----------|-----------|----------|\n");

    float tw=0, tt=0;
    for (int i=0; i<p_graded && i<NUM_ANS; i++) {
        printf("| %-6d| %-10.2f| %-10.2f| %-10.2f| %-10.2f|\n",
               pc_ids[i], pc_submit_time[i], pc_grade_time[i],
               pc_wt[i]>0?pc_wt[i]:0, pc_tat[i]);
        tw += pc_wt[i]>0?pc_wt[i]:0;
        tt += pc_tat[i];
    }
    printf("|-------|-----------|-----------|-----------|----------|\n");

    printf("\n  Race Condition Results:\n");
    printf("  %-28s: %d / %d\n",  "Answers graded",    p_graded, NUM_ANS);
    printf("  %-28s: %d\n",       "Race events (lost)", race_events);
    printf("  %-28s: %.2f ms\n",  "Avg Waiting Time",   p_graded>0?tw/p_graded:0);
    printf("  %-28s: %.2f ms\n",  "Avg Turnaround Time",p_graded>0?tt/p_graded:0);
    printf("\n  ⚠ count variable modified by 4 threads without any lock.\n");
    printf("  ⚠ Answers can be lost, duplicated, or buffer can corrupt.\n");
}


/* ══════════════════════════════════════════════════════════
 *  PROBLEM 2 — READER-WRITER
 *  No exclusive access — examiner writes while students read.
 *  Students may see half-updated, corrupted questions.
 * ══════════════════════════════════════════════════════════ */
static char rw_paper[3][80] = {
    "Q1: What is process scheduling?",
    "Q2: What is a semaphore?",
    "Q3: Describe deadlock conditions"
};
static int rw_version = 1;
static int rw_corrupt_reads = 0;
static int rw_reads_during_write = 0;
static volatile int rw_writing = 0;

THREAD_RET rw_reader_prob(void *arg)
{
    int id = *(int *)arg;
    sleep_ms(id * 40);
    if (rw_writing) {
        rw_corrupt_reads++;
        printf("  [RW ⚠]  Student %d reads DURING WRITE → corrupt data risk!\n", id);
    } else {
        printf("  [RW  ]  Student %d reads Q1: \"%.35s...\"\n", id, rw_paper[0]);
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

THREAD_RET rw_writer_prob(void *arg)
{
    (void)arg;
    sleep_ms(120);
    rw_writing = 1;
    printf("  [RW ⚠]  Examiner writing — NO LOCK — students may read now!\n");
    /* simulate slow write (dangerous window) */
    strncpy(rw_paper[0], "Q1: Explain semaphore vs mutex",
            sizeof(rw_paper[0])-1);
    sleep_ms(50);  /* gap where readers can see partial data */
    rw_version++;
    rw_writing = 0;
    printf("  [RW  ]  Examiner done — paper version %d\n", rw_version);
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static void run_rw_problem(void)
{
    printf("\n============================================================\n");
    printf("  PROBLEM 2: Reader-Writer (No Synchronization)\n");
    printf("  ExamSystem — 5 Students read, 1 Examiner updates paper\n");
    printf("============================================================\n");
    printf("  ⚠ No rw_mutex — examiner can write while students read\n\n");

    THREAD_T readers[5], writer;
    int r_ids[5]={1,2,3,4,5};
    thread_create(writer, rw_writer_prob, NULL);
    for (int i=0;i<5;i++) thread_create(readers[i], rw_reader_prob, &r_ids[i]);
    thread_join(writer);
    for (int i=0;i<5;i++) thread_join(readers[i]);

    printf("\n  | %-28s| %-6s|\n", "Metric", "Value");
    printf("  |------------------------------|-------|\n");
    printf("  | %-28s| %-6d|\n", "Total readers",           5);
    printf("  | %-28s| %-6d|\n", "Corrupt read events",     rw_corrupt_reads);
    printf("  | %-28s| %-6d|\n", "Paper version",           rw_version);
    printf("  | %-28s| %-6s|\n", "Write lock used",         "NO");
    printf("  |------------------------------|-------|\n");
    printf("\n  ⚠ Students and examiner accessed the paper simultaneously.\n");
    printf("  ⚠ In a real system: garbled questions, wrong marks shown.\n");
}


/* ══════════════════════════════════════════════════════════
 *  PROBLEM 3 — DINING PHILOSOPHERS (DEADLOCK)
 *  Each marker grabs left pen first → circular wait → deadlock.
 *  All 4 deadlock conditions are present.
 * ══════════════════════════════════════════════════════════ */
#define N_PHIL 5
static int pen[N_PHIL] = {1,1,1,1,1};  /* 1=available */
static int scripts_marked[N_PHIL] = {0};
static int deadlock_count = 0;

static const char *phil_names[N_PHIL] = {
    "Dr. Adams ", "Dr. Brown ", "Dr. Carter",
    "Dr. Diaz  ", "Dr. Evans "
};

THREAD_RET phil_prob(void *arg)
{
    int i    = *(int *)arg;
    int left = i, right = (i+1)%N_PHIL;

    sleep_ms(30);
    pen[left] = 0;  /* grab left pen */
    sleep_ms(80);   /* dangerous gap — all 5 hold their left pen here */

    if (pen[right]) {
        pen[right] = 0;
        printf("  [DP  ]  %s  marks a script ✔\n", phil_names[i]);
        sleep_ms(60);
        scripts_marked[i]++;
        pen[right] = 1;
        pen[left]  = 1;
    } else {
        deadlock_count++;
        printf("  [DP ⚠]  %s  STUCK — right pen[%d] taken → DEADLOCK\n",
               phil_names[i], right);
        pen[left] = 1;  /* forced drop — starvation */
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static void run_dp_problem(void)
{
    printf("\n============================================================\n");
    printf("  PROBLEM 3: Dining Philosophers (No Synchronization)\n");
    printf("  ExamSystem — 5 Markers, 5 Shared Pens\n");
    printf("============================================================\n");
    printf("  ⚠ Each marker grabs left pen first → circular wait risk\n\n");

    THREAD_T threads[N_PHIL];
    int ids[N_PHIL]={0,1,2,3,4};
    for (int i=0;i<N_PHIL;i++) thread_create(threads[i], phil_prob, &ids[i]);
    for (int i=0;i<N_PHIL;i++) thread_join(threads[i]);

    printf("\n  | %-28s| %-10s|\n", "Marker", "Scripts Marked");
    printf("  |-----------------------------|---------------|\n");
    for (int i=0;i<N_PHIL;i++)
        printf("  | %-28s| %-14d|\n", phil_names[i], scripts_marked[i]);
    printf("  |-----------------------------|---------------|\n");
    printf("\n  Deadlock events  : %d\n", deadlock_count);
    printf("  Conditions met   : Mutual Exclusion, Hold & Wait,\n");
    printf("                     No Preemption, Circular Wait\n");
    printf("  ⚠ Some markers could not mark — scripts ungraded.\n");
}


/* ── Main ───────────────────────────────────────────────── */
int main(void)
{
    printf("\n");
    printf("============================================================\n");
    printf("  FILE 1: Synchronization PROBLEMS\n");
    printf("  Online Examination System — ST5068CEM\n");
    printf("  Demonstrating: Race Conditions | Data Corruption | Deadlock\n");
    printf("============================================================\n");

    run_pc_problem();
    run_rw_problem();
    run_dp_problem();

    printf("\n============================================================\n");
    printf("  Summary\n");
    printf("  P1 Producer-Consumer : Race condition on answer buffer\n");
    printf("  P2 Reader-Writer     : Corrupt reads during exam update\n");
    printf("  P3 Dining Philosophers: Deadlock — markers cannot mark\n");
    printf("  → See solutions.c for Semaphore + Mutex fixes\n");
    printf("============================================================\n\n");
    return 0;
}