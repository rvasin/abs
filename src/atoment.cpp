#include "atoment.h"

AtomEnt::AtomEnt()
{
   AtomType = atString;
   DoubleValue = 0;
   IntValue = 0;
   StringValue = "";
   ListValue = nullptr;
}

void AtomEnt::ClearListValue()
{
   /*
   vector<AtomEnt *>::iterator it;
   for(it = ListValue.begin(); it != ListValue.end(); ++it) {
       delete it;
   }
   */
   // the working code to clear the list. but we don't call it in destructor for now:

   // to-do if apptype is atList:
   if (AtomType==atList) {
      for (size_t i=0; i<ListValue->size(); i++) delete ListValue->at(i);
      ListValue->clear();
      // to-do: verify two commands below because it hangs on example below (assign vector a to vector b)
      // while it works fine with commented code
      //delete ListValue;
      //ListValue = nullptr;
   }
   /* to-do an exception is here when program ends the code:
         set(a,list(10,20))
         set(b,a)
         print(b)
         print(a)
      this is probably because we empy b variable which destroys ListValue vector
      and then it tries to destroy a variable and it has such exception
   */
}

AtomEnt::~AtomEnt()
{
   // we don't call CleanListValue() in destructor. we call it manually when needed
}

double AtomEnt::GetDouble()
{
   if (AtomType==atInt) {
      return (double)IntValue;
   } else {
      return DoubleValue;
   }
   // to-do: should we also return double if atomtype is string?
   // it's easy to implement. but do we need it?
}

void AtomEnt::SetListElem(int i, AtomEnt * elem)
{
   // clear existing element
   ListValue->at(i)->ClearListValue();
   // set new value
   // it does not work this way:
   //ListValue[i]=elem;
   // it works only this way:
   ListValue->at(i)->Assign(elem);
}

void AtomEnt::RemoveListElem(int i)
{
   ListValue->at(i)->ClearListValue();
   ListValue->erase(ListValue->begin()+i);
}

void AtomEnt::PutListValue(vector<AtomEnt *> *&AListValue)
{
   AListValue=ListValue;
}

void AtomEnt::PrintValue()
{
   switch (AtomType) {
   case atDouble:
   cout << DoubleValue << endl;
   break;
   case atInt:
   cout << IntValue << endl;
   break;
   case atList:
   //cout << "()" << endl;
   cout << ToString() << endl;
   break;
   default:
   cout << StringValue << endl;
   break;
   }
}

string AtomEnt::ToString()
{
   string result="";
   switch (AtomType) {
   case atDouble: {
   // to_string() is function of C++11.
   //result=std::to_string(DoubleValue);
   stringstream ss;
   ss << DoubleValue;
   result = ss.str();
   break;
   }
   case atInt: {
   stringstream ss;
   ss << IntValue;
   result = ss.str();
   // in C++11 we have to do:
   //result=std::to_string(IntValue);
   break;
   }
   case atList: {
   result="(";
   for (size_t i=0; i<ListValue->size();i++) {
      if (i>0) result+=",";
      result+=ListValue->at(i)->ToString();
   }
   result+=")";
   break;
   }
   default:
   result=StringValue;
   break;
   }
   return result;
}

void AtomEnt::Assign(AtomEnt *Atom)
{
   AtomType = Atom->AtomType;
   DoubleValue = Atom->GetDouble();
   IntValue = Atom->GetInt();
   StringValue = Atom->GetString();
   Atom->PutListValue(ListValue);
}

bool AtomEnt::Equals(AtomEnt *Atom)
{
   bool r;
   if (GetAtomType()==atInt || GetAtomType()==atDouble) {
      if (GetDouble()==Atom->GetDouble()) r = true; else r=false;
   } else {
      if (GetString()==Atom->GetString()) r = true; else r=false;
   }
   return r;
}
