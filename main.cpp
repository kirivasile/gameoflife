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
	if (rank == 0) {
		masterRoutine(size);
	} else {
		bool temp;
		workerRoutine(rank, size - 1, temp);
	}
	mpiStatus = MPI_Finalize();
	if (mpiStatus != MPI_SUCCESS) {
		cout << "Failed to finalize MPI\n";
		return 1;
	}
	return 0;
}
