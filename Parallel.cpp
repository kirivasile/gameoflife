#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <pthread.h>
#include <semaphore.h>
#include <cmath>
#include <ctime>
#include <stdio.h>

using namespace std;


typedef pthread_cond_t cond_t;
typedef pthread_mutex_t mutex_t;
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
vector<pthread_t> threads; 			//Рабочие
vector<args_t*> myArgs; 			//Аргументы для потока
vector<bool> isReady;					
unsigned int stoppedIteration; 			//Текущая итерация
bool gameFinished; 				//Остановлена ли игра
int whichTable;				//Какую таблицу использовать для чтения
sem_t iterationSem;				
mutex_t mutexCV;
cond_t iterationCV;


void initializeStructures() {
	threads.resize(NUM_WS);
	myArgs.resize(NUM_WS);
	state = state_t::STARTED;
	isReady.resize(NUM_WS, false);
	stoppedIteration = 0;
	gameFinished = false;
	whichTable = 0;
	mutexCV = PTHREAD_MUTEX_INITIALIZER;
	iterationCV = PTHREAD_COND_INITIALIZER;
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

void* runParallel(void* arg) {
	args_t* arg_str = (args_t*)arg;
	int numIterations = arg_str->numIterations;
	int id = arg_str->pid;
	int x = field.size();
	if (x < 1) {
		printf("Pid: %d - Error in taking number of rows\n", id);
		return NULL;
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
		int lockStatus = sem_trywait(&iterationSem);
		if (lockStatus != 0) {
			pthread_mutex_lock(&mutexCV);
			sem_init(&iterationSem, 0, NUM_WS - 1);
			for (int i = 0; i < NUM_WS; ++i) {
				isReady[i] = true;
			}
			pthread_cond_broadcast(&iterationCV);
			pthread_mutex_unlock(&mutexCV);
		}

		if (gameFinished) {
			if (id == 0) {
				stoppedIteration += it;
				whichTable = (whichTable + it) % 2;
				printf("Stopped at iteration #%d\n", stoppedIteration);
			}
			return NULL;
		}
		
		pthread_mutex_lock(&mutexCV);
		while (!isReady[id]) {
			pthread_cond_wait(&iterationCV, &mutexCV);
		}
		isReady[id] = false;
		pthread_mutex_unlock(&mutexCV);
	}
	if (id == 0) {
		stoppedIteration += numIterations;
		whichTable = (whichTable + numIterations) % 2;
	}
	return NULL;
}

void stopCommand() {
	gameFinished = true;
	for (int i = 0; i < NUM_WS; ++i) {
		pthread_join(threads[i], NULL);
	}
	state = state_t::STARTED;
}

void quitCommand() {
	for (int i = 0; i < NUM_WS; ++i) {
		delete myArgs[i];
	}
	pthread_mutex_destroy(&mutexCV);
	sem_destroy(&iterationSem);
}

int main() {
	string command;
	cout << "To show the list of commands type HELP\n";
	while(true) {
		cout << "$ ";
		cin >> command;
		if (command == "START") {
			int numWorkers = 1;
			cin >> numWorkers;
			NUM_WS = numWorkers;
			string filePath;
			cin >> filePath;
			if (numWorkers < 1) {
				cout << "Please, enter positive number of workers\n";
				continue;
			}
			if (state != state_t::BEFORE_START) {
				cout << "The system has already started\n";
				continue;
			}
			int numRows = startCommand(filePath);
			if (numWorkers * 2 > numRows) {
				cout << "Number of workers should be not greater than half number of rows\n";
				state = state_t::BEFORE_START;
				continue;
			}
		}
		else if (command == "RANDOM") {
			int numRows;
			int numCols;
			int numWorkers = 1;
			cin >> numWorkers >> numRows >> numCols;
			if (numRows < 1 || numCols < 1 || numWorkers < 1) {
				cout << "Please, enter positive numbers\n";
				continue;
			}
			if (numWorkers * 2 > numRows) {
				cout << "Number of workers should be not greater than half number of rows\n";
				continue;
			}
			if (state != state_t::BEFORE_START) {
				cout << "The system has already started\n";
				continue;
			}
			NUM_WS = numWorkers;
			randomCommand(numRows, numCols);
		}
		else if (command == "RUN") {
			int numIterations;
			cin >> numIterations;
			if (numIterations < 1) {
				cout << "Please, enter positive number of iterations\n";
				continue;
			}
			if (state == state_t::BEFORE_START) {
				cout << "The system didn't start, please use the command START\n";
				continue;
			}
			for (int i = 0; i < NUM_WS; ++i) {
				myArgs[i] = new args_t(numIterations,i);
			}
			sem_init(&iterationSem, 0, NUM_WS - 1);
			isReady.resize(NUM_WS, false);
			gameFinished = false;
			state = state_t::RUNNING;
			for (int i = 0; i < NUM_WS; ++i) {
				pthread_create(&threads[i], NULL, runParallel, myArgs[i]);
			}
		}
		else if (command == "TIMERUN") {
			int numIterations;
			cin >> numIterations;
			if (numIterations < 1) {
				cout << "Please, enter positive number of iterations\n";
				continue;
			}
			if (state == state_t::BEFORE_START) {
				cout << "The system didn't start, please use the command START\n";
				continue;
			}
			for (int i = 0; i < NUM_WS; ++i) {
				myArgs[i] = new args_t(numIterations,i);
			}
			sem_init(&iterationSem, 0, NUM_WS - 1);
			isReady.resize(NUM_WS, false);
			gameFinished = false;
			state = state_t::RUNNING;
			time_t startTimer;
			time_t endTimer;
			time(&startTimer);
			for (int i = 0; i < NUM_WS; ++i) {
				pthread_create(&threads[i], NULL, runParallel, myArgs[i]);
			}
			for (int i = 0; i < NUM_WS; ++i) {
				pthread_join(threads[i], NULL);
			}
			time(&endTimer);
			double seconds = difftime(endTimer, startTimer);
			cout << "\n" << numIterations << " iterations were completed in ";
			cout << seconds << " sec\n";
			state = state_t::STARTED;
		}
		else if (command == "STATUS") {
			if (state == state_t::BEFORE_START) {
				cout << "The system hasn't started, please use the command START\n";
				continue;
			}
			if (state == state_t::RUNNING) {
				cout << "The system is running know, please use the command STOP first\n";
				continue;
			}
			statusCommand();
		}
		else if (command == "STOP") {
			if (state != state_t::RUNNING) {
				cout << "The system isn't running. Please, use RUN or START\n";
				continue;
			}
			stopCommand();
		}
		else if (command == "WAIT") {
			if (state != state_t::RUNNING) {
				cout << "The system isn't running. Please, use RUN or START\n";
				continue;
			}
			for (int i = 0; i < NUM_WS; ++i) {
				pthread_join(threads[i], NULL);
			}
			state = state_t::STARTED;
			quitCommand();
			break;
		}
		else if (command == "QUIT") {
			quitCommand();
			break;
		}
		else if (command == "HELP") {
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
		} else {
			cout << "Wrong command. Type HELP for the list\n";
		}
	}
	return 0;
}
