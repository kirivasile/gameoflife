#include "master.h"

using namespace std;

vector<vector<bool> > field;
state_t state = BEFORE_START;

void startCommand(const string &fieldString) {
	vector<bool> buf;
	for (int i = 0; i < fieldString.size(); ++i) {
		if (fieldString[i] == '.') {
			field.push_back(buf);
			buf.clear();
		} else if (fieldString[i] == '1') {
			buf.push_back(true);
		} else if (fieldString[i] == '0') {
			buf.push_back(false);
		}
	}
	state = STARTED;
}

void statusCommand() {
	int x = field.size();
	if (x < 1) {
		cout << "Error in taking number of rows\n";
		return;
	}
	int y = field[0].size();
	for (int i = 0; i < x; ++i) {
		for (int j = 0; j < y; ++j) {
			cout << field[i][j] << " ";
		}
		cout << "\n";
	}
}

void runCommand(int numIterations, int numWorkers) {
	//Отправляем размер массива
	int height = field.size();
	for (int i = 1; i < numWorkers + 1; ++i) {
		MPI_Send(&height, 1, MPI_INT, i, messageType::FIELD_DATA, MPI_COMM_WORLD);
		MPI_Send(&numIterations, 1, MPI_INT, i, messageType::FIELD_DATA, MPI_COMM_WORLD);
	}  	
	int dataSize = field.size() * field.size();
	unsigned short int *data = new unsigned short int[dataSize];
	for (int i = 0; i < field.size(); ++i) {
		for (int j = 0; j < field[i].size(); ++j) {
			data[i * field.size() + j] = field[i][j];
		}
	}
	for (int i = 1; i < numWorkers + 1; ++i) {
		MPI_Send(data, dataSize, MPI_UNSIGNED_SHORT, i, messageType::FIELD_DATA, MPI_COMM_WORLD);		
	}
	MPI_Status status;
	for (int i = 1; i < numWorkers + 1; ++i) {
		int borders[2];
		MPI_Recv(borders, 2, MPI_INT, i, messageType::FINISH_BORDERS, MPI_COMM_WORLD, &status);
		MPI_Recv(data + (borders[0] * height), (borders[1] - borders[0]) * height, MPI_INT, i, messageType::FINISH_DATA, MPI_COMM_WORLD, &status);
		for (int j = borders[0]; j < borders[1]; ++j) {
			for (int k = 0; k < height; ++k) {
				field[j][k] = data[borders[0] * height + (j - borders[0]) * height + k];
			}
		}
	}
	delete[] data;
	state = state_t::STARTED;
}

void masterRoutine(int size) {
	string command;
	while(true) {		
		cin >> command;
		if (command == "START") {
			string fieldString;
			cin >> fieldString;
			if (state != state_t::BEFORE_START) {
				cout << "The system has already started\n";
				continue;
			}
			startCommand(fieldString);
		} else if (command == "STATUS") {
			if (state == state_t::BEFORE_START) {
				cout << "The system hasn't started, please use the command START\n";
				continue;
			}
			if (state == state_t::RUNNING) {
				cout << "The system is running know, please use the command STOP first\n";
				continue;
			}
			statusCommand();
		} else if (command == "RUN") {
			int numIterations;
			cin >> numIterations;
			if (numIterations < 1) {
				cout << "Please enter the positive number of iterations\n";
				continue;			
			}
			if (state == state_t::BEFORE_START) {
				cout << "The system isn't started\n";
				continue;
			}
			if (state == state_t::RUNNING) {
				cout << "The system is running\n";
				continue;
			}
			runCommand(numIterations, size - 1);
		} else if (command == "QUIT") {
			break;
		} else {
			cout << "Wrong command. Type HELP for the list\n";
		}
	}
}
