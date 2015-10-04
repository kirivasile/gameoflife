#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <pthread.h>
#include <semaphore.h>
#include <cmath>
#include <ctime>

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

field_t field;
state_t state = state_t::BEFORE_START;
unsigned int NUM_WS; // Number of workers
vector<unsigned char> isReady;
vector<unsigned char> isFinishedReading;
vector<pthread_t> threads;
vector<args_t*> myArgs;
vector<cond_t> isReadyCV;
vector<cond_t> isFinishedReadingCV; //Когда все прочитали
vector<mutex_t> mutexCV;
vector<sem_t> iterationSems; //Семафоры, следящие за остановкой итераций
unsigned int stoppedIteration;
bool gameFinished;
mutex_t gameFinishedMutex;

void initializeStructures() {
	threads.resize(NUM_WS);
	myArgs.resize(NUM_WS);
	state = state_t::STARTED;
	isReadyCV = vector<cond_t>(NUM_WS, PTHREAD_COND_INITIALIZER);
	isFinishedReadingCV = vector<cond_t>(NUM_WS, PTHREAD_COND_INITIALIZER);
	mutexCV = vector<mutex_t>(NUM_WS, PTHREAD_MUTEX_INITIALIZER);
	isReady = vector<unsigned char>(NUM_WS, 2);
	isFinishedReading = vector<unsigned char>(NUM_WS, 0);
	iterationSems.resize(NUM_WS);
	for (int i = 0; i < NUM_WS; ++i) {
		sem_init(&iterationSems[i], 0, 1);
	}
	stoppedIteration = 0;
	gameFinished = false;
	gameFinishedMutex = PTHREAD_MUTEX_INITIALIZER;
}

void startCommand(const string &filePath) {
	initializeStructures();
	ifstream file;
	file.open(filePath);
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
	}
	file.close();
}

void randomCommand(const unsigned int &numRows, const unsigned int &numCols) {
	initializeStructures();
	vector<bool> buf(numCols, false);
	field.resize(numRows, buf);
	srand(time(0));
	for (int i = 0; i < numRows; ++i) {
		for (int j = 0; j < numCols; ++j) {
			field[i][j] = rand() % 2;
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
			cout << field[i][j] << " ";
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
	int upBorder = range * id, downBorder = min(range * (id + 1), x);
	if (downBorder - upBorder == 0) { //Когда рабочий будет тратить ресурсы впустую
		return NULL;
	}
	int down = (id + NUM_WS + 1) % NUM_WS, up = (id + NUM_WS - 1) % NUM_WS;
	for (int it = 0; it < numIterations; ++it) {
		//Подождать пока таблица готова к чтению
		pthread_mutex_lock(&mutexCV[id]);
		while(isReady[id] < 2) {
			pthread_cond_wait(&isReadyCV[id], &mutexCV[id]);
		}
		//Говорим, что информация в таблице - неактуальная
		isReady[id] = 0;
		pthread_mutex_unlock(&mutexCV[id]);
		vector<vector<bool> > bufField = field;
		vector<vector<bool> > bufWriteField = vector<vector<bool> >(downBorder - upBorder);
		for (int i = 0; i < downBorder - upBorder; ++i) {
			bufWriteField[i] = bufField[upBorder + i];
		}
		for (int i = upBorder; i < downBorder; ++i) {
			for (int j = 0; j < y; ++j) {
				int numNeighbors = 0;
				//Исправь на более адекватную систему (ВНИМАНИЕ!)
				if (bufField[(i + x - 1) % x][j]) {
					numNeighbors++;
				}
				if (bufField[(i + x - 1) % x][(j + y - 1) % y]) {
					numNeighbors++;
				}
				if (bufField[(i + x - 1) % x][(j + y + 1) % y]) {
					numNeighbors++;
				}
				if (bufField[(i + x + 1) % x][j]) {
					numNeighbors++;
				}
				if (bufField[(i + x + 1) % x][(j + y - 1) % y]) {
					numNeighbors++;
				}
				if (bufField[(i + x + 1) % x][(j + y + 1) % y]) {
					numNeighbors++;
				}
				if (bufField[i][(j + y - 1) % y]) {
					numNeighbors++;
				}
				if (bufField[i][(j + y + 1) % y]) {
					numNeighbors++;
				}
				if (!bufField[i][j] && numNeighbors == 3) {
					bufWriteField[i - upBorder][j] = true;
				}
				if (bufField[i][j] && (numNeighbors < 2 || numNeighbors > 3)) {
					bufWriteField[i - upBorder][j] = false;
				}
			}
		}
		//Перед тем, как закрывать доступ к чтению, нужно убедиться, что соседи прочитали
		pthread_mutex_lock(&mutexCV[id]);
		isFinishedReading[id] = 2;
		pthread_cond_broadcast(&isFinishedReadingCV[id]);
		pthread_mutex_unlock(&mutexCV[id]);

		pthread_mutex_lock(&mutexCV[down]);
		while (isFinishedReading[down] < 1) {
			pthread_cond_wait(&isFinishedReadingCV[down], &mutexCV[down]);
		}
		--isFinishedReading[down];
		pthread_mutex_unlock(&mutexCV[down]);

		pthread_mutex_lock(&mutexCV[up]);
		while (isFinishedReading[up] < 1) {
			pthread_cond_wait(&isFinishedReadingCV[up], &mutexCV[up]);
		}
		--isFinishedReading[up];
		pthread_mutex_unlock(&mutexCV[up]);

		for (int i = upBorder; i < downBorder; ++i) {
			field[i] = bufWriteField[i - upBorder];
		}
		//Посылаем сигнал, что данные обновились
		pthread_mutex_lock(&mutexCV[down]);
		++isReady[down];
		pthread_cond_broadcast(&isReadyCV[down]);
		pthread_mutex_unlock(&mutexCV[down]);
		
		pthread_mutex_lock(&mutexCV[up]);
		++isReady[up];
		pthread_cond_broadcast(&isReadyCV[up]);
		pthread_mutex_unlock(&mutexCV[up]);
		
		//Проверяем, не остановил ли нас master
		int lockStatus;
		lockStatus = sem_wait(&iterationSems[id]);
		if (lockStatus != 0) {
			printf("Pid: %d - sem_wait failed\n", id);
			return NULL;
		}
		lockStatus = sem_post(&iterationSems[id]);
		if (lockStatus != 0) {
			printf("Pid: %d - sem_post failed\n", id);
			return NULL;
		}
		//pthread_mutex_lock(&gameFinishedMutex);
		if (gameFinished) {
			if (id == 0) {
				stoppedIteration += it;
				printf("Stopped at iteration #%d\n", stoppedIteration);
			}
			isFinishedReading[id] = 2;
			//pthread_mutex_unlock(&gameFinishedMutex);
			return NULL;
		}
		//pthread_mutex_unlock(&gameFinishedMutex);
	}
	if (id == 0) {
		stoppedIteration += numIterations;
	}
	return NULL;
}

void stopCommand() {
	for (int i = 0; i < NUM_WS; ++i) {
		sem_wait(&iterationSems[i]);
	}
	gameFinished = true;
	for (int i = 0; i < NUM_WS; ++i) {
		sem_post(&iterationSems[i]);
	}
	for (int i = 0; i < NUM_WS; ++i) {
		pthread_join(threads[i], NULL);
	}
	state = state_t::STARTED;
}

void quitCommand() {
	for (int i = 0; i < NUM_WS; ++i) {
		sem_destroy(&iterationSems[i]);
		pthread_mutex_destroy(&mutexCV[i]);
	}
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
			startCommand(filePath);
		}
		else if (command == "RANDOM") {
			int numRows;
			int numCols;
			cin >> numRows >> numCols;
			if (numRows < 1 || numCols < 1) {
				cout << "Please, enter positive numbers\n";
				continue;
			}
			if (state != state_t::BEFORE_START) {
				cout << "The system has already started\n";
				continue;
			}
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
			gameFinished = false;
			state = state_t::RUNNING;
			for (int i = 0; i < NUM_WS; ++i) {
				pthread_create(&threads[i], NULL, runParallel, myArgs[i]);
			}

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
			stopCommand();
		}
		else if (command == "QUIT") {
			quitCommand();
			break;
		}
		else if (command == "HELP") {
			cout << "\nList of commands:\n";
			cout << "  START n file : n - number of workers, file - name of file with field\n";
			cout << "  RANDOM x y   : random field generation x - num of rows, y - num of cols\n";
			cout << "  STOP         : stop workers immedeately\n";
			cout << "  STATUS       : print current iteration and field. Use STOP before using it\n";
			cout << "  QUIT         : quit the game\n";
			cout << "  HELP         : short commands guide\n\n";
		} else {
			cout << "Wrong command. Type HELP for the list\n";
		}
	}
	return 0;
}