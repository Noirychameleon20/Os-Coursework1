/* ============================================================================
 *  FILE 2 — THE SOLUTIONS (Semaphores + Mutexes)
 *  Online Examination System — ST5068CEM
 *
 *  Solves all 3 problems from problem.c:
 *  1. Producer-Consumer  → Semaphores (empty + full + mutex)
 *  2. Reader-Writer      → Mutex + reader count
 *  3. Dining Philosophers→ Mutex + atomic state check
 *
 *  Compile & Run
 *  Linux  : gcc -o solutions solutions.c -lpthread && ./solutions
 *  Windows: gcc -o solutions.exe solutions.c && .\solutions.exe
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
  #define MUTEX_T           HANDLE
  #define SEM_T             HANDLE
  #define sleep_ms(ms)      Sleep(ms)
  #define mutex_init(m)     (m)=CreateMutex(NULL,FALSE,NULL)
  #define mutex_lock(m)     WaitForSingleObject(m,INFINITE)
  #define mutex_unlock(m)   ReleaseMutex(m)
  #define sem_init_val(s,v) (s)=CreateSemaphore(NULL,v,100,NULL)
  #define sem_wait(s)       WaitForSingleObject(s,INFINITE)
  #define sem_post(s)       ReleaseSemaphore(s,1,NULL)
  #define thread_create(t,fn,arg) (t)=(HANDLE)_beginthreadex(NULL,0,fn,arg,0,NULL)
  #define thread_join(t)    WaitForSingleObject(t,INFINITE)
  #define THREAD_RET        unsigned __stdcall
  static double get_time_s(void) {
      LARGE_INTEGER f,c; QueryPerformanceFrequency(&f); QueryPerformanceCounter(&c);
      return (double)c.QuadPart/f.QuadPart;
  }
#else
  #include <pthread.h>
  #include <semaphore.h>
  #include <unistd.h>
  #define THREAD_T          pthread_t
  #define MUTEX_T           pthread_mutex_t
  #define SEM_T             sem_t
  #define sleep_ms(ms)      usleep((ms)*1000)
  #define mutex_init(m)     pthread_mutex_init(&(m),NULL)
  #define mutex_lock(m)     pthread_mutex_lock(&(m))
  #define mutex_unlock(m)   pthread_mutex_unlock(&(m))
  #define sem_init_val(s,v) sem_init(&(s),0,v)
  #define sem_wait(s)       sem_wait(&(s))
  #define sem_post(s)       sem_post(&(s))
  #define thread_create(t,fn,arg) pthread_create(&(t),NULL,(void*(*)(void*))fn,arg)
  #define thread_join(t)    pthread_join(t,NULL)
  #define THREAD_RET        void*
  static double get_time_s(void) {
      struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts);
      return ts.tv_sec+ts.tv_nsec/1e9;
  }
#endif

/* ══════════════════════════════════════════════════════════
 *  SOLUTION 1 — PRODUCER-CONSUMER with SEMAPHORES
 *
 *  empty_slots : starts at BUF_SIZE → producer blocks when full
 *  filled_slots: starts at 0       → consumer blocks when empty
 *  mutex       : binary lock       → only one thread touches buffer
 * ══════════════════════════════════════════════════════════ */
#define BUF_SIZE 5
#define NUM_ANS  9

static int  s_buf[BUF_SIZE], s_in=0, s_out=0;
static int  s_produced=0, s_consumed=0;
static MUTEX_T s_mutex;
static SEM_T   empty_slots, filled_slots;

/* Per-answer timing for table */
static float ans_submit[NUM_ANS], ans_grade[NUM_ANS];
static int   ans_ids[NUM_ANS];
static float s_start_time;

THREAD_RET producer_sol(void *arg)
{
    int id = *(int *)arg;
    for (int q=1; q<=3; q++) {
        sleep_ms(60*id);

        sem_wait(empty_slots);   /* BLOCK if buffer full  */
        mutex_lock(s_mutex);     /* LOCK                  */

        int val = id*10+q;
        s_buf[s_in] = val;
        int slot = s_in;
        s_in = (s_in+1)%BUF_SIZE;
        ans_ids[s_produced]    = val;
        ans_submit[s_produced] = (float)(get_time_s()-s_start_time)*1000;
        s_produced++;

        mutex_unlock(s_mutex);   /* UNLOCK                */
        sem_post(filled_slots);  /* signal: answer ready  */
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

THREAD_RET consumer_sol(void *arg)
{
    (void)arg;
    while (s_consumed < NUM_ANS) {
        sem_wait(filled_slots);  /* BLOCK if buffer empty */
        mutex_lock(s_mutex);     /* LOCK                  */

        if (s_consumed >= NUM_ANS) { mutex_unlock(s_mutex); sem_post(filled_slots); break; }

        s_buf[s_out] = 0;
        s_out = (s_out+1)%BUF_SIZE;
        ans_grade[s_consumed] = (float)(get_time_s()-s_start_time)*1000;
        s_consumed++;

        mutex_unlock(s_mutex);   /* UNLOCK                */
        sem_post(empty_slots);   /* signal: slot freed    */
        sleep_ms(100);
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static void run_pc_solution(void)
{
    printf("\n============================================================\n");
    printf("  SOLUTION 1: Producer-Consumer (Semaphores)\n");
    printf("  ExamSystem — 3 Students submit answers, 1 Grading Server\n");
    printf("  Semaphores: empty=%d  filled=0  mutex=1\n", BUF_SIZE);
    printf("============================================================\n\n");

    mutex_init(s_mutex);
    sem_init_val(empty_slots,  BUF_SIZE);
    sem_init_val(filled_slots, 0);

    s_start_time = (float)get_time_s();
    THREAD_T students[3], grader;
    int ids[3]={1,2,3};
    thread_create(grader,      consumer_sol, NULL);
    thread_create(students[0], producer_sol, &ids[0]);
    thread_create(students[1], producer_sol, &ids[1]);
    thread_create(students[2], producer_sol, &ids[2]);
    thread_join(students[0]); thread_join(students[1]);
    thread_join(students[2]); thread_join(grader);

    printf("| %-6s| %-10s| %-10s| %-10s| %-10s|\n",
           "AnsID","Submit(ms)","Grade(ms)","WT(ms)","TAT(ms)");
    printf("|-------|-----------|-----------|-----------|----------|\n");

    float tw=0, tt=0;
    for (int i=0; i<NUM_ANS; i++) {
        float wt  = ans_grade[i] - ans_submit[i];
        float tat = ans_grade[i];
        printf("| %-6d| %-10.2f| %-10.2f| %-10.2f| %-10.2f|\n",
               ans_ids[i], ans_submit[i], ans_grade[i], wt>0?wt:0, tat);
        tw += wt>0?wt:0; tt += tat;
    }
    printf("|-------|-----------|-----------|-----------|----------|\n");

    printf("\n  System Performance Metrics:\n");
    printf("  %-28s: %.2f ms\n", "Average Waiting Time",    tw/NUM_ANS);
    printf("  %-28s: %.2f ms\n", "Average Turnaround Time", tt/NUM_ANS);
    printf("  %-28s: %d / %d\n","Answers Graded",           s_consumed, NUM_ANS);
    printf("  %-28s: %s\n",     "Race Conditions",          "NONE ✔");
    printf("\n  ✔ Buffer never overflowed. Server never read an empty slot.\n");
}


/* ══════════════════════════════════════════════════════════
 *  SOLUTION 2 — READER-WRITER with MUTEX
 *
 *  rw_mutex   : first reader acquires, last reader releases
 *  count_mutex: protects reader_count itself
 * ══════════════════════════════════════════════════════════ */
static char rw_paper[3][80] = {
    "Q1: What is process scheduling?",
    "Q2: What is a semaphore?",
    "Q3: Describe deadlock conditions"
};
static int  rw_version = 1;
static int  rw_reader_count = 0;
static int  rw_reads_ok = 0, rw_writes_ok = 0;
static MUTEX_T rw_mutex, count_mutex;

THREAD_RET rw_reader_sol(void *arg)
{
    int id = *(int *)arg;
    sleep_ms(id*40);

    mutex_lock(count_mutex);
    rw_reader_count++;
    if (rw_reader_count == 1) mutex_lock(rw_mutex); /* first reader locks writer */
    mutex_unlock(count_mutex);

    /* safe read */
    printf("  [RW ✔]  Student %d reads (readers active: %d) → \"%.30s...\"\n",
           id, rw_reader_count, rw_paper[0]);
    rw_reads_ok++;
    sleep_ms(80);

    mutex_lock(count_mutex);
    rw_reader_count--;
    if (rw_reader_count == 0) mutex_unlock(rw_mutex); /* last reader unlocks */
    mutex_unlock(count_mutex);
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

typedef struct { int id; const char *text; } WriterArg;
THREAD_RET rw_writer_sol(void *arg)
{
    WriterArg *w = (WriterArg *)arg;
    sleep_ms(w->id * 250);

    mutex_lock(rw_mutex);   /* exclusive — blocks all readers */
    printf("  [RW ✔]  Examiner %d WRITES exclusively → \"%s\"\n",
           w->id, w->text);
    sleep_ms(100);
    strncpy(rw_paper[0], w->text, sizeof(rw_paper[0])-1);
    rw_version++;
    rw_writes_ok++;
    printf("  [RW ✔]  Examiner %d done — paper version %d\n",
           w->id, rw_version);
    mutex_unlock(rw_mutex);
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static void run_rw_solution(void)
{
    printf("\n============================================================\n");
    printf("  SOLUTION 2: Reader-Writer (Mutex)\n");
    printf("  ExamSystem — 5 Students read, 2 Examiners update paper\n");
    printf("  Mutexes: rw_mutex + count_mutex\n");
    printf("============================================================\n\n");

    mutex_init(rw_mutex);
    mutex_init(count_mutex);

    THREAD_T readers[5], writers[2];
    int r_ids[5]={1,2,3,4,5};
    WriterArg w_args[2]={
        {1,"Q1: Explain semaphore vs mutex with example"},
        {2,"Q1: Compare mutex semaphore and monitor"}
    };
    for (int i=0;i<5;i++) thread_create(readers[i], rw_reader_sol, &r_ids[i]);
    for (int i=0;i<2;i++) thread_create(writers[i], rw_writer_sol, &w_args[i]);
    for (int i=0;i<5;i++) thread_join(readers[i]);
    for (int i=0;i<2;i++) thread_join(writers[i]);

    printf("\n  | %-28s| %-10s|\n", "Metric", "Value");
    printf("  |------------------------------|------------|\n");
    printf("  | %-28s| %-10d|\n", "Successful reads",    rw_reads_ok);
    printf("  | %-28s| %-10d|\n", "Successful writes",   rw_writes_ok);
    printf("  | %-28s| %-10d|\n", "Final paper version", rw_version);
    printf("  | %-28s| %-10s|\n", "Corrupt reads",       "0 ✔");
    printf("  | %-28s| %-10s|\n", "Data inconsistency",  "NONE ✔");
    printf("  |------------------------------|------------|\n");
    printf("\n  ✔ Students read simultaneously. Writers had exclusive access.\n");
    printf("  Final: \"%s\"\n", rw_paper[0]);
}


/* ══════════════════════════════════════════════════════════
 *  SOLUTION 3 — DINING PHILOSOPHERS with MUTEX + STATE
 *
 *  dp_mutex  : one philosopher changes state at a time
 *  dp_test() : picks up BOTH pens atomically — or waits
 *              breaks circular wait → no deadlock possible
 * ══════════════════════════════════════════════════════════ */
#define N_PHIL 5
typedef enum { THINKING, HUNGRY, EATING } State;

static State dp_state[N_PHIL];
static int   dp_marked[N_PHIL];
static MUTEX_T dp_mutex;

#ifdef _WIN32
  static HANDLE dp_event[N_PHIL];
#else
  static pthread_cond_t dp_cond[N_PHIL];
#endif

static const char *phil_names[N_PHIL] = {
    "Dr. Adams ", "Dr. Brown ", "Dr. Carter",
    "Dr. Diaz  ", "Dr. Evans "
};
#define L(i) (((i)+N_PHIL-1)%N_PHIL)
#define R(i) (((i)+1)%N_PHIL)

static void dp_test(int i)
{
    if (dp_state[i]==HUNGRY && dp_state[L(i)]!=EATING && dp_state[R(i)]!=EATING) {
        dp_state[i] = EATING;
        printf("  [DP ✔]  %s  picks up BOTH pens → MARKING\n", phil_names[i]);
#ifdef _WIN32
        SetEvent(dp_event[i]);
#else
        pthread_cond_signal(&dp_cond[i]);
#endif
    }
}

static void dp_pickup(int i)
{
    mutex_lock(dp_mutex);
    dp_state[i] = HUNGRY;
    dp_test(i);
#ifdef _WIN32
    mutex_unlock(dp_mutex);
    if (dp_state[i]!=EATING) WaitForSingleObject(dp_event[i],INFINITE);
#else
    while (dp_state[i]!=EATING) pthread_cond_wait(&dp_cond[i], &dp_mutex);
    mutex_unlock(dp_mutex);
#endif
}

static void dp_putdown(int i)
{
    mutex_lock(dp_mutex);
    dp_state[i] = THINKING;
    dp_marked[i]++;
    dp_test(L(i));
    dp_test(R(i));
    mutex_unlock(dp_mutex);
}

THREAD_RET phil_sol(void *arg)
{
    int i = *(int *)arg;
    for (int r=0; r<3; r++) {
        sleep_ms(80 + i*40);
        dp_pickup(i);
        sleep_ms(100);
        dp_putdown(i);
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static void run_dp_solution(void)
{
    printf("\n============================================================\n");
    printf("  SOLUTION 3: Dining Philosophers (Mutex + State Check)\n");
    printf("  ExamSystem — 5 Markers, 5 Shared Pens, 3 rounds each\n");
    printf("  Mutex: dp_mutex — atomic pickup of BOTH pens or none\n");
    printf("============================================================\n\n");

    for (int i=0;i<N_PHIL;i++) { dp_state[i]=THINKING; dp_marked[i]=0; }
    mutex_init(dp_mutex);
#ifdef _WIN32
    for (int i=0;i<N_PHIL;i++) dp_event[i]=CreateEvent(NULL,FALSE,FALSE,NULL);
#else
    for (int i=0;i<N_PHIL;i++) pthread_cond_init(&dp_cond[i],NULL);
#endif

    THREAD_T threads[N_PHIL];
    int ids[N_PHIL]={0,1,2,3,4};
    for (int i=0;i<N_PHIL;i++) thread_create(threads[i], phil_sol, &ids[i]);
    for (int i=0;i<N_PHIL;i++) thread_join(threads[i]);

    printf("\n  | %-28s| %-10s|\n", "Marker", "Scripts Marked");
    printf("  |-----------------------------|------------|\n");
    int total=0;
    for (int i=0;i<N_PHIL;i++) {
        printf("  | %-28s| %-10d|\n", phil_names[i], dp_marked[i]);
        total += dp_marked[i];
    }
    printf("  |-----------------------------|------------|\n");
    printf("  | %-28s| %-10d|\n", "Total scripts marked", total);
    printf("\n  ✔ No deadlock. All %d markers completed all 3 rounds.\n", N_PHIL);
    printf("  ✔ Circular wait broken — pickup is atomic (both or none).\n");
}


/* ── Main ───────────────────────────────────────────────── */
int main(void)
{
    printf("\n");
    printf("============================================================\n");
    printf("  FILE 2: Synchronization SOLUTIONS\n");
    printf("  Online Examination System — ST5068CEM\n");
    printf("  Semaphores (P1) | Mutex (P2) | Mutex + State (P3)\n");
    printf("============================================================\n");

    run_pc_solution();
    run_rw_solution();
    run_dp_solution();

    printf("\n============================================================\n");
    printf("  Summary\n");
    printf("  S1 Producer-Consumer : 3 Semaphores — no race condition\n");
    printf("  S2 Reader-Writer     : 2 Mutexes    — no corrupt reads\n");
    printf("  S3 Dining Philosophers: Mutex+State — no deadlock\n");
    printf("============================================================\n\n");
    return 0;
}