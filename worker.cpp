#include "worker.h"
#include <stdio.h>

using namespace std;

unsigned short int* workerRoutine(int id, int numWorkers, bool& stopped, int sizeFromMaster, int masterNumIt) { //sizeFromMaster = -1 в потоках, кроме мастера
	int upBorder = 0, downBorder = 0, sizeForMPI = 0; //для MPI_Gatherv
	int numIterations, range, down, up, fieldSize;
	vector<vector<bool> > field;
	MPI_Status status;
	unsigned short int* commitData;
        if (id == 0) {
                commitData = new unsigned short int[sizeFromMaster * sizeFromMaster];
		numIterations = masterNumIt;
        }
	if (id != 0) {
	fieldSize = 0;
	MPI_Recv(&fieldSize, 1, MPI_INT, 0, messageType::FIELD_DATA, MPI_COMM_WORLD, &status);
	//QUIT command
	if (fieldSize == -1) {
		return NULL;
	}
	sizeForMPI = fieldSize;
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
	range = ceil((double)fieldSize / (double)numWorkers);
        upBorder = min(range * (id - 1), fieldSize);
	downBorder = min(range * id, fieldSize);
        down = id == numWorkers ? 1 : id + 1;
	up = id == 1 ? numWorkers : id - 1;
 	}
	for (int it = 0; it < numIterations; ++it) {
		if (id != 0) {
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
		//Коммит
		}
		if (it % 2 == 0) {
			if (id == 0) {
				sizeForMPI = sizeFromMaster;
			}
			unsigned short int* dataForCommit = new unsigned short int[fieldSize * (downBorder - upBorder)];
			for (int i = upBorder; i < downBorder; ++i) {
				for (int j = 0; j < fieldSize; ++j) {
					dataForCommit[(i - upBorder) * fieldSize + j] = field[i][j];
				}
			}
			int* masDispr;
			int* masCountr;
			if (id == 0) {
				masDispr = new int[numWorkers + 1];
				masCountr = new int[numWorkers + 1];
				masDispr[0] = 0;
				masDispr[1] = 0;
				masCountr[0] = 0;
				printf("Id = %d, i=0, dispr = %d, countr = %d\n", id, masDispr[0], masCountr[0]);
				for (int i = 1; i < numWorkers + 1; ++i) {
					int rangeMPI = ceil((double)sizeForMPI / (double)numWorkers);
					if (i != numWorkers) {
						masDispr[i + 1] = masDispr[i] + rangeMPI * sizeForMPI;
					}
					int upBorderForCount = min(rangeMPI * (i - 1), sizeForMPI);
					int downBorderForCount = min(rangeMPI * i, sizeForMPI);
					masCountr[i] = (downBorderForCount - upBorderForCount) * sizeForMPI;
					if (it != 10) {
						printf("Id = %d, i=%d, dispr = %d, countr = %d\n", id, i, masDispr[i], masCountr[i]);
					}
				}
				for (int i = 0; i < sizeForMPI * sizeForMPI; ++i) {
					commitData[i] = 2;
				}
			}
			if (id != 5) {
				printf("Worker %d sent amount = %d\n", id, sizeForMPI * (downBorder - upBorder));
				for (int i = 0; i < downBorder - upBorder; ++i) {
					for (int j = 0; j < sizeForMPI; ++j) {
						printf("%d ", dataForCommit[i * fieldSize + j]);
					}
					printf("\n");
				}
			}
			int amount = sizeForMPI * (downBorder - upBorder);
			MPI_Gatherv(dataForCommit, amount, MPI_UNSIGNED_SHORT, commitData,
					masCountr, masDispr, MPI_UNSIGNED_SHORT, 0, MPI_COMM_WORLD);
			//if (id == 1) {
			//	MPI_Send(dataForCommit, amount, MPI_UNSIGNED_SHORT, 0, 0, MPI_COMM_WORLD);
			//}
			//if (id == 0) {
			//	MPI_Recv(commitData, sizeForMPI * sizeForMPI, MPI_UNSIGNED_SHORT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//}
			if (id == 0) {
				printf("Master received\n");
				for (int i = 0; i < sizeForMPI; ++i) {
					for (int j = 0; j < sizeForMPI; ++j) {
						printf("%d ", commitData[i * sizeForMPI + j]);
					}
					printf("\n");
				}
			}
			if (id == 0 && stopped) {
                                return commitData;
                        }
			if (id != 0) {
				break;
			}
		} 			
	}
	if (id != 0) {
	//Пошлём мастеру наши границы и данные
	printf("Worker %d stopped at %d iterations\n", id, numIterations);
	int borders[2];
	borders[0] = upBorder;
	borders[1] = downBorder;
	MPI_Send(borders, 2, MPI_INT, 0, messageType::FINISH_BORDERS, MPI_COMM_WORLD);
	unsigned short int *dataForMaster = new unsigned short int[fieldSize * (downBorder - upBorder)];
	for (int i = upBorder; i < downBorder; ++i) {
		for (int j = 0; j < fieldSize; ++j) {
			dataForMaster[(i - upBorder) * fieldSize + j] = field[i][j];
		}
	}
	MPI_Send(dataForMaster, fieldSize * (downBorder - upBorder), MPI_UNSIGNED_SHORT, 0, messageType::FINISH_DATA, MPI_COMM_WORLD);
	}
	if (id == 0) {
		printf("wrong\n");
	}
	return commitData;
}
