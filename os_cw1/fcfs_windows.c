/* ============================================================================
 *  FCFS Scheduling — Cross-Platform (Linux & Windows)
 *  Context : Online Examination System
 *  Module  : ST5068CEM | Platforms and Operating Systems
 *
 *  Compile & Run
 *  Linux  : gcc -o fcfs fcfs.c && ./fcfs
 *  Windows: gcc -o fcfs.exe fcfs.c && .\fcfs.exe
 * ============================================================================
 */

#include <stdio.h>
#include <time.h>

#ifdef _WIN32
  #include <windows.h>
  static double get_time_ms(void) {
      LARGE_INTEGER freq, count;
      QueryPerformanceFrequency(&freq);
      QueryPerformanceCounter(&count);
      return (double)count.QuadPart / freq.QuadPart * 1000.0;
  }
#else
  static double get_time_ms(void) {
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
  }
#endif

typedef struct {
    int id;
    char name[40];
    int arrival;
    int burst;
    int start;
    int finish;
    int waiting;
    int turnaround;
    int response;
} Task;

#define N 9

static void fcfs(Task t[], int n) {
    for (int i = 0; i < n - 1; i++)
        for (int j = i + 1; j < n; j++)
            if (t[j].arrival < t[i].arrival) {
                Task tmp = t[i]; t[i] = t[j]; t[j] = tmp;
            }
    int time = 0;
    for (int i = 0; i < n; i++) {
        if (time < t[i].arrival) time = t[i].arrival;
        t[i].start      = time;
        t[i].finish     = time + t[i].burst;
        t[i].waiting    = t[i].start - t[i].arrival;
        t[i].turnaround = t[i].finish - t[i].arrival;
        t[i].response   = t[i].start  - t[i].arrival;
        time = t[i].finish;
    }
}

static void print_table(Task t[], int n) {
    printf("\nProcess Details:\n");
    printf("%-4s| %-8s| %-6s| %-6s| %-7s| %-10s| %-12s| %-8s\n",
           "PID","Arrival","Burst","Start","Finish","Waiting","Turnaround","Response");
    printf("----|---------|-------|-------|--------|-----------|-------------|----------\n");
    for (int i = 0; i < n; i++)
        printf(" %-3d| %7d | %5d | %5d | %6d | %9d | %11d | %7d\n",
               t[i].id, t[i].arrival, t[i].burst, t[i].start,
               t[i].finish, t[i].waiting, t[i].turnaround, t[i].response);
}

static void print_metrics(Task t[], int n, double exec_ms) {
    float tw=0, tt=0, tr=0;
    int max_wt=0, min_wt=t[0].waiting, max_tat=0, min_tat=t[0].turnaround;
    int burst_sum=0, wcl=0, total_time=t[n-1].finish - t[0].start;
    for (int i = 0; i < n; i++) {
        tw += t[i].waiting; tt += t[i].turnaround; tr += t[i].response;
        burst_sum += t[i].burst;
        if (t[i].waiting    > max_wt)  max_wt  = t[i].waiting;
        if (t[i].waiting    < min_wt)  min_wt  = t[i].waiting;
        if (t[i].turnaround > max_tat) max_tat = t[i].turnaround;
        if (t[i].turnaround < min_tat) min_tat = t[i].turnaround;
        if (t[i].burst      > wcl)     wcl     = t[i].burst;
    }
    printf("\nPerformance Metrics:\n");
    printf("==========================================\n");
    printf("Average Waiting Time:     %.2f units\n",  tw/n);
    printf("Average Turnaround Time:  %.2f units\n",  tt/n);
    printf("Average Response Time:    %.2f units\n",  tr/n);
    printf("\n");
    printf("Maximum Waiting Time:     %d units\n",    max_wt);
    printf("Minimum Waiting Time:     %d units\n",    min_wt);
    printf("Maximum Turnaround Time:  %d units\n",    max_tat);
    printf("Minimum Turnaround Time:  %d units\n",    min_tat);
    printf("Throughput:               %.4f processes/unit time\n", (float)n/total_time);
    printf("CPU Utilization:          %.2f%%\n", (float)burst_sum/total_time*100.0f);
    printf("\n");
    printf("==========================================\n");
    printf("Real-time Execution Metrics:\n");
    printf("==========================================\n");
    printf("Program execution time:         %.4f milliseconds\n", exec_ms);
    printf("Scheduling latency (overhead):  <1 ms\n");
    printf("Average Process Latency:        %.2f units\n", tw/n);
    printf("Worst-case latency prediction:  %d units\n",   wcl);
    printf("\n");
}

int main(void) {
    printf("\n");
    printf("============================================================\n");
    printf("  FCFS Scheduling  |  Online Examination System\n");
    printf("  ST5068CEM  -  Platforms and Operating Systems\n");
    printf("============================================================\n");

    Task tasks[N] = {
        { 1, "Student Login / Auth",      3,  3 },
        { 2, "Fetch Exam Question",        1,  7 },
        { 3, "Generate Score Report",      9, 10 },
        { 4, "Start Exam Timer",           4,  3 },
        { 5, "Submit Answer (Student A)",  8,  2 },
        { 6, "Authentication Service",     1,  5 },
        { 7, "Proctoring Log Writer",      8,  3 },
        { 8, "Session Monitor",            6,  6 },
        { 9, "Answer Submission Handler",  4, 10 },
    };

    double t0 = get_time_ms();
    fcfs(tasks, N);
    double t1 = get_time_ms();

    print_table(tasks, N);
    print_metrics(tasks, N, t1 - t0);
    return 0;
}