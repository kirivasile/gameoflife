#include <string>
#include <iostream>

using namespace std;


int main() {
	string s, ps = "";
	int v, pv;
	cin >> ps >> pv ;
	while ( cin >> s >> v ) {
	if(s == ps) {
		pv += v ;
	} else {
		cout << ps << "\t" << pv << "\n" ;
		pv = v ;
		ps = s ;
	}
	}
	cout << ps << "\t" << pv << "\n" ;
	return 0 ;
}
