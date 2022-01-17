#include "utils.h"

bool char_in_array(char ch, const string& ar)
{
   string::size_type p;
   bool found = false;
   p = 0;
   while (p<ar.size() && !found) {
      if (ch==ar[p]) found = true;
      p++;
   }
   return found;
}

bool is_space(char ch)
{
   return char_in_array(ch," \r\n\t");
}

bool is_sep(char ch)
{
   return char_in_array(ch,",()");
}

bool is_space_or_sep(char ch)
{
   return (is_space(ch) || is_sep(ch));
}

string strtrimleft(const string& str)
{
   size_t p=0;
   while (p<str.length() && is_space(str[p])) p++;
   return str.substr(p,str.length()-p);
}

string strtrimright(const string& str)
{
   size_t p=str.length()-1;
   while (p>=0 && is_space(str[p])) p--;
   return str.substr(0,p+1);
}

string strtrim(const string& str)
{
   string r = strtrimleft(str);
   r = strtrimright(r);
   return r;
}

bool file_read(const string& FileName, string& FileBody)
{
   ifstream ifs(FileName.c_str());
   FileBody.assign((std::istreambuf_iterator<char>(ifs)),(std::istreambuf_iterator<char>()));
   return true;
}

bool file_write(const string& FileName, const string& FileBody, bool append)
{
   ofstream ofs;
   ofs.open(FileName, append ? ios::out | ios::app : ios::out);
   ofs << FileBody;
   ofs.close();
   return true;
}

bool file_exists(const string& FileName)
{
   // not: it may return false when file is locked by another application. for example
   // in C++14 and C++17 there are ready functions like
   // std::experimental::filesystem::exists(filename); in C++14
   // std::filesystem::exists(filename); in C++17
   ifstream ifs(FileName);
   //return ifs.good();
   bool r = ifs.is_open();
   if (r) ifs.close();
   return r;
}

string tok(string& str, const string& sep)
{
   string r;
   size_t p = str.find(sep);
   if (p==string::npos) {
      r=str;
      str="";
   } else {
      r=str.substr(0,p);
      str.erase(0,p+sep.length());
   }
   return r;
}

string rtok(string& str, const string& sep)
{
   string r;
   size_t p = str.rfind(sep);
   if (p==string::npos) {
      r=str;
      str="";
   } else {
      r=str.substr(p+sep.length(),str.length()-(p-sep.length()+1));
      str.erase(p,str.length()-p);
   }
   return r;
}

string strreplace(const string& str, const string& oldstr, const string& newstr)
{
   string r=str;
   size_t p=0;
   size_t len=oldstr.length();
   size_t nlen=newstr.length();
   p=r.find(oldstr);
   while (p!=string::npos) {
      r.replace(p,len,newstr);
      p=r.find(oldstr,p+nlen);
   }
   return r;
}

int strreplacewithcount(string& str, const string& oldstr, const string& newstr)
{
   int c=0; // count
   string r=str;
   size_t p=0;
   size_t len=oldstr.length();
   size_t nlen=newstr.length();
   p=r.find(oldstr);
   while (p!=string::npos) {
      r.replace(p,len,newstr);
      p=r.find(oldstr,p+nlen);
      c++;
   }
   str=r;
   return c;
}

bool IsLeapYear(int year)
{
   return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// in current version month is passed in range: 1..12
int GetDaysInMonth(int year, int month)
{
   int days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
   int result = days[month-1];
   if (month==2 && IsLeapYear(year)) result++;
   return result;
}

int GetDaysInYear(int year)
{
   int result = 365;
   if (IsLeapYear(year)) result++;
   return result;
}
