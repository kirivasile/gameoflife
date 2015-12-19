#include "worker.h"

using namespace std;

void workerRoutine(int rank, int size) {
	cout << rank << " started\n";
	MPI_Status status;
	int *height = new int[1];
	MPI_Recv(height, 1, MPI_INT, 0, messageType::FIELD_DATA, MPI_COMM_WORLD, &status);
	int fieldSize = height[0];
	cout << rank << ": size=" << fieldSize << "\n";
	unsigned short int *data = new unsigned short int[fieldSize * fieldSize];
	MPI_Recv(data, fieldSize * fieldSize, MPI_UNSIGNED_SHORT, 0, messageType::FIELD_DATA, MPI_COMM_WORLD, &status);
	vector<vector<bool> > field = vector<vector<bool> >(fieldSize, vector<bool>(fieldSize));
	for (int i = 0; i < fieldSize; ++i) {
		for (int j = 0; j < fieldSize; ++j) {
			field[i][j] = data[i * fieldSize + j] == 1 ? true : false;
		}
	}
	cout << "Worker #" << rank << "\n";
	for (int i = 0; i < fieldSize; ++i) {
		for (int j = 0; j < fieldSize; ++j) {
			cout << (field[i][j] == true ? 1 : 0) << " ";
		}
		cout << "\n";
	}
}
