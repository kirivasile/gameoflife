#include "worker.h"
#include <stdio.h>

using namespace std;

void workerRoutine(int id, int numWorkers) {
	while (true) {
	MPI_Status status;
	int fieldSize = 0;
	MPI_Recv(&fieldSize, 1, MPI_INT, 0, messageType::FIELD_DATA, MPI_COMM_WORLD, &status);
	//QUIT command
	if (fieldSize == -1) {
		return;
	}
	int numIterations = 0;
	MPI_Recv(&numIterations, 1, MPI_INT, 0, messageType::FIELD_DATA, MPI_COMM_WORLD, &status);
	unsigned short int *data = new unsigned short int[fieldSize * fieldSize];
	MPI_Recv(data, fieldSize * fieldSize, MPI_UNSIGNED_SHORT, 0, messageType::FIELD_DATA, MPI_COMM_WORLD, &status);
	vector<vector<bool> > field = vector<vector<bool> >(fieldSize, vector<bool>(fieldSize));
	for (int i = 0; i < fieldSize; ++i) {
		for (int j = 0; j < fieldSize; ++j) {
			field[i][j] = data[i * fieldSize + j] == 1 ? true : false;
		}
	}
	//Очень сложное распределение поля для рабочих
	int range = floor((double)fieldSize / (double)numWorkers);
	int diff = max(0, fieldSize  - range * numWorkers);
	vector<int> upBorders = vector<int>(numWorkers + 1, 0);
	for (int i = 2; i < numWorkers + 1; ++i) {
		if (i <= diff + 1) {
			upBorders[i] = upBorders[i - 1] + range + 1;
		} else {
			upBorders[i] = upBorders[i - 1] + range;
		}
	}
	int upBorder = upBorders[id], downBorder = id == numWorkers ? fieldSize : upBorders[id + 1];	
        int down = id == numWorkers ? 1 : id + 1;
	int up = id == 1 ? numWorkers : id - 1; 
	//printf("Worker %d, range=%d, upBorder=%d, downBorder=%d,down=%d,up=%d\n",id,range,upBorder,downBorder,down,up);
	int stop = 0;
	MPI_Request request;
	if (id == 1) {
		MPI_Irecv(&stop, 1, MPI_INT, 0, messageType::STOP_SGN, MPI_COMM_WORLD, &request);
	} else {
		MPI_Irecv(&numIterations, 1, MPI_INT, 1, messageType::STOP_SGN, MPI_COMM_WORLD, &request);
	}
	for (int it = 0; it < numIterations; ++it) {
		if (id == 1) {
			if (stop == 1) {
				int newNumIt = it + numWorkers / 2 + 1;
				for (int worker = 2; worker < numWorkers + 1; ++worker) {
					MPI_Ssend(&newNumIt, 1, MPI_INT, worker, messageType::STOP_SGN, MPI_COMM_WORLD);
				}
				numIterations = newNumIt;
				stop = 0;
			}					
		}
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
	}
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
	/*for (int i = upBorder; i < downBorder; ++i) {
                for (int j = 0; j < fieldSize; ++j) {
                	cout << field[i][j] << " ";
                }
       		cout << "\n\n";
        }*/
	}	
}
