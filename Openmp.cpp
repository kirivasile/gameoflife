#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <pthread.h>
#include <semaphore.h>
#include <cmath>
#include <ctime>
#include <stdio.h>
#include <omp.h>

using namespace std;

typedef vector<vector<bool> > field_t;

struct args_t {
	args_t() {}
	args_t(int num, int id) : numIterations(num), pid(id) {}
	int numIterations;
	int pid;
};

enum state_t {
	BEFORE_START,
	STARTED,
	RUNNING
};


field_t field, field2;					//Поле
state_t state = state_t::BEFORE_START; 		//Состояние, в котором находится сейчас программа
unsigned int NUM_WS;				//Число потоков
unsigned int stoppedIteration; 			//Текущая итерация
bool gameFinished; 				//Остановлена ли игра
bool stopped;
int whichTable;				//Какую таблицу использовать для чтения
bool noErrors;				//Если при вводе данных при команде START не было ошибок
int numIterations;

void initializeStructures() {
	state = state_t::STARTED;
	stoppedIteration = 0;
	gameFinished = false;
	whichTable = 0;
	noErrors = false;
}

int startCommand(const string &filePath) {
	initializeStructures();
	ifstream file;
	file.open(filePath);
	field.clear();
	field2.clear();
	while (!file.eof()) {
		vector<bool> buf;
		string str;
		getline(file, str);
		for (unsigned int i = 0; i < str.size(); ++i) {
			if (str[i] == '1') {
				buf.push_back(true);
			}
			if (str[i] == '0') {
				buf.push_back(false);
			}
		}
		field.push_back(buf);
		field2.push_back(buf);
	}
	file.close();
	return field.size();
}

void randomCommand(const unsigned int &numRows, const unsigned int &numCols) {
	initializeStructures();
	vector<bool> buf(numCols, false);
	field.resize(numRows, buf);
	field2.resize(numRows, buf);
	srand(time(0));
	for (int i = 0; i < numRows; ++i) {
		for (int j = 0; j < numCols; ++j) {
			bool value = rand() % 2;
			field[i][j] = value;
			field2[i][j] = value;
		}
	}
}

void statusCommand() {
	int x = field.size();
	if (x < 1) {
		cout << "Error in taking number of rows\n";
		return;
	}
	cout << "Iteration #" << stoppedIteration << "\n";
	int y = field[0].size();
	for (int i = 0; i < x; ++i) {
		for (int j = 0; j < y; ++j) {
			if (whichTable == 0) {
				cout << field[i][j] << " ";
			} else {
				cout << field2[i][j] << " ";
			}
		}
		cout << "\n";
	}
}

void runParallel(int numIterations) {
	int id = omp_get_thread_num();
	int x = field.size();
	if (x < 1) {
		printf("Pid: %d - Error in taking number of rows\n", id);
	}
	int y = field[0].size();
	int range = ceil((double)x / (double)NUM_WS);
	int upBorder = min(range * id, x), downBorder = min(range * (id + 1), x);
	int down = (id + NUM_WS + 1) % NUM_WS, up = (id + NUM_WS - 1) % NUM_WS;
	field_t *ptr1, *ptr2;
	if (whichTable == 0) {
		ptr1 = &field;
		ptr2 = &field2;
	} else {
		ptr1 = &field2;
		ptr2 = &field;
	}
	for (int it = 0; it < numIterations; ++it) {
		for (int i = upBorder; i < downBorder; ++i) {
			for (int j = 0; j < y; ++j) {
				int numNeighbors = 0;
				int upper = (i + x - 1) % x;
				int lower = (i + x + 1) % x;
				int left = (j + y - 1) % y;
				int right = (j + y + 1) % y;
				if ((*ptr1)[upper][j]) {
					numNeighbors++;
				}
				if ((*ptr1)[upper][left]) {
					numNeighbors++;
				}
				if ((*ptr1)[upper][right]) {
					numNeighbors++;
				}
				if ((*ptr1)[lower][j]) {
					numNeighbors++;
				}
				if ((*ptr1)[lower][left]) {
					numNeighbors++;
				}
				if ((*ptr1)[lower][right]) {
					numNeighbors++;
				}
				if ((*ptr1)[i][left]) {
					numNeighbors++;
				}
				if ((*ptr1)[i][right]) {
					numNeighbors++;
				}
				if (!(*ptr1)[i][j] && numNeighbors == 3) {
					(*ptr2)[i][j] = true;
				} else if ((*ptr1)[i][j] && (numNeighbors < 2 || numNeighbors > 3)) {
					(*ptr2)[i][j] = false;
				} else {
					(*ptr2)[i][j] = (*ptr1)[i][j];
				}
			}
		}
		swap(ptr1, ptr2);
		#pragma omp single
		{
			if (gameFinished) {
				stoppedIteration += it;
				whichTable = (whichTable + it) % 2;
				printf("Stopped at iteration #%d\n", stoppedIteration);
				stopped = true;
			}
		}
		if (stopped && gameFinished) {
			return;
		}
	}
	if (id == 0) {
		stoppedIteration += numIterations;
		whichTable = (whichTable + numIterations) % 2;
	}
	return;
}

void stopCommand() {
	gameFinished = true;
	state = state_t::STARTED;
}

int main() {
	#ifndef _OPENMP
		cout << "OpenMP is not supported!\n";
		return 1;
	#endif
	string command;
	cout << "To show the list of commands type HELP\n";

	#pragma omp parallel num_threads(2)
	{
		while(true) {
			#pragma omp master 
			{
				cout << "$ ";
				cin >> command;
			}
			if (command == "START") {
				#pragma omp master 
				{
					int numWorkers = 1;
					cin >> numWorkers;
					if (numWorkers < 1) {
						cout << "Please, enter positive number of workers\n";
					} else if (state != state_t::BEFORE_START) {
						cout << "The system has already started\n";
					} else {
					NUM_WS = numWorkers;
					omp_set_num_threads(NUM_WS);
					string filePath;
					cin >> filePath;
					int numRows = startCommand(filePath);
						if (numWorkers * 2 > numRows) {
							cout << "Number of workers should be not greater than half number of rows\n";
							state = state_t::BEFORE_START;
						}
					}
				}
			}
			else if (command == "RANDOM") {
				#pragma omp master 
				{
					int numRows;
					int numCols;
					int numWorkers = 1;
					cin >> numWorkers >> numRows >> numCols;
					if (numRows < 1 || numCols < 1 || numWorkers < 1) {
						cout << "Please, enter positive numbers\n";
					} else if (numWorkers * 2 > numRows) {
						cout << "Number of workers should be not greater than half number of rows\n";
					} else if (state != state_t::BEFORE_START) {
						cout << "The system has already started\n";
					} else {
						NUM_WS = numWorkers;
						randomCommand(numRows, numCols);
					}
				}
			}
			else if (command == "RUN") {
				#pragma omp master 
				{
					cin >> numIterations;
					if (numIterations < 1) {
						cout << "Please, enter positive number of iterations\n";
					} else if (state == state_t::BEFORE_START) {
						cout << "The system didn't start, please use the command START\n";
					} else {
						gameFinished = false;
						stopped = false;
						state = state_t::RUNNING;
						noErrors = true;
					}
				}
				#pragma omp barrier
				if (noErrors) {
					omp_set_nested(1);
					int threadId = omp_get_thread_num();
					if (threadId == 1) {
						#pragma omp parallel num_threads(NUM_WS)
						{
							runParallel(numIterations);
						}

					}
				}
			}
			else if (command == "TIMERUN") {
				#pragma omp master 
				{
					cin >> numIterations;
					if (numIterations < 1) {
						cout << "Please, enter positive number of iterations\n";
					} else if (state == state_t::BEFORE_START) {
						cout << "The system didn't start, please use the command START\n";
					} else {
						gameFinished = false;
						stopped = false;
						state = state_t::RUNNING;
						time_t startTimer;
						time_t endTimer;
						time(&startTimer);
						omp_set_nested(1);
						#pragma omp parallel num_threads(NUM_WS)
						{
							runParallel(numIterations);
						}
						time(&endTimer);
						double seconds = difftime(endTimer, startTimer);
						cout << "\n" << numIterations << " iterations were completed in ";
						cout << seconds << " sec\n";
						state = state_t::STARTED;
					}
				}
			}
			else if (command == "STATUS") {
				#pragma omp master 
				{
					if (state == state_t::BEFORE_START) {
						cout << "The system hasn't started, please use the command START\n";
					} else if (state == state_t::RUNNING) {
						cout << "The system is running know, please use the command STOP first\n";
					} else {
						statusCommand();
					}
				}
			}
			else if (command == "STOP") {
				#pragma omp master 
				{
					if (state != state_t::RUNNING) {
						cout << "The system isn't running. Please, use RUN or START\n";
					} else {
						stopCommand();
					}
				}
			}
			else if (command == "QUIT") {
				break;
			}
			else if (command == "HELP") {
				#pragma omp master 
				{
					cout << "\nList of commands:\n";
					cout << "  START n file : n - number of workers, file - name of file with field,\n";
					cout << "                 number of workers must be not greater than number of rows\n";
					cout << "  RANDOM n x y : random field generation, n - number of workers,\n";
					cout << "                 x - num of rows, y - num of cols\n";
					cout << "  RUN n        : run n iterations\n";
					cout << "  TIMERUN n    : run n iterations with showing the time, and automatically stop\n";
					cout << "  STOP         : stop workers immediately\n";
					cout << "  STATUS       : print current iteration and field. Use STOP before using it\n";
					cout << "  WAIT         : wait till the workers stop and quit the programm.\n";
					cout << "  QUIT         : quit the game\n";
					cout << "  HELP         : short commands guide\n\n";
				}
			} else {
				#pragma omp master 
				{
					cout << "Wrong command. Type HELP for the list\n";
				}
			}
		}
	}
	return 0;
}
