#pragma once

typedef void (*pending_task_method)(const void *args);

typedef struct {
  pending_task_method method;
  void *args;
} pending_task_t;

void pending_task_set(pending_task_method method, void *args);

void pending_task_run(void);