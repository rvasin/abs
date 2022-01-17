#pragma once

#include <string>
#include <time.h>
#include <sys/time.h>
#include <ctime>
#include <unistd.h>
#include <stdio.h> // popen, pclose
#include <cmath>
#include <time.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include "utils.h"

using namespace std;

bool char_in_array(char ch, const string& ar);
bool is_space(char ch);
bool is_sep(char ch);
bool is_space_or_sep(char ch);
string strtrimleft(const string& str);
string strtrimright(const string& str);
string strtrim(const string& str);
bool file_read(const string& FileName, string& FileBody);
bool file_write(const string& FileName, const string& FileBody, bool append=false);
bool file_exists(const string& FileName);
string tok(string& str, const string& sep);
string rtok(string& str, const string& sep);
string strreplace(const string& str, const string& oldstr, const string& newstr);
int strreplacewithcount(string& str, const string& oldstr, const string& newstr);
bool IsLeapYear(int year);
int GetDaysInMonth(int year, int month);
int GetDaysInYear(int year);
