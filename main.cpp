#include "dependencies.h"
#include "master.h"
#include "worker.h"

using namespace std;

int main(int argc, char** argv) {
	int mpiStatus = MPI_Init(&argc, &argv);	
	if (mpiStatus != MPI_SUCCESS) {
		cout << "Failed to init MPI\n";
		return 1;
	}
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	if (size <= 1) {
		cout << "Not enough free workers\n";
		return 1;
	}
	//Разделение комунникаторов
	int color1 = -1, color2 = -1;
	if (rank == 0 || rank == 1) {
		color1 = 1;
	} else {
		color1 = 2;
	}
	if (rank == 0) {
		color2 = 1;
	} else {
		color2 = 2;
	}
	MPI_Comm StopComm, WorkerComm;
	MPI_Comm_split(MPI_COMM_WORLD, color1, rank, &StopComm);
	MPI_Comm_split(MPI_COMM_WORLD, color2, rank, &WorkerComm);
	if (rank == 0) {
		masterRoutine(size);
	} else {
		bool temp;
		int tempInt;
		workerRoutine(rank, size - 1, temp, tempInt, tempInt);
	}
	mpiStatus = MPI_Finalize();
	if (mpiStatus != MPI_SUCCESS) {
		cout << "Failed to finalize MPI\n";
		return 1;
	}
	return 0;
}
