//This is a version that assumes that the first element of an array is the length of the array
//Updated on April 24, 2025
//
#include "libsysy64.h"

long long getint() {
  long long t;
  scanf("%lld", &t);
  return t;
}

long long getch() {
  char c;
  scanf("%c", &c);
  return (long long)c;
}

long long getarray(long long a[]) {
  long long n;
  scanf("%lld", &n);
  a[0] = n;
  for (long long i = 0; i < n; i++) scanf("%lld", &a[i+1]);
  return n;
}

void putint(long long a) {
  printf("%lld", a);
}

void putch(long long a) {
  printf("%c", (int)a);
}

void putarray(long long n, long long a[]) {
  printf("%lld:", n);
  for (long long i = 0; i < n; i++) printf(" %lld", a[i+1]);
  printf("\n");
}

/* Timing function implementation */
__attribute((constructor)) void before_main() {
  for (int i = 0; i < _SYSY_N; i++)
    _sysy_h[i] = _sysy_m[i] = _sysy_s[i] = _sysy_us[i] = 0;
  _sysy_idx = 1;
}
__attribute((destructor)) void after_main() {
  for (int i = 1; i < _sysy_idx; i++) {
    fprintf(stderr, "Timer@%04d-%04d: %dH-%dM-%dS-%dus\n", \
        _sysy_l1[i], _sysy_l2[i], _sysy_h[i], _sysy_m[i], _sysy_s[i], _sysy_us[i]);
    _sysy_us[0] += _sysy_us[i];
    _sysy_s[0] += _sysy_s[i];
    _sysy_us[0] %= 1000000;
    _sysy_m[0] += _sysy_m[i];
    _sysy_s[0] %= 60;
    _sysy_h[0] += _sysy_h[i];
    _sysy_m[0] %= 60;
  }
  // fprintf(stderr, "TOTAL: %dH-%dM-%dS-%dus\n", _sysy_h[0], _sysy_m[0], _sysy_s[0], _sysy_us[0]);
}
void _sysy_starttime(int lineno) {
  _sysy_l1[_sysy_idx] = lineno;
  gettimeofday(&_sysy_start, NULL);
}
void _sysy_stoptime(int lineno) {
  gettimeofday(&_sysy_end, NULL);
  _sysy_l2[_sysy_idx] = lineno;
  _sysy_us[_sysy_idx] += 1000000 * ( _sysy_end.tv_sec - _sysy_start.tv_sec ) + _sysy_end.tv_usec - _sysy_start.tv_usec;
  _sysy_s[_sysy_idx] += _sysy_us[_sysy_idx] / 1000000 ;
  _sysy_us[_sysy_idx] %= 1000000;
  _sysy_m[_sysy_idx] += _sysy_s[_sysy_idx] / 60 ;
  _sysy_s[_sysy_idx] %= 60;
  _sysy_h[_sysy_idx] += _sysy_m[_sysy_idx] / 60 ;
  _sysy_m[_sysy_idx] %= 60;
  _sysy_idx ++;
}
void starttime() {_sysy_starttime(__LINE__);}
void stoptime() {_sysy_stoptime(__LINE__);}
