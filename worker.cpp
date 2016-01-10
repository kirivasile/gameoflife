#include "worker.h"
#include <cstdio>
#include <cstring>

using namespace std;

MPI_Request* req;

void ibcast(int* stopSignal) {
	req = new MPI_Request();
	MPI_Ibcast(stopSignal, 1, MPI_INT, 0, MPI_COMM_WORLD, req);
}

unsigned short int* gatherCommit(unsigned short int* data, int count, int id, int numWorkers, int height) {
	int* masDispr;
        int* masCountr;
        if (id == 0) {
        	masDispr = new int[numWorkers + 1];
                masCountr = new int[numWorkers + 1];
                masDispr[0] = 0;
                masDispr[1] = 0;
                masCountr[0] = 0;
		int range = floor((double)height / (double)numWorkers);
		int diff = max(0, height  - range * numWorkers);
		vector<int> upBorders = vector<int>(numWorkers + 1, 0);
	       	for (int i = 2; i < numWorkers + 1; ++i) {
			if (i <= diff + 1) {
	                	upBorders[i] = upBorders[i - 1] + range + 1;
        		} else {
                		upBorders[i] = upBorders[i - 1] + range;
        		}
		}
		for (int i = 1; i < numWorkers + 1; ++i) {
			int upBorder = upBorders[i];
			int downBorder = i == numWorkers ? height : upBorders[i + 1];
			masCountr[i] = (downBorder - upBorder) * height;
		}
		for (int i = 1; i < numWorkers; ++i) {
			masDispr[i + 1] = masDispr[i] + masCountr[i];
		}
        }
	unsigned short int* commitData = new unsigned short int[height * height];
        MPI_Gatherv(data, count, MPI_UNSIGNED_SHORT, commitData,
		masCountr, masDispr, MPI_UNSIGNED_SHORT, 0, MPI_COMM_WORLD);
	return commitData;		
}

void workerRoutine(int id, int numWorkers, MPI_Comm& workerComm) {
	while(true) {
	int upBorder = 0, downBorder = 0;
	int numIterations, range, down, up, fieldSize;
	int commitNum = 0;
	vector<vector<bool> > field;
	MPI_Status status;

	int* stopSignal = new int();
	*stopSignal = 0;
	ibcast(stopSignal);

	fieldSize = 0;
	MPI_Recv(&fieldSize, 1, MPI_INT, 0, messageType::FIELD_DATA, MPI_COMM_WORLD, &status);
	//QUIT command
	if (fieldSize == -1) {
		return;
	}
	numIterations = 0;
	MPI_Recv(&numIterations, 1, MPI_INT, 0, messageType::FIELD_DATA, MPI_COMM_WORLD, &status);
	unsigned short int *data = new unsigned short int[fieldSize * fieldSize];
	MPI_Recv(data, fieldSize * fieldSize, MPI_UNSIGNED_SHORT, 0, messageType::FIELD_DATA, MPI_COMM_WORLD, &status);
	field = vector<vector<bool> >(fieldSize, vector<bool>(fieldSize));
	for (int i = 0; i < fieldSize; ++i) {
		for (int j = 0; j < fieldSize; ++j) {
			field[i][j] = data[i * fieldSize + j] == 1 ? true : false;
		}
	}
	//Очень сложное распределение поля для рабочих
	range = floor((double)fieldSize / (double)numWorkers);
	int diff = max(0, fieldSize  - range * numWorkers);
	vector<int> upBorders = vector<int>(numWorkers + 1, 0);
	for (int i = 2; i < numWorkers + 1; ++i) {
		if (i <= diff + 1) {
			upBorders[i] = upBorders[i - 1] + range + 1;
		} else {
			upBorders[i] = upBorders[i - 1] + range;
		}
	}
	upBorder = upBorders[id];
	downBorder = id == numWorkers ? fieldSize : upBorders[id + 1];
        down = id == numWorkers ? 1 : id + 1;
	up = id == 1 ? numWorkers : id - 1;
	unsigned short int* dataForCommit = new unsigned short int[fieldSize * (downBorder - upBorder)];
	unsigned short int* prevCommit = new unsigned short int[fieldSize * (downBorder - upBorder)];
	int mpiTest = 0;
	for (int it = 0; it < numIterations; ++it) {
		vector<vector<bool> > writeField = field;
		for (int i = upBorder; i < downBorder; ++i) {
			for (int j = 0; j < fieldSize; ++j) {
				int numNeighbors = 0;
				int upper = (i + fieldSize - 1) % fieldSize;
				int lower = (i + fieldSize + 1) % fieldSize;
				int left = (j + fieldSize - 1) % fieldSize;
				int right = (j + fieldSize + 1) % fieldSize;
				if (field[upper][j])	numNeighbors++;
				if (field[upper][left]) numNeighbors++;
				if (field[upper][right]) numNeighbors++;
				if (field[lower][j]) numNeighbors++;
				if (field[lower][left]) numNeighbors++;
				if (field[lower][right]) numNeighbors++;
				if (field[i][left]) numNeighbors++;
				if (field[i][right]) numNeighbors++;
				if (!field[i][j] && numNeighbors == 3) {
					writeField[i][j] = true;
				} else if (field[i][j] && (numNeighbors < 2 || numNeighbors > 3)) {
					writeField[i][j] = false;
				}
			}
		}
		unsigned short int *dataForUp, *dataForDown;
		dataForUp = new unsigned short int[fieldSize];
		dataForDown = new unsigned short int[fieldSize];
		for (int i = 0; i < fieldSize; ++i) {
			dataForUp[i] = writeField[upBorder][i];
			dataForDown[i] = writeField[downBorder - 1][i];
		}
		if (up != id) {
			MPI_Send(dataForUp, fieldSize, MPI_UNSIGNED_SHORT, up, messageType::DOWN_DATA, MPI_COMM_WORLD);
		}
		if (down != id) {
			MPI_Send(dataForDown, fieldSize, MPI_UNSIGNED_SHORT, down, messageType::UP_DATA, MPI_COMM_WORLD);
		}
		if (up != id) {
			MPI_Recv(dataForUp, fieldSize, MPI_UNSIGNED_SHORT, up, messageType::UP_DATA, MPI_COMM_WORLD, &status);
		}
		if (down != id) {
                        MPI_Recv(dataForDown, fieldSize, MPI_UNSIGNED_SHORT, down, messageType::DOWN_DATA, MPI_COMM_WORLD, &status);
                }
		int upMessageRow = upBorder == 0 ? fieldSize - 1 : upBorder - 1;
		int downMessageRow = downBorder == fieldSize ? 0 : downBorder;
		for (int i = 0; i < fieldSize; ++i) {
			writeField[upMessageRow][i] = dataForUp[i];
			writeField[downMessageRow][i] = dataForDown[i];
		}
		field = writeField;
		//Проверка на stop
		if (mpiTest != 2) {
			MPI_Test(req, &mpiTest, MPI_STATUS_IGNORE);
		}
		if(mpiTest == 1) {
			if (up != id) {
                	        MPI_Send(dataForUp, fieldSize, MPI_UNSIGNED_SHORT, up, messageType::DOWN_DATA, MPI_COMM_WORLD);
        	        }
	                if (down != id) {
                        	MPI_Send(dataForDown, fieldSize, MPI_UNSIGNED_SHORT, down, messageType::UP_DATA, MPI_COMM_WORLD);
                	}
			printf("Worker %d caught stopSignal at %d iteration\n", id, it);
			int* commitNums = new int[numWorkers];
			MPI_Allgather(&commitNum, 1, MPI_INT, commitNums, 1, MPI_INT, workerComm);
			int minCommitNum = numIterations + 1;
			for (int i = 0; i < numWorkers; ++i) {
				if (commitNums[i] < minCommitNum) {
					minCommitNum = commitNums[i];
				}
			}
			if (minCommitNum != commitNum) {
				gatherCommit(prevCommit, (downBorder - upBorder) * fieldSize, id, numWorkers, fieldSize);
			} else {
				gatherCommit(dataForCommit, (downBorder - upBorder) * fieldSize, id, numWorkers, fieldSize);
			}
			return;
                }
		if (it % 100 == 0) {
			commitNum = it;		
			for (int i = upBorder; i < downBorder; ++i) {
         		       	for (int j = 0; j < fieldSize; ++j) {
					prevCommit[(i - upBorder) * fieldSize + j] = dataForCommit[(i - upBorder) * fieldSize + j];
    		                	dataForCommit[(i - upBorder) * fieldSize + j] = field[i][j];
                		}
        		}
		}
	}
	gatherCommit(dataForCommit, (downBorder - upBorder) * fieldSize, id, numWorkers, fieldSize);
	return;
	}
}
