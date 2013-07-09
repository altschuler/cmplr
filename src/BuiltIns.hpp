#include <curses.h>

extern "C"
double pchar(double ascii) {
	cout << (char) ascii << flush;
	return 0;
}

extern "C"
double pdoub(double num) {
	cout << num << flush;
	return 0;
}

extern "C"
double pline() {
	cout << '\n';
	return 0;
}

extern "C"
double wait(double time) {
	usleep(time);
	return 0;
}

extern "C"
double clrscr() {
	cout << "\x1b[H\x1b[2J";
	return 0;
}
