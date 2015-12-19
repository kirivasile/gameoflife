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

void masterRoutine(int size) {
	string command;
	for (int i = 0; i < 2; ++i) {		
		cin >> command;
		if (command == "START") {
			string fieldString;
			cin >> fieldString;
			if (state != state_t::BEFORE_START) {
				cout << "The system has already started\n";
				continue;
			}
			startCommand(fieldString);
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
		} else {
			cout << "Wrong command. Type HELP for the list\n";
		}
	}
}
