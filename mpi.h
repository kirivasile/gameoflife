#pragma once

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <stdio.h>
#include <mpi.h>
#include "master.h"
#include "worker.h"

enum state_t {
	BEFORE_START,
	STARTED,
	RUNNING
};