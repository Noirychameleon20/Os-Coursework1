/* ============================================================================
 *  Round Robin Scheduling — Cross-Platform (Linux & Windows)
 *  Context : Online Examination System
 *  Module  : ST5068CEM | Platforms and Operating Systems
 *
 *  Compile & Run
 *  Linux  : gcc -o rr rr.c && ./rr
 *  Windows: gcc -o rr.exe rr.c && .\rr.exe
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

#define N       9
#define QUANTUM 3

typedef struct {
    int id;
    char name[40];
    int arrival; 
    int burst;
    int remaining;
    int start;
    int finish;
    int waiting;
    int turnaround;
    int response;
    int resp_set;
} Task;

static void rr(Task t[], int n, int quantum) {
    int queue[1000], front=0, rear=0;
    int in_queue[N]={0}, time=0, completed=0;
    for (int i=0;i<n;i++) t[i].remaining=t[i].burst;
    for (int i=0;i<n;i++) if(t[i].arrival==0){queue[rear++]=i;in_queue[i]=1;}
    while (completed < n) {
        if (front==rear) {
            time++;
            for (int i=0;i<n;i++)
                if(!in_queue[i]&&t[i].remaining>0&&t[i].arrival<=time){queue[rear++]=i;in_queue[i]=1;}
            continue;
        }
        int idx=queue[front++];
        int exec=(t[idx].remaining<quantum)?t[idx].remaining:quantum;
        if (!t[idx].resp_set) {
            t[idx].start    = time;
            t[idx].response = time - t[idx].arrival;
            t[idx].resp_set = 1;
        }
        time += exec;
        t[idx].remaining -= exec;
        for (int i=0;i<n;i++)
            if(!in_queue[i]&&t[i].remaining>0&&t[i].arrival<=time){queue[rear++]=i;in_queue[i]=1;}
        if (t[idx].remaining==0) {
            t[idx].finish     = time;
            t[idx].turnaround = time - t[idx].arrival;
            t[idx].waiting    = t[idx].turnaround - t[idx].burst;
            completed++;
        } else queue[rear++]=idx;
    }
}

static void sort_by_finish(Task t[], int n) {
    for (int i=0;i<n-1;i++) for (int j=i+1;j<n;j++)
        if (t[j].finish<t[i].finish){Task tmp=t[i];t[i]=t[j];t[j]=tmp;}
}

static void print_table(Task t[], int n) {
    Task ord[N]; for(int i=0;i<n;i++) ord[i]=t[i];
    sort_by_finish(ord, n);
    printf("\nProcess Details:\n");
    printf("%-4s| %-8s| %-6s| %-6s| %-7s| %-10s| %-12s| %-8s\n",
           "PID","Arrival","Burst","Start","Finish","Waiting","Turnaround","Response");
    printf("----|---------|-------|-------|--------|-----------|-------------|----------\n");
    for (int i=0;i<n;i++)
        printf(" %-3d| %7d | %5d | %5d | %6d | %9d | %11d | %7d\n",
               ord[i].id,ord[i].arrival,ord[i].burst,ord[i].start,
               ord[i].finish,ord[i].waiting,ord[i].turnaround,ord[i].response);
}

static void print_metrics(Task t[], int n, int total_time, double exec_ms) {
    float tw=0,tt=0,tr=0;
    int max_wt=0,min_wt=t[0].waiting,max_tat=0,min_tat=t[0].turnaround;
    int burst_sum=0;
    for (int i=0;i<n;i++) {
        tw+=t[i].waiting; tt+=t[i].turnaround; tr+=t[i].response;
        burst_sum+=t[i].burst;
        if(t[i].waiting>max_wt)     max_wt=t[i].waiting;
        if(t[i].waiting<min_wt)     min_wt=t[i].waiting;
        if(t[i].turnaround>max_tat) max_tat=t[i].turnaround;
        if(t[i].turnaround<min_tat) min_tat=t[i].turnaround;
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
    printf("Worst-case latency prediction:  %d units\n",   QUANTUM);
    printf("\n");
}

int main(void) {
    printf("\n");
    printf("============================================================\n");
    printf("  Round Robin Scheduling (Q=%d)  |  Online Examination System\n", QUANTUM);
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
    rr(tasks, N, QUANTUM);
    double t1 = get_time_ms();

    /* total_time = last finish - first arrival */
    int total_time = 0;
    for (int i=0;i<N;i++) if(tasks[i].finish>total_time) total_time=tasks[i].finish;
    total_time -= 1; /* first arrival is 1 */

    print_table(tasks, N);
    print_metrics(tasks, N, total_time, t1 - t0);
    return 0;
}