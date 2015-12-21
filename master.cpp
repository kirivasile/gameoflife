#include "master.h"
#include <unistd.h>
#include <ctime>

using namespace std;

vector<vector<bool> > field;
state_t state = BEFORE_START;
int numWorkers = 0;

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

void randomCommand(int x, int y) {
	srand(time(NULL));
	field = vector<vector<bool> >(x, vector<bool>(y));
	for (int i = 0; i < x; ++i) {
		for (int j = 0; j < y; ++j) {
			field[i][j] = rand() % 2;
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

void runCommand(int numIterations) {
	//Отправляем размер массива
	state = state_t::RUNNING;
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
	//Искуственная задержка
	sleep(1);
}

void timerunCommand(int numIterations) {
	double startTime = MPI_Wtime();
	state = state_t::RUNNING;
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
	//Получаем данные
 	state = state_t::STARTED;
	for (int i = 1; i < numWorkers + 1; ++i) {
                int borders[2];
                MPI_Recv(borders, 2, MPI_INT, i, messageType::FINISH_BORDERS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(data + (borders[0] * height), (borders[1] - borders[0]) * height, MPI_INT, i, messageType::FINISH_DATA, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for (int j = borders[0]; j < borders[1]; ++j) {
                        for (int k = 0; k < height; ++k) {
                                field[j][k] = data[borders[0] * height + (j - borders[0]) * height + k];
                        }
                }
        }
	double endTime = MPI_Wtime();
	cout << "Working time: " << endTime - startTime << "\n";       
}

void stopCommand() {
	int height = field.size();
	int dataSize = height * height;
        unsigned short int *data = new unsigned short int[dataSize];
	int stop = 1;
	MPI_Send(&stop, 1, MPI_INT, 1, messageType::STOP_SGN, MPI_COMM_WORLD);
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

void quitCommand() {
	int quit = -1;
	for (int worker = 1; worker < numWorkers + 1; ++worker) {
		MPI_Send(&quit, 1, MPI_INT, worker, messageType::FIELD_DATA, MPI_COMM_WORLD);
	}	
}

void masterRoutine(int size) {
	string command;
	numWorkers = size - 1;
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
		} else if (command == "RANDOM") {
			int x, y;
			cin >> x >> y;
			if (state != state_t::BEFORE_START) {
				cout << "The system has already started\n";
				continue;
			}
			randomCommand(x,y);
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
			runCommand(numIterations);
		} else if (command == "TIMERUN") {
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
                        timerunCommand(numIterations);
		} else if (command == "STOP") {
			if (state != state_t::RUNNING) {
				cout << "The system isn't running\n";
				continue;
			}
			stopCommand();
		} else if (command == "QUIT") {
			quitCommand();
			break;
		} else {
			cout << "Wrong command. Type HELP for the list\n";
		}
	}
}
