#pragma once

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "sys/times.h"
//#include "sys/vtimes.h"

void cpu_process_load_init();
void cpu_total_load_init();

double cpu_process_load_value();
double cpu_total_load_value();

int mem_parse_line(char* line);
int mem_process_load_value();