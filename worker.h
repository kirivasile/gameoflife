#pragma once

#include "dependencies.h"
#include <cmath>

unsigned short int* workerRoutine(int rank, int size, bool& stopped, int sizeFromMaster = 0, int masterNumIt = 0, int itToStop = 0);
