#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <string.h>


// 构建cache行结构
typedef struct cache_line {
  int valid;
  int tag;
  /** 本实验未设置块偏移b(需注意此处的b为cacheline中的必要组成部分)，
      因为要求仅需模拟是否命中，并不需要读取数据，所以此处不设置，
      此处取而代之的是time_tamp*/
  int time_tamp;    // LRU
} Cache_line;


// 构建cache组结构
typedef struct cache_set {
  int S;
  int E;
  int B;
  Cache_line **line;
} Cache;


// 缓存命中、不命中及冲突不命中
int hit_count = 0, miss_count = 0, eviction_count = 0;
int verbose = 0;
char t[1000];
// 初始化cache
Cache *cache = NULL;

void initCache(int s, int E, int b) {
  int S = 1 << s;
  int B = 1 << b;
  // 为整个cache分配空间
  cache = (Cache *)malloc(sizeof(Cache));
  cache->S = S;
  cache->E = E;
  cache->B = B;
  // 为S组分配空间
  cache->line = (Cache_line **)malloc(sizeof(Cache_line *) * S);
  for (int i = 0; i < S; i++) {
    // 为每组中的E行分配空间
    cache->line[i] = (Cache_line *)malloc(sizeof(Cache_line) * E);
    for (int j = 0; j < E; j++) {
      // 初始化
      cache->line[i][j].valid = 0;
      cache->line[i][j].tag = -1;
      cache->line[i][j].time_tamp = 0;
    }
  }
}


void free_Cache() {
  int S = cache->S;
  for (int i = 0; i < S; i++) {
    free(cache->line[i]);
  }
  free(cache->line);
  free(cache);
}

// 如果对应的组索引ops中有符合要求的，就返回行索引i，否则返回-1
int get_index(int op_s, int op_tag) {
  for (int i = 0; i < cache->E; i++) {
    if (cache->line[op_s][i].valid &&
	cache->line[op_s][i].tag == op_tag) {
      return i;
    }
  }
  return -1;
}

// LRU替换策略
int find_LRU(int op_s) {
  int max_index = 0, max_stamp = 0;
  for (int i = 0; i < cache->E; i++) {
    if (cache->line[op_s][i].time_tamp > max_stamp) {
      max_stamp = cache->line[op_s][i].time_tamp;
      max_index = i;
    }
  }
  return max_index;
}

// ops组中的E行是否都有效
int is_full(int op_s) {
  for (int i = 0; i < cache->E; i++) {
    if (cache->line[op_s][i].valid == 0)
      return i;
  }
  return -1;
}

// 增加time-tamp
void update(int i, int op_s, int op_tag) {
  cache->line[op_s][i].valid = 1;
  cache->line[op_s][i].tag = op_tag;
  for (int j = 0; j < cache->E; j++) {
    if (cache->line[op_s][j].valid == 1) {
      cache->line[op_s][j].time_tamp++;
    }
  }
  cache->line[op_s][i].time_tamp = 0;
}

// 是否命中、缺失还是冲突不命中
void update_info(int op_tag, int op_s) {
  int index = get_index(op_s, op_tag);
  if (index == -1) {
    miss_count++;
    if (verbose) printf(" miss");
    int i = is_full(op_s);
    if (i == -1) {
      eviction_count++;
      if (verbose) printf(" eviction");
      i = find_LRU(op_s);
    }
    update(i, op_s, op_tag);
  } else {
    hit_count++;
    if (verbose) printf(" hit");
    update(index, op_s, op_tag);
  }
}

void get_trace(int s, int E, int b) {
  FILE *pFile;
  pFile = fopen(t, "r");
  if (pFile == NULL) {
    exit(-1);
  }
  char identifier;
  unsigned address;
  int size;
  while (fscanf(pFile, "%c %x,%d", &identifier, &address, &size) > 0) {
    int op_tag = address >> (s + b);
    int op_s = (address >> b) & ((unsigned)(-1) >> (8 * sizeof(unsigned) - s));
    switch(identifier) {
    case 'M':
      if (verbose) printf("%c %x,%d", identifier, address, size);
      update_info(op_tag, op_s);
      update_info(op_tag, op_s);
      if (verbose) printf("\n");
      break;
    case 'L':
      if (verbose) printf("%c %x,%d", identifier, address, size);
      update_info(op_tag, op_s);
      if (verbose) printf("\n");
      break;
    case 'S':
      if (verbose) printf("%c %x,%d", identifier, address, size);
      update_info(op_tag, op_s);
      if (verbose) printf("\n");
      break;
    }
  }
  fclose(pFile);
}

void print_help() {
  printf("** A Cache Simulator by Deconx\n");
  printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
  printf("Options:\n");
  printf("-h         Print this help message.\n");
  printf("-v         Optional verbose flag.\n");
  printf("-s <num>   Number of set index bits.\n");
  printf("-E <num>   Number of lines per set.\n");
  printf("-b <num>   Number of block offset bits.\n");
  printf("-t <file>  Trace file.\n\n\n");
  printf("Examples:\n");
  printf("linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
  printf("linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

int main(int argc, char *argv[]) {
  char opt;
  int s, E, b;
  while (-1 != (opt = getopt(argc, argv, "hvs:E:b:t:"))) {
    switch (opt) {
    case 'h':
      print_help();
      exit(0);
    case 'v':
      verbose = 1;
      break;
    case 's':
      s = atoi(optarg);
      break;
    case 'E':
      E = atoi(optarg);
      break;
    case 'b':
      b = atoi(optarg);
      break;
    case 't':
      strcpy(t, optarg);
      break;
    default:
      print_help();
      exit(-1);
    }
  }
  initCache(s, E, b);
  get_trace(s, E, b);
  free_Cache();
  printSummary(hit_count, miss_count, eviction_count);
  return 0;
}
