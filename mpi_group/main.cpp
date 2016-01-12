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
	MPI_Comm workerComm;
	int color = rank == 0 ? 1 : 2;
	MPI_Comm_split(MPI_COMM_WORLD, color, rank, &workerComm);
	if (rank == 0) {
		masterRoutine(size);
	} else {
		workerRoutine(rank, size - 1, workerComm);
	}
	mpiStatus = MPI_Finalize();
	if (mpiStatus != MPI_SUCCESS) {
		cout << "Failed to finalize MPI\n";
		return 1;
	}
	return 0;
}
