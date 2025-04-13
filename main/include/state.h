#pragma once
#include "global.h"

void beginSystemState();
void stateUpdate(void *e);

void changeState(SystemState newState);
void requestChangeState(SystemState requestedState);

// Print the current state
const char *getState(SystemState state, char *buffer);


bool inWebConfigMode();
