#include <string>
#include <iostream>

using namespace std;

int main() {
	string s ;
	while (cin >> s) {
		if(s.length() > 0 && (s[0] == ’r’ || s[0] == ’R’)) {
			cout << s << "\t " << 1 << "\n" ;
		}
	}
	return 0;
}