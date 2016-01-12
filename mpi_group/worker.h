#pragma once

#include "dependencies.h"
#include <cmath>

void workerRoutine(int rank, int size, MPI_Comm& workerComm);

void ibcast(int* stopSignal);

unsigned short int* gatherCommit(unsigned short int* data, int count, int id, int numWorkers, int height);
