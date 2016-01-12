#pragma once

#include "dependencies.h"

enum state_t {
	BEFORE_START,
	STARTED,
	RUNNING
};

void masterRoutine(int size);
