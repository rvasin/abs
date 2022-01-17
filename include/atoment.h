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

// as a first variant we define list here
enum TAtomType {atDouble,atInt,atString,atList};

class AtomEnt
{
protected:
   TAtomType AtomType;
   double DoubleValue;
   int IntValue;
   string StringValue;
   vector<AtomEnt *> *ListValue;
public:
   AtomEnt();
   ~AtomEnt();
   void SetAtomType(TAtomType AAtomType) { AtomType = AAtomType;};
   TAtomType GetAtomType() { return AtomType;};
   void SetDouble(double value) { DoubleValue = value;};
   double GetDouble();
   void SetInt(int value) { IntValue = value;};
   int GetInt() { return IntValue;};
   void SetString(const string& value) { StringValue = value;};
   string GetString() { return StringValue;};
   string ToString();
   void ClearListValue();
   void CreateListValue() { ListValue=new vector<AtomEnt *>;};
   void AddListElem(AtomEnt *elem) { ListValue->push_back(elem);};
   AtomEnt *GetListElem(int i) { return ListValue->at(i);};
   void SetListElem(int i, AtomEnt * elem);
   void RemoveListElem(int i);
   void InsertListElem(int i, AtomEnt *elem) { ListValue->insert(ListValue->begin()+i,elem);};
   int GetListSize() { return ListValue->size();};
   void PutListValue(vector<AtomEnt *> *&AListValue);
   void PrintValue();
   void Assign(AtomEnt *Atom);
   bool Equals(AtomEnt *Atom);
};

