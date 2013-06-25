extern "C"
double pchar(double ascii) {
  cout << (char)ascii << flush;
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
