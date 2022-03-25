#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROCESS_MAX_NUM 131072

static int flag_show_pids = 0;
static int flag_numeric_sort = 0;
static int flag_version = 0;
static int arg_numeric_sort = 0;
static struct option long_options[] = {
    {"show-pids", no_argument, &flag_show_pids, 1},
    {"version", no_argument, &flag_version, 1},
    {"numeric-sort", no_argument, &flag_numeric_sort, 1},
    {0, 0, 0, 0}};
struct ProcessInfo {
  char comm[128];
  int child;
  int brother;
  char pid[32];
} process[PROCESS_MAX_NUM];

void read_process_info(char *pid_s) {
  char *buffer = malloc(strlen("/proc/") + strlen(pid_s) + strlen("/stat") + 1);
  strcpy(buffer, "/proc/");
  strcat(buffer, pid_s);
  strcat(buffer, "/stat");
  FILE *fp = fopen(buffer, "r");
  if (fp) {
    char state;
    int pid, ppid;
    char comm[128];
    if (fscanf(fp, "%d %s %c %d", &pid, comm, &state, &ppid) != EOF) {
      strcpy(process[pid].pid, pid_s);
      strcpy(process[pid].comm, comm);
      if (ppid >= 0 && ppid < PROCESS_MAX_NUM) {
        if (process[ppid].child == 0) {
          process[ppid].child = pid;
        } else {
          process[pid].brother = process[ppid].child;
          process[ppid].child = pid;
        }
      }
    }
    fclose(fp);
  }
  return;
}

void build_pstree() {
  struct dirent *de;
  DIR *dr = NULL;
  while (dr == NULL) {
    dr = opendir("/proc");
  }
  while ((de = readdir(dr)) != NULL) {
    if (isdigit(de->d_name[0])) {
      read_process_info(de->d_name);
    }
  }
  closedir(dr);
  return;
}

int print_pid(int pid) {
  if (flag_show_pids) {
    printf("%s(%s)", process[pid].comm, process[pid].pid);
    return strlen(process[pid].comm) + strlen(process[pid].pid) + 2;
  }
  printf("%s", process[pid].comm);
  return strlen(process[pid].comm);
}

int cmp(const void *a, const void *b) { return *(int *)a - *(int *)b; }

void sort_pstree(int pid) {
  if (process[pid].child == 0) {
    return;
  }
  int childs[128], child_num = 0;
  for (int sid = process[pid].child; sid != 0; sid = process[sid].brother) {
    childs[child_num++] = sid;
  }
  qsort(childs, child_num, sizeof(int), cmp);
  int sid;
  sid = process[pid].child = childs[0];
  for (int i = 1; i < child_num; i++) {
    process[sid].brother = childs[i];
    sid = childs[i];
  }
  process[sid].brother = 0;
  return;
}

void print_pstree(int pid, int depth, int depth_point[], int depth_point_len) {
  depth += print_pid(pid);
  if (process[pid].child == 0) {
    putchar('\n');
    return;
  }
  char *prefix = malloc(depth + 1);
  for (int i = 0; i < depth; i++) prefix[i] = ' ';
  prefix[depth] = 0;
  for (int sid = process[pid].child, flag = 1; sid != 0;
       sid = process[sid].brother) {
    if (flag) {
      if (process[sid].brother != 0) {
        printf("─┬─");
        depth_point[depth_point_len++] = depth + 1;
      } else {
        printf("───");
        if (depth_point[depth_point_len - 1] == depth + 1) depth_point_len--;
      }
      print_pstree(sid, depth + 3, depth_point, depth_point_len);
      flag = 0;
    } else {
      for (int i = 0, j = 0; i < depth; i++) {
        if (j < depth_point_len && depth_point[j] == i) {
          j++;
          printf("│");
        } else
          putchar(' ');
      }
      if (process[sid].brother != 0) {
        printf(" ├─");
        depth_point[depth_point_len++] = depth + 1;
      } else {
        printf(" └─");
        if (depth_point[depth_point_len - 1] == depth + 1) depth_point_len--;
      }
      print_pstree(sid, depth + 3, depth_point, depth_point_len);
    }
  }
  return;
}

int main(int argc, char *argv[]) {
  build_pstree();
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    // printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]);
  int c;
  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "pnV", long_options, &option_index);
    if (c == -1) break;
    switch (c) {
      case 'p':
        flag_show_pids = 1;
        break;
      case 'n':
        flag_numeric_sort = 1;
        break;
      case 'V':
        flag_version = 1;
        break;
      default:
        fprintf(stderr, "error args\n");
        return 0;
    }
  }
  if (flag_version) {
    fprintf(stderr, "pstree (PSmisc) 0.1\n");
    fprintf(stderr, "Copyright (C) 2022-2023 Xiang Li\n");
    return 0;
  }
  strcpy(process[0].comm, "(root)");
  strcpy(process[0].pid, "0");
  if (flag_numeric_sort) sort_pstree(0);
  int depth_point[256];
  print_pstree(0, 0, depth_point, 0);
  return 0;
}
