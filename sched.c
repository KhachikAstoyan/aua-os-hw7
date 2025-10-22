#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int pid, arrival_time, burst_time;
  int start_time, completion_time;
  int turnaround_time, waiting_time, response_time;
  int done; // for SJF
} Process;

typedef struct {
  char label[16];
  int start, end;
} Segment;

int cmp_arrival_pid(const void *a, const void *b) {
  const Process *x = a, *y = b;
  if (x->arrival_time != y->arrival_time)
    return x->arrival_time - y->arrival_time;
  return x->pid - y->pid;
}

int cmp_pid(const void *a, const void *b) {
  const Process *x = a, *y = b;
  return x->pid - y->pid;
}

void finalize(Process *p) {
  p->turnaround_time = p->completion_time - p->arrival_time;
  p->waiting_time = p->start_time - p->arrival_time;
  p->response_time = p->waiting_time; // non-preemptive
}

void print_gantt(const char *title, Segment *seg, int m) {
  printf("=== %s ===\n", title);
  printf("Gantt Chart: ");
  for (int i = 0; i < m; i++)
    printf("| %s ", seg[i].label);
  printf("|\n");

  printf("Timeline  : ");
  if (m > 0) {
    printf("%d", seg[0].start);
    for (int i = 0; i < m; i++)
      printf(" --- %d", seg[i].end);
  }
  printf("\n");
}

void print_table_and_avgs(Process *p, int n) {
  qsort(p, n, sizeof(Process), cmp_pid); // display by PID

  double sw = 0, st = 0, sr = 0;
  printf("PID     AT     BT     WT     TAT    RT\n");
  for (int i = 0; i < n; i++) {
    printf("%-7d %-6d %-6d %-6d %-6d %-6d\n", p[i].pid, p[i].arrival_time,
           p[i].burst_time, p[i].waiting_time, p[i].turnaround_time,
           p[i].response_time);
    sw += p[i].waiting_time;
    st += p[i].turnaround_time;
    sr += p[i].response_time;
  }
  printf("\nAverage Waiting Time: %.2f\n", sw / n);
  printf("Average Turnaround Time: %.2f\n", st / n);
  printf("Average Response Time: %.2f\n\n", sr / n);
}

void simulate_fcfs(const Process *in, int n) {
  Process *p = malloc(sizeof(Process) * n);
  Segment *seg = malloc(sizeof(Segment) * (3 * n));
  if (!p || !seg) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
  }

  memcpy(p, in, sizeof(Process) * n);
  for (int i = 0; i < n; i++)
    p[i].done = 0;

  qsort(p, n, sizeof(Process), cmp_arrival_pid);

  int time = 0, m = 0;
  for (int i = 0; i < n; i++) {
    if (time < p[i].arrival_time) { // IDLE gap
      strcpy(seg[m].label, "IDLE");
      seg[m].start = time;
      seg[m].end = p[i].arrival_time;
      m++;
      time = p[i].arrival_time;
    }
    p[i].start_time = time;
    time += p[i].burst_time;
    p[i].completion_time = time;
    finalize(&p[i]);

    snprintf(seg[m].label, sizeof(seg[m].label), "P%d", p[i].pid);
    seg[m].start = p[i].start_time;
    seg[m].end = p[i].completion_time;
    m++;
  }

  print_gantt("First Come First Served (FCFS)", seg, m);
  print_table_and_avgs(p, n);

  free(seg);
  free(p);
}

int pick_sjf(Process *p, int n, int t) {
  int idx = -1;
  for (int i = 0; i < n; i++) {
    if (p[i].done || p[i].arrival_time > t)
      continue;
    if (idx == -1)
      idx = i;
    else if (p[i].burst_time < p[idx].burst_time)
      idx = i;
    else if (p[i].burst_time == p[idx].burst_time &&
             p[i].arrival_time < p[idx].arrival_time)
      idx = i;
  }
  return idx;
}

int earliest_arrival_not_done(Process *p, int n) {
  int t = INT_MAX;
  for (int i = 0; i < n; i++)
    if (!p[i].done && p[i].arrival_time < t)
      t = p[i].arrival_time;
  return t;
}

void simulate_sjf(const Process *in, int n) {
  Process *p = malloc(sizeof(Process) * n);
  Segment *seg = malloc(sizeof(Segment) * (3 * n));
  if (!p || !seg) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
  }

  memcpy(p, in, sizeof(Process) * n);
  for (int i = 0; i < n; i++)
    p[i].done = 0;

  int time = p[0].arrival_time;
  for (int i = 1; i < n; i++)
    if (p[i].arrival_time < time)
      time = p[i].arrival_time;

  int finished = 0, m = 0;
  while (finished < n) {
    int idx = pick_sjf(p, n, time);
    if (idx == -1) {
      int next_t = earliest_arrival_not_done(p, n);
      if (next_t == INT_MAX)
        break;
      strcpy(seg[m].label, "IDLE");
      seg[m].start = time;
      seg[m].end = next_t;
      m++;
      time = next_t;
      continue;
    }

    p[idx].start_time = time;
    time += p[idx].burst_time;
    p[idx].completion_time = time;
    p[idx].done = 1;
    finalize(&p[idx]);
    finished++;

    snprintf(seg[m].label, sizeof(seg[m].label), "P%d", p[idx].pid);
    seg[m].start = p[idx].start_time;
    seg[m].end = p[idx].completion_time;
    m++;
  }

  print_gantt("Shortest Job First (SJF)", seg, m);
  print_table_and_avgs(p, n);

  free(seg);
  free(p);
}

int main(void) {
  int n;
  printf("Enter the number of processes: ");
  if (scanf("%d", &n) != 1 || n <= 0) {
    fprintf(stderr, "Invalid number of processes.\n");
    return 1;
  }

  Process *procs = malloc(sizeof(Process) * n);
  if (!procs) {
    fprintf(stderr, "Memory allocation failed.\n");
    return 1;
  }

  for (int i = 0; i < n; i++) {
    procs[i].pid = i + 1;
    printf("Enter the arrival time and burst time for process %d: ", i + 1);
    if (scanf("%d %d", &procs[i].arrival_time, &procs[i].burst_time) != 2 ||
        procs[i].arrival_time < 0 || procs[i].burst_time <= 0) {
      fprintf(stderr, "Invalid input for process %d.\n", i + 1);
      free(procs);
      return 1;
    }
  }
  puts("");

  simulate_fcfs(procs, n);
  simulate_sjf(procs, n);

  free(procs);
  return 0;
}
