#include "pending_task.h"
#include <stdlib.h>

static pending_task_t pending_task = {NULL, NULL};

void pending_task_set(pending_task_method method, void *args) {
  if (!pending_task.method) {
    pending_task.method = method;
    pending_task.args = args;
  }
}

void pending_task_run(void) {
  if (pending_task.method) {
    pending_task.method(pending_task.args);
    free(pending_task.args);
    pending_task.method = NULL;
    pending_task.args = NULL;
  }
}