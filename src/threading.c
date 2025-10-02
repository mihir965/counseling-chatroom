#include "../include/threading.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *agent_assign_thread(void *arg) {
  struct agent_assign_args *args = (struct agent_assign_args *)arg;

  printf("Sleeping for 2 seconds to let the agent join\n");

  sleep(2);

  if (!room_assign_ai_agent(args->room, args->agent_name)) {
    printf("There was an error assigning an agent to the room\n");
  }

  free(args->agent_name);
  free(args);
  return NULL;
}
