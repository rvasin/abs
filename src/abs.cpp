#include "abs.h"

#include <time.h>
#include <sys/time.h>
#include <ctime>
#include <unistd.h>
#include <stdio.h> // popen, pclose
#include <cmath>
#include <time.h>
#include <algorithm>
#include <sstream>
#include "utils.h"

string funnames[] = {"add","mult","sub","div","mod","inc","dec","set","get","unset",
   "eq","noteq","and","or","not","less","lesseq","gr","greq","if","switch","while","do","print","input",
   "cmd","exit","sys","puts","cat","append","len","pos","copy","erase","insert","replace","treplace",
   "upper","lower","run","round","pow","sqrt","abs","floor","ceil","trunc","env","max",
   "min","sin","cos","tan","asin","acos","atan","sinh","cosh","tanh","asinh",
   "acosh","atanh","exp","log","log10","eval","include","includeonce","argc","argv","tok",
   "rtok","frac","sqr","char","rand","trim","trimleft","trimright","fileread","fileexists",
   "str","int","float","pi","e","sign","filewrite","fileappend","now","strdate",
   "year","month","day","hour","minute","second","week","dayofweek","dayofyear",
   "date","difftime","isleapyear","daysinyear","daysinmonth","format","printf","fun","var",
   "list","listsize","listget","listset","listappend","listremove","listinsert",
   "listcreate","listcopy"};

TreeNode::TreeNode()
{
   atom = new AtomEnt();
   VarName = "";
   CustFunName = "";
}

TreeNode::~TreeNode()
{
   delete atom;
}

string TreeNode::GetFunName()
{
   return funnames[(int)FunCode];
}

void TreeNode::ClearParams()
{
   params.clear();
}

ByteCode::ByteCode()
{
   // init random seed
   srand(time(NULL));

   debug=false;
   ShowRunTime = false;
   ShowTree = false;
   code = "";
   MainPos = 0;
   // init funcodes
   string funname;
   size_t funcount = sizeof(funnames)/sizeof(funnames[0]);
   for (size_t i=0; i<funcount; i++) {
      funname = funnames[i];
      funcodes[funname]=(TFunCodes)i;
   }
}

ByteCode::~ByteCode()
{
   // in current version we clear vars only in destructor
   // we may implement clear() command similar to Matlab which will clear all vars
   // for example, if may be useful in interactive mode: after some work we call clear()
   // and continue with clean "workspace" without functions definitions.
   map<string,AtomEnt *>::iterator it;
   for(it = vars.begin(); it != vars.end(); ++it) {
       /*
       this is one of points where we clear allocated in memory lists
       the second point should be in unset() function
       and also in place where we cleanup local variables

       technically in initial version of list support we may try to use lists only via vars
       thus such garbage collection will work fine. it will handle even multi-level lists
       it works fine even in this mode:
            abs> set(a,list(list(1,2),3))
            abs> print(listsize(a))
            2abs> set(b,listget(a,0))
            abs> print(b)
            (1,2)abs>
       the problem may be only when we define subtree variable b before declaration of variable a
       in this case here in cleanup it will first clear var b (hence it will delete branch of a)
       and then it will clean main tree a. but it's ok. it will not damage the destruction process.

       the only problem maybe with the cases when we create list as parameter of function like:
       print(listsize(list(10,20)))
       this case is not handled now. normally in listsize() and similar functions
       we need to analyze - if passed param is not var but it's atom list then we need to clean it at the end
       again: the problem maybe with cases like that:
            set(a,list(list(1,2),3))
            print(listget(a,0))
       in this case listget(0 will destroy branch list(1,2)

       */
       it->second->ClearListValue();
       delete it->second;
   }
   // if we call it in destructor then we don't need to call it. it's caller automatically by destructor
   vars.clear();
}

void ByteCode::SetArgList(int argc, char *argv[])
{
   for (int i=0; i<argc; i++) {
      arglist.push_back(argv[i]);
   }
}

TFunCodes ByteCode::GetFunCode(const string& funname)
{
   int code=-1;
   map<string,TFunCodes>::const_iterator ifind = funcodes.find(funname);
   if ( ifind != funcodes.end() ) {
      code = ifind->second;
   }
   return (TFunCodes)code; // it's not 100% correct. because if it's = -1 then this value is not in enum list
}

void ByteCode::skip_spaces()
{
   // comments may be only placed in the same place as spaces
   // for example, after , separator or before
   // but it cannot interrupt string in the middle, for example.

   // original skip spaces code:
   //while (is_space(code[MainPos])) MainPos++;

   // new code with comments removing
   bool skipped=true;
   while (MainPos<code.size() && skipped) {
      skipped=false;
      if (code[MainPos]=='#') {
         while (MainPos<code.size() && code[MainPos]!='\n') MainPos++;
         if (MainPos<code.size()) MainPos++; // skip \n
         skipped=true;
      } else if (is_space(code[MainPos])) {
         MainPos++;
         skipped=true;
      }
   }
}

void ByteCode::ParseTreeNode(TreeNode * & anode)
{
   TreeNode *node = anode;

   bool IsFirstNode=true;
   bool HaveNode;
   do {

   AtomEnt *atom;

   if (debug) cout << "begin parsing. MainPos: " << MainPos << endl;

   // skip all spaces
   skip_spaces();

   if (debug) cout << "after skip spaces. MainPos: " << MainPos << endl;

   // now do parsing of values
   if (char_in_array(code[MainPos],"-0123456789")) {
      // number may be negative. thus also parse - as a first char
      // to-do: also parse decimal separator .
      // will it be possible to use doubles like .12 or should it be recorded as 0.12 ?

      // load number from string
      string st="";
      while (MainPos<code.size() && !is_space_or_sep(code[MainPos])) {
         st+=code[MainPos];
         MainPos++;
      }
      atom = node->GetAtom();
      node->SetNodeType(ntAtom);
      //  if there is . in st then it's double:
      // to-do: maybe use standard C++ function to locate char in string instead of char_in_array?
      if (char_in_array('.',st)) {
         // covert string to double, C++ way:
         // http://www.cplusplus.com/reference/string/stod/
         // stod is only from version C++11
         // C way:
         // http://www.cplusplus.com/reference/cstdlib/atof/
         //atom->SetDouble(std::stod(st));
         atom->SetAtomType(atDouble);
         atom->SetDouble(atof(st.c_str()));
         if (debug) cout << "parsed double:" << endl;
         if (debug) atom->PrintValue();
      } else {
         // otherwise it's integer and use:
         atom->SetAtomType(atInt);
         atom->SetInt(atoi(st.c_str()));
         if (debug) cout << "parsed int:" << endl;
         if (debug) atom->PrintValue();
      }
   } else if (code[MainPos]=='\"') {
      atom = node->GetAtom();
      node->SetNodeType(ntAtom);
      atom->SetAtomType(atString);
      MainPos++; // skip starting "
      string st="";
      // to-do: do escape or special chars like \" \n \r and \\ (double back slash)
      while (MainPos<code.size() && code[MainPos]!='\"') {
         if (code[MainPos]=='\\') {
            MainPos++;
            if (code[MainPos]=='"') {
               st+='"';
            } else if (code[MainPos]=='r') {
               st+="\r";
            } else if (code[MainPos]=='n') {
               st+="\n";
            } else if (code[MainPos]=='t') {
               st+="\t";
            } else if (code[MainPos]=='\\') {
               st+="\\";
            }
         } else {
            st+=code[MainPos];
         }
         MainPos++;
      }
      atom->SetString(st);
      MainPos++; // skip ending "
      if (debug) cout << "parsed string:" << endl;
      if (debug) atom->PrintValue();
   } else {
      // the rest is function's name or variable
      // for first version support we don't have identifiers, we will only support function names.
      string st="";
      while (MainPos<code.size() && !is_space_or_sep(code[MainPos]) ) {
         st+=code[MainPos];
         MainPos++;
      }
      node->SetNodeType(ntVar);
      if (MainPos<code.size() && code[MainPos]=='(') {
         node->SetNodeType(ntFun);
      }
      if (node->GetNodeType()==ntVar) {
         if (debug) cout << "parsed variable:" << endl;
         if (debug) cout << st << endl;
         if (debug) cout << "MainPos: " << MainPos << endl;
         node->SetVarName(st);
      } else {
         if (debug) cout << "parsed function:" << endl;
         if (debug) cout << st << endl;
         if (debug) cout << "MainPos: " << MainPos << endl;
         MainPos++;
         TFunCodes FunCode = GetFunCode(st);
         node->SetFunCode(FunCode);
         if (FunCode==-1) {
            // this is a custom function
            node->SetCustFunName(st);
         }
         int i = 0;
         while (MainPos<code.size() && code[MainPos]!=')') {
            TreeNode *param1 = new TreeNode();
            ParseTreeNode(param1);
            node->AddParam(param1);
            /*
            // we dont need to do it here
            if (i==0 && FunCode==-1) {
               // this is a custom function
               // in custom function the first parameter is function name
               // remeber location of this function (location of functin's node)
               custcodes[param1->GetVarName()]=node;
            }
            */
            i++;
            skip_spaces();
            if (MainPos<code.size() && code[MainPos]!=')') MainPos++; // this is , param separator
         }
         // to-do: check if the function was passed without params like cmd()

         // function's name found. now evaluate each parameter
         //evalfun(st,atom);

         skip_spaces();
         MainPos++; // skip function's )

      }
   }
   skip_spaces();
   if (MainPos<code.size() && !is_sep(code[MainPos])) {
      // we have one more node
      HaveNode=true;
      if (IsFirstNode) {
         IsFirstNode = false;
         TreeNode *cmd_node = new TreeNode();
         cmd_node->SetNodeType(ntFun);
         cmd_node->SetFunCode(f_cmd);
         cmd_node->AddParam(node);
         node = new TreeNode();
         anode = cmd_node;
      } else {
         anode->AddParam(node);
         node = new TreeNode();
      }
   } else {
      HaveNode=false;
      if (!IsFirstNode) anode->AddParam(node);
   }

   } while (HaveNode);
}

#define fun_double_one(fcode,oper)          \
   case fcode: {                            \
      paramnode1 = node->GetParam(0);       \
      EvalTreeNode(paramnode1,param1,locvars);      \
      double r=oper(param1->GetDouble());   \
      atom->SetAtomType(atDouble);          \
      atom->SetDouble(r);                   \
      break;                                \
   }

#define fun_minmax(fcode,cond)                                    \
   case fcode: {                                                  \
      paramnode1 = node->GetParam(0);                             \
      EvalTreeNode(paramnode1,param1,locvars);                            \
      if (param1->GetAtomType()==atInt) {                         \
         int r = param1->GetInt();                                \
         for (int i=1; i<node->GetParamCount(); i++) {            \
            paramnode1 = node->GetParam(i);                       \
            EvalTreeNode(paramnode1,param1,locvars);                      \
            if (param1->GetInt() cond r) r=param1->GetInt();      \
         }                                                        \
         atom->SetAtomType(atInt);                                \
         atom->SetInt(r);                                         \
      } else {                                                    \
         double r = param1->GetDouble();                          \
         for (int i=1; i<node->GetParamCount(); i++) {            \
            paramnode1 = node->GetParam(i);                       \
            EvalTreeNode(paramnode1,param1,locvars);                      \
            if (param1->GetInt() cond r) r=param1->GetDouble();   \
         }                                                        \
         atom->SetAtomType(atDouble);                             \
         atom->SetDouble(r);                                      \
      }                                                           \
      break;                                                      \
   }

#define fun_incdec(fcode,oper)                                    \
   case fcode: {                                                  \
      paramnode1 = node->GetParam(0);                             \
      string VarName;                                             \
      GetVarName(paramnode1,VarName,locvars);                     \
      int step = 1;                                               \
      if (node->GetParamCount()==2) {                             \
         paramnode2 = node->GetParam(1);                          \
         EvalTreeNode(paramnode2,param2,locvars);                 \
         step=param2->GetInt();                                   \
      }                                                           \
      map<string,AtomEnt *>::const_iterator ifind;                \
      bool found = false;                                         \
      AtomEnt *var;                                               \
      if (locvars) {                                              \
         ifind = locvars->find(VarName);                          \
         if ( ifind != locvars->end() ) {                         \
            var = ifind->second;                                  \
            found = true;                                         \
         }                                                        \
      }                                                           \
      if (!found) {                                               \
         ifind= vars.find(VarName);                               \
         if ( ifind != vars.end() ) {                             \
            var = ifind->second;                                  \
            found = true;                                         \
         }                                                        \
      }                                                           \
      if ( found ) {                                              \
         var->SetInt(var->GetInt() oper step);                    \
         atom->Assign(var);                                       \
      } else {                                                    \
         atom->SetAtomType(atString);                             \
         atom->SetString("error: variable not found");            \
      }                                                           \
      break;                                                      \
   }

// to-do: compare must also work for strings
#define fun_comp(fcode,oper)                                      \
   case fcode: {                                                 \
      paramnode1 = node->GetParam(0);                             \
      paramnode2 = node->GetParam(1);                             \
      EvalTreeNode(paramnode1,param1,locvars);                            \
      EvalTreeNode(paramnode2,param2,locvars);                            \
      double r;                                                   \
      if (param1->GetDouble() oper param2->GetDouble()) r = 1; else r=0; \
      atom->SetAtomType(atDouble);                                \
      atom->SetDouble(r);                                         \
      break;                                                      \
   }

// that is not optimal way of updating string.
// normally we should work directly on string located in param1
#define fun_tok(fcode,oper)                                       \
   case fcode: {                                                  \
      paramnode1 = node->GetParam(0);                             \
      string VarName;                                             \
      GetVarName(paramnode1,VarName,locvars);                             \
      GetVar(VarName,param1,locvars);                                     \
      string VarValue=param1->GetString();                        \
      paramnode2 = node->GetParam(1);                             \
      EvalTreeNode(paramnode2,param2,locvars);                            \
                                                                  \
      string r=oper(VarValue,param2->GetString());                \
                                                                  \
      param1->SetString(VarValue);                                \
      SetVar(VarName,param1);                                     \
                                                                  \
      atom->SetAtomType(atString);                                \
      atom->SetString(r);                                         \
      break;                                                      \
   }

#define fun_str(fcode,oper)                                       \
   case fcode: {                                                  \
      paramnode1 = node->GetParam(0);                             \
      EvalTreeNode(paramnode1,param1,locvars);                            \
      string r = oper(param1->GetString());                       \
      atom->SetAtomType(atString);                                \
      atom->SetString(r);                                         \
      break;                                                      \
   }

#define fun_datepart(fcode,oper)                                  \
   case fcode: {                                                  \
      paramnode1 = node->GetParam(0);                             \
      EvalTreeNode(paramnode1,param1,locvars);                            \
      time_t t = (time_t)param1->GetInt();                        \
      struct tm *tst = localtime(&t);                             \
      atom->SetAtomType(atInt);                                   \
      atom->SetInt(tst->oper);                                    \
      break;                                                      \
   }

bool ByteCode::GetVar(string VarName, AtomEnt *atom, map<string,AtomEnt *> * locvars)
{
   bool found = false;
   map<string,AtomEnt *>::const_iterator ifind;
   if (locvars) {
      ifind = locvars->find(VarName);
      if ( ifind != locvars->end() ) {
         AtomEnt *var = ifind->second;
         atom->Assign(var);
         found = true;
      }
   }
   if (!found) {
      ifind = vars.find(VarName);
      if ( ifind != vars.end() ) {
         AtomEnt *var = ifind->second;
         if (debug) cout << "found var with value: " << endl;
         if (debug) var->PrintValue();
         atom->Assign(var);
         found = true;
      } else {
         atom->SetAtomType(atString);
         string msg = "error: variable ";
         msg+=VarName;
         msg+=" not found";
         if (debug) cout << msg << endl;
         atom->SetString(msg);
      }
   }
   return found;
}

bool ByteCode::SetVar(string VarName, AtomEnt *atom, map<string,AtomEnt *> * locvars)
{
   AtomEnt *var;
   map<string,AtomEnt *>::const_iterator ifind;
   bool localfound = false;
   if (locvars) {
      ifind = locvars->find(VarName);
      if ( ifind != locvars->end() ) {
         // variable is found
         var = ifind->second;
         localfound = true;
      }
   }
   if (!localfound) {
      ifind = vars.find(VarName);
      if ( ifind != vars.end() ) {
         // variable is found
         var = ifind->second;
      } else {
         // if variable not found in local and global lists
         // then declare it as a new global variable
         if (debug) cout << "setting global var" << endl;
         var = new AtomEnt();
         vars[VarName] = var;
      }
   }
   var->Assign(atom);
   return true;
}

bool ByteCode::GetVarName(TreeNode *node, string& VarName, map<string,AtomEnt *> * locvars)
{
   if (node->GetNodeType()==ntVar) {
      VarName = node->GetVarName();
   } else {
      // this is the case when setting or getting of variable
      // is done via string
      // for example as:
      // get("x")
      // or set("x",1)
      // or even complex functions calls, for example, we may simulate array with indexes as
      // set(cat("x",i),1) and i may be functions local variable so it's like setting
      // set(x1,1)   set(x2,1) if we are doing it in a loop
      AtomEnt *atom = new AtomEnt();
      EvalTreeNode(node,atom,locvars);
      // to-do: verify if atom is not string then we need to return error
      VarName = atom->GetString();
      delete atom;
   }
   return true;
}

void ByteCode::PrintTreeNode(TreeNode *node, string align)
{
   cout << align;
   if (node->GetNodeType()==ntAtom) {
      cout << "atom: ";
      node->GetAtom()->PrintValue();
   } else if (node->GetNodeType()==ntVar) {
      cout << "var: " << node->GetVarName() << endl;
   } else {
      cout << "function: " << node->GetFunName() << endl;
      for (int i=0; i<node->GetParamCount(); i++) {
         PrintTreeNode(node->GetParam(i),align+"   ");
      }
   }
}

void ByteCode::ClearLocalVars(map<string,AtomEnt *> * locvars)
{
   map<string,AtomEnt *>::iterator it;
   for(it = locvars->begin(); it != locvars->end(); ++it) {
       it->second->ClearListValue();
       delete it->second;
   }
}

void ByteCode::EvalTreeNode(TreeNode *node, AtomEnt *atom, map<string,AtomEnt *> * locvars)
{
   if (node->GetNodeType()==ntAtom) {
      if (debug) cout << "eval atom" << endl;
      atom->Assign(node->GetAtom());
   } else if (node->GetNodeType()==ntVar) {
      if (debug) cout << "eval var: " << node->GetVarName() << endl;
      GetVar(node->GetVarName(),atom,locvars);
   } else {
   if (debug) cout << "eval function: " << node->GetFunName() << endl;
      // this if function
   // we need to evaluate each parameter until we find )
   // then we need to pass the parameter to function
   // then we need to call the function
   // then we need to save value returned by function to atom

   // in current version we will have max 2 params.
   AtomEnt *param1 = new AtomEnt();
   AtomEnt *param2 = new AtomEnt();
   TreeNode *paramnode1;
   TreeNode *paramnode2;
   TFunCodes FunCode = node->GetFunCode();
   switch (FunCode) {
   // using {} after each case because in this case each variable with the same name like r is defined in its own scope of case
   // to-do: verify should we put break out of {}
   case f_add: {
      // code for add() and mult() is almost the same. only initial conditions and operation are different
      // do we need to refactor it into one code?
      double dr=0;
      int ir=0;
      bool IsInt = true;
      for (int i=0; i<node->GetParamCount(); i++) {
         paramnode1 = node->GetParam(i);
         EvalTreeNode(paramnode1,param1,locvars);
         if (param1->GetAtomType()==atDouble || !IsInt) {
            if (IsInt) {
               dr=ir;
               IsInt=false;
            }
            dr+=param1->GetDouble(); // even it param is int it GetDouble will convert it to double.
         } else ir+=param1->GetInt();
      }
      if (IsInt) {
         atom->SetAtomType(atInt);
         atom->SetInt(ir);
      } else {
         atom->SetAtomType(atDouble);
         atom->SetDouble(dr);
      }
      break;
   }
   case f_mult: {
      double dr=1;
      int ir=1;
      bool IsInt = true;
      for (int i=0; i<node->GetParamCount(); i++) {
         paramnode1 = node->GetParam(i);
         EvalTreeNode(paramnode1,param1,locvars);
         if (param1->GetAtomType()==atDouble || !IsInt) {
            if (IsInt) {
               dr=ir;
               IsInt=false;
            }
            dr*=param1->GetDouble(); // even it param is int it GetDouble will convert it to double.
         } else ir*=param1->GetInt();
      }
      if (IsInt) {
         atom->SetAtomType(atInt);
         atom->SetInt(ir);
      } else {
         atom->SetAtomType(atDouble);
         atom->SetDouble(dr);
      }
      break;
   }
   case f_sub: {
      paramnode1 = node->GetParam(0);
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode1,param1,locvars);
      EvalTreeNode(paramnode2,param2,locvars);
      if (param1->GetAtomType()==atInt && param2->GetAtomType()==atInt) {
         atom->SetAtomType(atInt);
         atom->SetInt(param1->GetInt()-param2->GetInt());
      } else {
         atom->SetAtomType(atDouble);
         atom->SetDouble(param1->GetDouble()-param2->GetDouble());
      }
      break;
   }
   case f_div: {
      paramnode1 = node->GetParam(0);
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode1,param1,locvars);
      EvalTreeNode(paramnode2,param2,locvars);
      if (param1->GetAtomType()==atInt && param2->GetAtomType()==atInt) {
         atom->SetAtomType(atInt);
         atom->SetInt(param1->GetInt()/param2->GetInt());
      } else {
         atom->SetAtomType(atDouble);
         if (debug) cout << "param1: " << param1->GetDouble() << endl;
         if (debug) cout << "param2: " << param2->GetDouble() << endl;
         atom->SetDouble(param1->GetDouble()/param2->GetDouble());
      }
      break;
   }
   case f_mod: {
      // get remainder of division.
      paramnode1 = node->GetParam(0);
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode1,param1,locvars);
      EvalTreeNode(paramnode2,param2,locvars);
      atom->SetAtomType(atInt);
      atom->SetInt(param1->GetInt() % param2->GetInt());
      break;
   }
   fun_incdec(f_inc,+)
   fun_incdec(f_dec,-)
   case f_set: {
      // set variable
      paramnode1 = node->GetParam(0);
      string VarName;
      GetVarName(paramnode1,VarName,locvars);

      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode2,param2,locvars);
      if (debug) cout << "set variable: " << VarName << endl;
      if (debug) cout << "value: ";
      if (debug) param2->PrintValue();
      SetVar(VarName,param2,locvars);
      break;
   }
   case f_get: {
      // get variable
      paramnode1 = node->GetParam(0);
      string VarName;
      GetVarName(paramnode1,VarName,locvars);
      if (debug) cout << "get variable:" << VarName << endl;

      GetVar(VarName,atom,locvars);

      if (debug) cout << "got value:" << endl;
      if (debug) atom->PrintValue();
      break;
   }
   case f_unset:
      // unset("varname")  - remove variable definition
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      // to-do: implement it
      // to-do: call ClearListValue()
      // returns: 0 if unset was successful (variable was found) and 1 otherwise.
      atom->SetAtomType(atInt);
      atom->SetInt(0);
      break;
   case f_eq: case f_noteq: {
      paramnode1 = node->GetParam(0);
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode1,param1,locvars);
      EvalTreeNode(paramnode2,param2,locvars);
      double r;
      // to-do: eq must also work for int and double. so analyze atom type here
      bool cond = param1->Equals(param2);
      if (FunCode==f_noteq) cond = !cond;
      if (cond) r = 1; else r=0;
      atom->SetAtomType(atDouble);
      atom->SetDouble(r);
      break;
   }
   case f_and: {
      /*
      special situations: if function runs without parameters and() it returns true
      if it runs with one parameter and(p) then it returns true if param is true
      */
      paramnode1 = node->GetParam(0);
      //paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode1,param1,locvars);
      double r=1;
      for (int i=0; i<node->GetParamCount(); i++) {
         paramnode1 = node->GetParam(i);
         EvalTreeNode(paramnode1,param1,locvars);
         if (param1->GetDouble()==0) {
            r=0;
            break;
         }
      }
      atom->SetAtomType(atDouble);
      atom->SetDouble(r);
      break;
   }
   case f_or: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      double r=0;
      for (int i=0; i<node->GetParamCount(); i++) {
         paramnode1 = node->GetParam(i);
         EvalTreeNode(paramnode1,param1,locvars);
         if (param1->GetDouble()!=0) {
            r=1;
            break;
         }
      }
      atom->SetAtomType(atDouble);
      atom->SetDouble(r);
      break;
   }
   case f_not: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      double r;
      if (param1->GetDouble()==0) r = 1; else r=0;
      atom->SetAtomType(atDouble);
      atom->SetDouble(r);
      break;
   }
   fun_comp(f_less,<)
   fun_comp(f_lesseq,<)
   fun_comp(f_gr,>)
   fun_comp(f_greq,>=)
   case f_if: {
      // to-do: implement multiple conditions and actions
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      double r;
      if (param1->GetDouble()!=0) {
         paramnode2 = node->GetParam(1);
         EvalTreeNode(paramnode2,param2,locvars);
         r = 1;
      } else {
         if (node->GetParamCount()==3) {
            paramnode2 = node->GetParam(2);
            EvalTreeNode(paramnode2,param2,locvars);
         }
         r=0;
      }
      // the very good question what should return if function - a number
      // or a result of evaluation of the working condition
      // may be in this case it must return param2 as result?
      // in C++/C there is (condition ? value1 : value2)
      // thus probably we need to simply implement similar construct
      // hence if will return numbers, but function with another name
      // for example: cond will return param2 as coded here
      // the same makes sense for switch() and other branching functions

      // to-do: probably set the result as int
      atom->SetAtomType(atDouble);
      atom->SetDouble(r);
      break;
   }
   case f_switch: {
      // to-do: implement multiple conditions and actions
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      double r=0;
      int action_index=0;
      int pcount=node->GetParamCount();
      for (int i=1; i<pcount; i+=2) {
         paramnode2 = node->GetParam(i);
         AtomEnt *cond_param = new AtomEnt(); // should we allocate it each time? or maybe allocate it once before for loop?
         EvalTreeNode(paramnode2,cond_param,locvars);
         if (param1->Equals(cond_param)) {
            action_index = i+1;
            break;
         }
         delete cond_param;
      }
      if (action_index==0 && pcount / 2 == 0) action_index = pcount-1;
      if (action_index>0) {
         paramnode2 = node->GetParam(action_index);
         EvalTreeNode(paramnode2,param2,locvars);
      }
      atom->SetAtomType(atDouble);
      atom->SetDouble(r);
      break;
   }
   case f_while: {
      // first param is condition, second param is body
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      while (param1->GetDouble()!=0) {
         paramnode2 = node->GetParam(1); // should we take ref to loop body each time here? or just get it once before while?
         EvalTreeNode(paramnode2,param2,locvars); // should we allocate param2 each time as in switch or use existing instance?
         EvalTreeNode(paramnode1,param1,locvars);
         if (debug) cout << "condition value:" << endl;
         if (debug) param1->PrintValue();
      }
      // what to return? (0 in case of at least one run? and 1 otherwise?)
      atom->SetAtomType(atInt);
      atom->SetInt(0);
      break;
   }
   case f_do: {
      // first param is body, second param is condition
      // it works the same way as do/while loop in C/C++
      // and not as repeat/until loop in Pascal
      do {
         // should we get refs to paranode1 and paramnode2 each time in do()? most prob do it only once before do()
         paramnode1 = node->GetParam(0);
         EvalTreeNode(paramnode1,param1,locvars);
         paramnode2 = node->GetParam(1);
         EvalTreeNode(paramnode2,param2,locvars);
      } while (param2->GetDouble()!=0);
      // what to return? (0 in case of at least one run? and 1 otherwise?)
      atom->SetAtomType(atInt);
      atom->SetInt(0);
      break;
   }
   case f_print: {
      // to-do: support of printing of several params
      // to-do: print should not all \r\n at the end
      // for that purpose we should implement println function (while it's not good idea
      // because on Linux it must be used with \n and on Windows it must be used \r\n
      // so probably it's best to use print function without adding line separator and that's all
      if (debug) cout << "printing:" << endl;
      for (int i=0; i<node->GetParamCount(); i++) {
         paramnode1 = node->GetParam(i);
         EvalTreeNode(paramnode1,param1,locvars);

         switch (param1->GetAtomType()) {
         case atDouble:
            cout << param1->GetDouble();
            break;
         case atInt:
            cout << param1->GetInt();
            break;
         case atList:
            cout << param1->ToString();
            break;
         default:
            cout << param1->GetString();
            break;
         }
      }
      // to-do: what should it return if we are printing double? should it return double?
      // so we may use:
      //atom->Assign(param1);
      atom->SetAtomType(atString);
      atom->SetString(param1->GetString());
      break;
   }
   case f_puts: {
      for (int i=0; i<node->GetParamCount(); i++) {
         paramnode1 = node->GetParam(i);
         EvalTreeNode(paramnode1,param1,locvars);
         StringBuffer += param1->ToString();
      }
      atom->SetAtomType(atString);
      // do we need to return string here?
      atom->SetString(param1->GetString());
      break;
   }
   case f_cat: {
      string r="";
      for (int i=0; i<node->GetParamCount(); i++) {
         paramnode1 = node->GetParam(i);
         EvalTreeNode(paramnode1,param1,locvars);
         // should cat() work only on strings? in this case we need to use param1->GetString();
         // in current realization it may accept strings or numbers
         r+=param1->ToString();
      }
      atom->SetAtomType(atString);
      atom->SetString(r);
      break;
   }
   case f_append: {
      paramnode1 = node->GetParam(0);
      string VarName;
      GetVarName(paramnode1,VarName,locvars);
      GetVar(VarName,param1,locvars);
      string VarValue=param1->GetString();

      // the code is similar to cat()
      string r="";
      for (int i=1; i<node->GetParamCount(); i++) {
         paramnode2 = node->GetParam(i);
         EvalTreeNode(paramnode2,param2,locvars);
         // in current realization it may accept strings or numbers
         r+=param2->ToString();
      }
      VarValue+=r;

      param1->SetString(VarValue);
      SetVar(VarName,param1);
      // what should be return: the resulting string or 0 in case of success?
      atom->SetAtomType(atInt);
      atom->SetInt(0);
      break;
   }
   case f_len: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      int r=param1->GetString().length();
      atom->SetAtomType(atInt);
      atom->SetInt(r);
      break;
   }
   case f_pos: {
      // we may call this function find() in C++ way
      // string
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      // substring
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode2,param2,locvars);
      // position
      int pos=0;
      if (node->GetParamCount()==3) {
         TreeNode *paramnode3 = node->GetParam(2);
         AtomEnt *param3 = new AtomEnt();
         EvalTreeNode(paramnode3,param3,locvars);
         pos = param3->GetInt();
         delete param3;
      }
      int r=param1->GetString().find(param2->GetString(),pos);
      atom->SetAtomType(atInt);
      atom->SetInt(r);
      break;
   }
   case f_copy: {
      // we may call this function substr() in C++ way
      // string
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      // pos
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode2,param2,locvars);
      // len
      TreeNode *paramnode3 = node->GetParam(2);
      AtomEnt *param3 = new AtomEnt();
      EvalTreeNode(paramnode3,param3,locvars);
      string r=param1->GetString().substr(param2->GetInt(),param3->GetInt());
      delete param3;
      atom->SetAtomType(atString);
      atom->SetString(r);
      break;
   }
   case f_erase: {
      // string
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      // pos
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode2,param2,locvars);
      // len
      TreeNode *paramnode3 = node->GetParam(2);
      AtomEnt *param3 = new AtomEnt();
      EvalTreeNode(paramnode3,param3,locvars);
      // it's implemented in C++ way. so the function returns modified string
      // while we may also implement it as a new function in pascal way - delete function
      // where the first argument will be name of variable of string.
      // so based on this variable the string will be modified
      string r=param1->GetString().erase(param2->GetInt(),param3->GetInt());
      delete param3;
      atom->SetAtomType(atString);
      atom->SetString(r);
      break;
   }
   case f_insert: {
      // to-do: it's implemented in C++ way. probably second param should be substring
      // and 3rd should be position
      // string
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      // position
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode2,param2,locvars);
      // substring
      TreeNode *paramnode3 = node->GetParam(2);
      AtomEnt *param3 = new AtomEnt();
      EvalTreeNode(paramnode3,param3,locvars);
      string r=param1->GetString();
      r.insert(param2->GetInt(),param3->GetString());
      delete param3;
      atom->SetAtomType(atString);
      atom->SetString(r);
      break;
   }
   case f_replace: {
      // probably implement also another variant of replace function which will
      // use first param as variable of string. then it will modify that variable
      // and it will return number of replacements done
      // so we may even implement it in this function
      // we need to analyze the first param. if it's a variable then work as described above
      // if it's a string then use the variant implemented below
      // but how about the situation when we really need to add the first parameter as a variable
      // but we don't want to replace in the variable storage string
      // there are two solutions: we may use str(varname) it will create a copy of string
      // so we call the function as replace(str(varname),"old","new")
      // in this case the first parameter will be the string
      // or we need to use function with another name like replacevar(varname,"old","new")

      // string
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      // old string
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode2,param2,locvars);
      // new string
      TreeNode *paramnode3 = node->GetParam(2);
      AtomEnt *param3 = new AtomEnt();
      EvalTreeNode(paramnode3,param3,locvars);
      string r=strreplace(param1->GetString(),param2->GetString(),param3->GetString());
      delete param3;
      atom->SetAtomType(atString);
      atom->SetString(r);
      break;
   }
   case f_treplace: {
      // this is another variant of replace function which will
      // use first param as variable of string. then it will modify that variable
      // and it will return number of replacements done
      // so we may even implement it in this function
      // we need to analyze the first param. if it's a variable then work as described above
      // if it's a string then use the variant implemented below
      // but how about the situation when we really need to add the first parameter as a variable
      // but we don't want to replace in the variable storage string
      // there are two solutions: we may use str(varname) it will create a copy of string
      // so we call the function as replace(str(varname),"old","new")
      // in this case the first parameter will be the string
      // or we need to use function with another name like replacevar(varname,"old","new")

      // string
      paramnode1 = node->GetParam(0);
      string VarName;
      GetVarName(paramnode1,VarName,locvars);
      GetVar(VarName,param1,locvars);
      string VarValue=param1->GetString();
      // old string
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode2,param2,locvars);
      // new string
      TreeNode *paramnode3 = node->GetParam(2);
      AtomEnt *param3 = new AtomEnt();
      EvalTreeNode(paramnode3,param3,locvars);
      int r=strreplacewithcount(VarValue,param2->GetString(),param3->GetString());
      param1->SetString(VarValue);
      SetVar(VarName,param1);
      delete param3;
      atom->SetAtomType(atInt);
      atom->SetInt(r);
      break;
   }
   case f_upper: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      string r=param1->GetString();
      //std::transform(r.begin(), r.end(),r.begin(), ::toupper); // C++11
      for (auto & c: r) c = toupper(c);
      atom->SetAtomType(atString);
      atom->SetString(r);
      break;
   }
   case f_lower: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      string r=param1->GetString();
      //std::transform(r.begin(), r.end(),r.begin(), ::tolower); // C++11
      for (auto & c: r) c = tolower(c);
      atom->SetAtomType(atString);
      atom->SetString(r);
      break;
   }
   case f_run: {
      // to-do: may be it's a temp name. maybe we will replace it into more meanful name
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      string s=param1->GetString();
      FILE *fp;
      char line[1035];
      string r="";
      // https://stackoverflow.com/questions/22166633/mingw-function-not-found-when-compiled-with-std-c11
      // popen function not found when compiled with -std=c++11
      // so as a solution we will use -std=gnu++11 option instead of -std=c++11 option
      fp = popen(s.c_str(), "r");
      if (fp != NULL) {
         while (fgets(line, sizeof(line)-1, fp) != NULL) {
            //printf("%s", line);
            r+=string(line);
            //r+="\n";
         }
         pclose(fp);
      }
      atom->SetAtomType(atString);
      atom->SetString(r);
      break;
   }
   fun_double_one(f_round,round)
   case f_pow: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode2,param2,locvars);
      double r=pow(param1->GetDouble(),param2->GetDouble());
      atom->SetAtomType(atDouble);
      atom->SetDouble(r);
      break;
   }
   fun_double_one(f_sqrt,sqrt)
   fun_double_one(f_abs,abs)
   fun_double_one(f_floor,floor)
   fun_double_one(f_ceil,ceil)
   fun_double_one(f_trunc,trunc)
   case f_env: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      string r="";
      char *cr = getenv(param1->GetString().c_str());
      if (cr!=NULL) {
         r = string(cr);
      }
      atom->SetAtomType(atString);
      atom->SetString(r);
      break;
   }
   fun_minmax(f_max,>)
   fun_minmax(f_min,<)
   fun_double_one(f_sin,sin)
   fun_double_one(f_cos,cos)
   fun_double_one(f_tan,tan)
   fun_double_one(f_asin,asin)
   fun_double_one(f_acos,acos)
   fun_double_one(f_atan,atan)
   fun_double_one(f_sinh,sinh)
   fun_double_one(f_cosh,cosh)
   fun_double_one(f_tanh,tanh)
   fun_double_one(f_asinh,asinh)
   fun_double_one(f_acosh,acosh)
   fun_double_one(f_atanh,atanh)
   fun_double_one(f_exp,exp)
   fun_double_one(f_log,log)
   fun_double_one(f_log10,log10)
   case f_eval: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      MainPos = 0;
      code=param1->GetString();

      if (debug) cout << "eval code:" << endl;
      if (debug) cout << code << endl;

      DeleteTreeNode(node);
      ParseTreeNode(node);
      EvalTreeNode(node,atom,locvars); // evaluating node directly to atom
      break;
   }
   case f_include: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      string filename=param1->GetString();

      string filebody;
      file_read(filename,filebody);

      MainPos = 0;
      code = filebody;
      if (debug) cout << "including code:" << endl;
      if (debug) cout << code << endl;

      // current node with function f_include will be overwritten by new code
      DeleteTreeNode(node); // it will delete only parameter - file name passed to include
      ParseTreeNode(node);
      EvalTreeNode(node,atom,locvars); // evaluating node directly to atom

      break;
   }
   case f_includeonce: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      string filename=param1->GetString();

      bool DoInclude = true;

      if (true) {
         unordered_set<string>::const_iterator it = IncludeList.find(filename);
         if (it== IncludeList.end()) {
            IncludeList.insert(filename);

         } else DoInclude = false;
      }

      if (DoInclude) {
         string filebody;
         file_read(filename,filebody);

         MainPos = 0;
         code = filebody;
         if (debug) cout << "including code:" << endl;
         if (debug) cout << code << endl;

         // current node with function f_include will be overwritten by new code
         // it will delete only parameter - file name passed to include
         DeleteTreeNode(node);
         ParseTreeNode(node);
         // evaluating node directly to atom
         EvalTreeNode(node,atom,locvars);
      }
      break;
   }
   case f_argc: {
      atom->SetAtomType(atInt);
      atom->SetInt(arglist.size());
      break;
   }
   case f_argv: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      string r="";
      if (param1->GetInt()<(int)arglist.size()) {
         r = arglist[param1->GetInt()];
      }
      atom->SetAtomType(atString);
      atom->SetString(r);
      break;
   }
   fun_tok(f_tok,tok)
   fun_tok(f_rtok,rtok)
   case f_frac: {
      // note: if passed argument is negative then fractional part will also be negative
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      double intpart;
      double fractpart = modf(param1->GetDouble(),&intpart);
      atom->SetAtomType(atDouble);
      atom->SetDouble(fractpart);
      break;
   }
   case f_sqr: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      if (param1->GetAtomType()==atInt ) {
         atom->SetAtomType(atInt);
         atom->SetInt(param1->GetInt()*param1->GetInt());
      } else {
         atom->SetAtomType(atDouble);
         atom->SetDouble(param1->GetDouble()*param1->GetDouble());
      }
      break;
   }
   case f_char: {
      // argument ASCII code of char
      // function return char represented by ASCII code
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      char ch=char(param1->GetInt());
      string r = string(1,ch);
      atom->SetAtomType(atString);
      atom->SetString(r);
      break;
   }
   case f_rand: {
      // if no argument then it will simply return double
      switch (node->GetParamCount()) {
      case 1:
         paramnode1 = node->GetParam(0);
         EvalTreeNode(paramnode1,param1,locvars);
         if (param1->GetAtomType()==atInt) {
            atom->SetAtomType(atInt);
            atom->SetInt(rand() % param1->GetInt());
         } else {
            atom->SetAtomType(atDouble);
            atom->SetDouble(param1->GetDouble()*(double)rand()/RAND_MAX);
         }
         break;
      case 2:
         paramnode1 = node->GetParam(0);
         EvalTreeNode(paramnode1,param1,locvars);
         paramnode2 = node->GetParam(1);
         EvalTreeNode(paramnode2,param2,locvars);
         if (param1->GetAtomType()==atInt && param2->GetAtomType()==atInt) {
            atom->SetAtomType(atInt);
            atom->SetInt(param1->GetInt() + rand() % (param2->GetInt() - param1->GetInt()));
         } else {
            double r = (double)rand()/RAND_MAX;
            atom->SetAtomType(atDouble);
            atom->SetDouble(param1->GetDouble()+r*(param2->GetDouble()-param1->GetDouble()));
         }
         break;
      default:
         // it will return random value from 0 to 1
         atom->SetAtomType(atDouble);
         atom->SetDouble((double)rand()/RAND_MAX);
         break;
      }
      break;
   }
   fun_str(f_trim,strtrim)
   fun_str(f_trimleft,strtrimleft)
   fun_str(f_trimright,strtrimright)
   case f_fileread: {
      // to-do: it works with full paths like /home/user/test/test.txt
      // but it does not work with paths like ~/test/test.txt
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      string r="";
      //if (file_exists())
      file_read(param1->GetString(),r);
      atom->SetAtomType(atString);
      atom->SetString(r);
      break;
   }
   case f_fileexists: {
      // to-do: it works with full paths like /home/user/test/test.tx
      // but it does not work with paths like ~/test/test.txt
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      int r;
      if (file_exists(param1->GetString())) r=1; else r=0;
      atom->SetAtomType(atInt);
      atom->SetInt(r);
      break;
   }
   case f_str: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      atom->SetAtomType(atString);
      atom->SetString(param1->ToString());
      break;
   }
   case f_int: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      int r;
      switch (param1->GetAtomType()) {
      case atDouble:
      r = (int)param1->GetDouble();
      break;
      case atInt:
      r = param1->GetInt();
      break;
      default:
      r = atoi(param1->GetString().c_str());
      break;
      }
      atom->SetAtomType(atInt);
      atom->SetInt(r);
      break;
   }
   case f_float: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      double r;
      switch (param1->GetAtomType()) {
      case atDouble:
      r=param1->GetDouble();
      break;
      case atInt:
      r=(double)param1->GetInt();
      break;
      default:
      r = atof(param1->GetString().c_str());
      break;
      }
      atom->SetAtomType(atDouble);
      atom->SetDouble(r);
      break;
   }
   case f_pi: {
      atom->SetAtomType(atDouble);
      // this is one of the way to get value of pi with double precision
      // another way is to define it as constant
      // third way is to use _USE_MATH_DEFINES macro M_PI
      atom->SetDouble(4*atan(1));
      break;
   }
   case f_e: {
      atom->SetAtomType(atDouble);
      atom->SetDouble(exp(1));
      break;
   }
   case f_sign: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      int r;
      if (param1->GetDouble()>0) r=1; else if (param1->GetDouble()<0) r=-1; else r=0;
      atom->SetAtomType(atInt);
      atom->SetInt(r);
      break;
   }
   case f_filewrite: case f_fileappend: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode2,param2,locvars);
      file_write(param1->GetString(),param2->GetString(),(node->GetFunCode()==f_fileappend));
      atom->SetAtomType(atInt);
      atom->SetInt(0); // to-do: return non zero in case of failure
      break;
   }
   case f_now: {
      int r=time(nullptr);
      atom->SetAtomType(atInt);
      atom->SetInt(r);
      break;
   }
   case f_strdate: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);

      // the first param is date. because if second param format
      // is not specified than we may use default format
      time_t t = (time_t)param1->GetInt();
      string f;
      if (node->GetParamCount()==2) {
         paramnode2 = node->GetParam(1);
         EvalTreeNode(paramnode2,param2,locvars);
         f=param2->GetString();
      } else {
         // it's ISO 8601 date and time format
         f = "%Y-%m-%d %H:%M:%S";
         // but the problem is that there are C and C++ implementations of strftime
         // C++ http://en.cppreference.com/w/cpp/chrono/c/strftime
         // C http://en.cppreference.com/w/c/chrono/strftime
         // there the expressions like %T, %D are part of C++11. it must be there
         // right now C++ variant also does not work
      }

      string r="";
      char buf[100];
      if (std::strftime(buf, sizeof(buf), f.c_str(), localtime(&t))>0) {
         r = buf;
      }
      atom->SetAtomType(atString);
      atom->SetString(r);
      break;
   }
   // think: maybe instead of different functions year(), month() etc
   // use just datepart(date,param) function with parameter?
   fun_datepart(f_year,tm_year+1900)
   fun_datepart(f_month,tm_mon+1)
   fun_datepart(f_day,tm_mday)
   fun_datepart(f_hour,tm_hour)
   fun_datepart(f_minute,tm_min)
   fun_datepart(f_second,tm_sec)
   fun_datepart(f_dayofweek,tm_wday+1) // Sunday will be returned as 1
   fun_datepart(f_dayofyear,tm_yday+1) // 1st Jan will be returned as 1
   fun_datepart(f_week,tm_yday / 7 + 1)
   case f_date: {
      int c = node->GetParamCount();
      struct tm tms;
      int p=0;
      AtomEnt *param;
      int val;
      while (p<c) {
         param = new AtomEnt();
         EvalTreeNode(node->GetParam(p),param,locvars);
         val = param->GetInt();
         switch (p) {
         case 0: tms.tm_year = val-1900; break;
         case 1: tms.tm_mon = val-1; break;
         case 2: tms.tm_mday = val; break;
         case 3: tms.tm_hour = val; break;
         case 4: tms.tm_min = val; break;
         case 5: tms.tm_sec = val; break;
         }
         delete param;
         p++;
      }
      time_t t = mktime(&tms);
      atom->SetAtomType(atInt);
      atom->SetInt((int)t);
      break;
   }
   case f_difftime: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      time_t t1 = (time_t)param1->GetInt();
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode2,param2,locvars);
      time_t t2 = (time_t)param2->GetInt();
      double r = difftime(t1,t2);
      //if (debug) cout << "t1: " << t1 << endl;
      //if (debug) cout << "t2: " << t2 << endl;
      //if (debug) cout << "r: " << r << endl;
      atom->SetAtomType(atDouble);
      atom->SetDouble(r);
      break;
   }
   case f_isleapyear: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      int r;
      if (IsLeapYear(param1->GetInt())) r=1; else r=0;
      atom->SetAtomType(atInt);
      atom->SetInt(r);
      break;
   }
   case f_daysinyear: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      int r=GetDaysInYear(param1->GetInt());
      atom->SetAtomType(atInt);
      atom->SetInt(r);
      break;
   }
   case f_daysinmonth: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode2,param2,locvars);
      int r=GetDaysInMonth(param1->GetInt(),param2->GetInt());
      atom->SetAtomType(atInt);
      atom->SetInt(r);
      break;
   }
   case f_format: case f_printf: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);

      string f = param1->GetString();
      string r="";
      int p=0;
      size_t i=0;
      i = f.find("%");
      size_t t=0;
      string tag;
      AtomEnt *param;
      while (i!=string::npos) {
         r+=f.substr(t,i-t);
         t = i+1;
         // note: p - pointer, n - nothing to print are not supported now
         while (t<f.size() && !char_in_array(f[t],"%sdiuoxXfFeEgGaA")) t++;
         if (t<f.size() && p<node->GetParamCount()) {
            param = new AtomEnt();
            p++;
            EvalTreeNode(node->GetParam(p),param,locvars);
            tag = f.substr(i,t-i+1);
            if (f[t]=='%') {
               r+='%';
            } else {
               char buf[100];
               if (f[t]=='s') {
                  snprintf(buf, sizeof(buf), tag.c_str(), param->GetString().c_str());
               } else if (char_in_array(f[t],"diuoxX")) {
                  snprintf(buf, sizeof(buf), tag.c_str(), param->GetInt());
               } else {
                  snprintf(buf, sizeof(buf), tag.c_str(), param->GetDouble());
               }
               r += buf;
            }
            delete param;
         }
         t++;
         i = f.find("%",t);
      }
      if (t<f.size()) r+=f.substr(t); // copy tail until the end of string
      if (node->GetFunCode()==f_format) {
         atom->SetAtomType(atString);
         atom->SetString(r);
      } else {
         cout << r;
         atom->SetAtomType(atInt);
         atom->SetInt(0);
      }
      break;
   }
   case f_input: {
      if (node->GetParamCount()==1) {
         paramnode1 = node->GetParam(0);
         EvalTreeNode(paramnode1,param1,locvars);
         // to-do: probably the same as in f_print if param type is int or double
         // then also print the number
         cout << param1->GetString();
      }
      string input;
      getline(cin,input); // correct way to read whole line until \n
      atom->SetAtomType(atString);
      atom->SetString(input);
      break;
   }
   case f_cmd: {
      for (int i=0; i<node->GetParamCount(); i++) {
         paramnode1 = node->GetParam(i);
         EvalTreeNode(paramnode1,param1,locvars);
      }
      // returns value of last param
      // to-do: check if the function was passed without params like cmd()
      atom->Assign(param1);
      break;
   }
   case f_exit: {
      int status=0;
      if (node->GetParamCount()==1) {
         paramnode1 = node->GetParam(0);
         EvalTreeNode(paramnode1,param1,locvars);
         status=param1->GetInt();
      }
      atom->SetAtomType(atInt);
      atom->SetInt(status);
      exit(status);
      break;
   }
   case f_sys: {
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      int r;
      r=system(param1->GetString().c_str());
      atom->SetAtomType(atInt);
      atom->SetInt(r);
      break;
   }
   case f_var: {
      // declare local variable
      // two parameters, first is variable name
      // second is (option) variable initial value
      int r = 1;
      paramnode1 = node->GetParam(0);
      string VarName = paramnode1->GetVarName();
      map<string,AtomEnt *>::const_iterator ifind;
      if (locvars) {
         ifind = locvars->find(VarName);
         AtomEnt *var;
         if ( ifind != locvars->end() ) {
            // variable is found
            var = ifind->second;
         } else {
            var = new AtomEnt();
            (*locvars)[VarName] = var;
         }
         if (node->GetParamCount()==2) {
            paramnode2 = node->GetParam(1);
            EvalTreeNode(paramnode2,param2,locvars);
            var->Assign(param2);
         }
         r = 0;
      }
      atom->SetAtomType(atInt);
      atom->SetInt(r);
      break;

      /*
      EvalTreeNode(paramnode1,param1,locvars);
      int r;
      r=system(param1->GetString().c_str());
      atom->SetAtomType(atInt);
      atom->SetInt(r);
      */
      break;
   }
   case f_fun: {
      // all we need to do is to remember location of this custom function
      paramnode1 = node->GetParam(0);
      custcodes[paramnode1->GetVarName()]=node;
      atom->SetAtomType(atInt);
      atom->SetInt(0);
      break;
   }
   case f_list: {
      atom->SetAtomType(atList);
      atom->CreateListValue();
      for (int i=0; i<node->GetParamCount(); i++) {
         paramnode1 = node->GetParam(i);
         AtomEnt *param = new AtomEnt();
         EvalTreeNode(paramnode1,param,locvars);
         atom->AddListElem(param);
         // note: we don't delete param here because it stays in memory
      }
      break;
   }
   case f_listsize: {
      // first param maybe a variable or list
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param2,locvars);
      atom->SetAtomType(atInt);
      atom->SetInt(param2->GetListSize());
      // if node is atom then we need to clear the list (while we should not clear it if it's some branch
      // of multi-level tree)
      // otherwise if the node is var than we don't clear it
      //if (node->GetNodeType()==ntAtom) delete param;
      break;
   }
   case f_listget: {
      // first param maybe a variable or list
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      // to-do: if first param is not evaluated into list then show error message
      // second param is index in list
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode2,param2,locvars);
      int ElemIndex =param2->GetInt();
      AtomEnt *Elem = param1->GetListElem(ElemIndex);
      if (debug) cout << "list elem value: " << Elem->ToString() << endl;
      atom->Assign(Elem);
      break;
   }
   case f_listset: {
      //the function is very similar to listinsert the only difference:
      // it replaces, updated existing element instead of replacing it
      // first param maybe a variable or list
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      // to-do: if first param is not evaluated into list then show error message
      // second param is index in list
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode2,param2,locvars);
      int ElemIndex =param2->GetInt();

      TreeNode *paramnode3 = node->GetParam(2);
      AtomEnt *param3 = new AtomEnt();
      EvalTreeNode(paramnode3,param3,locvars);

      param1->SetListElem(ElemIndex, param3);
      // second way: get element and then set it here
      // this method works fine
      /*
      AtomEnt *param =param1->GetListElem(ElemIndex);
      if (debug) {
         cout << "got element value: " << endl;
         param->PrintValue();
      }
      param->ClearListValue();
      param->Assign(param3);
      */

      // we don't delete param3 here

      // to-do: should we return 0 in case element was found and removed and -1 otherwise?
      atom->SetAtomType(atInt);
      atom->SetInt(0);
      break;
   }
   case f_listappend: {
      // first param maybe a variable or list
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      // to-do: if first param is not evaluated into list then show error message
      // second param is index in list
      paramnode2 = node->GetParam(1);
      AtomEnt *param = new AtomEnt();
      EvalTreeNode(paramnode2,param,locvars);
      param1->AddListElem(param);
      if (debug) {
         cout << "resulting list after adding: " << endl;
         param1->PrintValue();
      }
      // we don't delete param here
      atom->SetAtomType(atInt);
      atom->SetInt(0);
      break;
   }
   case f_listremove: {
      // first param maybe a variable or list
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      // to-do: if first param is not evaluated into list then show error message
      // second param is index in list
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode2,param2,locvars);
      int ElemIndex =param2->GetInt();
      param1->RemoveListElem(ElemIndex);
      // to-do: should we return 0 in case element was found and removed and -1 otherwise?
      atom->SetAtomType(atInt);
      atom->SetInt(0);
      break;
   }
   case f_listinsert: {
      // first param maybe a variable or list
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      // to-do: if first param is not evaluated into list then show error message
      // second param is index in list
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode2,param2,locvars);
      int ElemIndex =param2->GetInt();

      TreeNode *paramnode3 = node->GetParam(2);
      AtomEnt *param3 = new AtomEnt();
      EvalTreeNode(paramnode3,param3,locvars);

      param1->InsertListElem(ElemIndex, param3);
      // we don't delete param3 here

      // to-do: should we return 0 in case element was found and removed and -1 otherwise?
      atom->SetAtomType(atInt);
      atom->SetInt(0);
      break;
   }
   case f_listcreate: {
      atom->SetAtomType(atList);
      atom->CreateListValue();

      // first is number of elements in new list
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      int ElemCount =param1->GetInt();

      // second parameter is obligatory default value
      paramnode2 = node->GetParam(1);
      EvalTreeNode(paramnode2,param2,locvars);

      // we do element by element growing of the list with default value
      for (int i=0; i<ElemCount; i++) {
         AtomEnt *param = new AtomEnt();
         param->Assign(param2);
         atom->AddListElem(param);
         // note: we don't delete param here because it stays in memory
      }
      break;
   }
   case f_listcopy: {
      // first param maybe a variable or list
      // to-do: current realization only create one-level copy of the list
      // but if the list is a deep tree with multiple-branches then these branches are not copied
      // we need to create a recursive function to create a copy of each element and its sub-elements
      paramnode1 = node->GetParam(0);
      EvalTreeNode(paramnode1,param1,locvars);
      for (int i=0; i<param1->GetListSize(); i++) {
         AtomEnt *elem = param1->GetListElem(i);
         AtomEnt *param = new AtomEnt();
         param->Assign(elem);
         atom->AddListElem(param);
         // note: we don't delete param here because it stays in memory
      }
      atom->SetAtomType(atList);
      break;
   }

   default:
      // this is not a built-in function. it's probably a custom function
      //paramnode1 = node->GetParam(0);
      //TreeNode *funnode = custcodes[paramnode1->GetVarName()];
      if (debug) cout << "calling custom function: " << node->GetCustFunName() << endl;
      TreeNode *funnode = custcodes[node->GetCustFunName()];
      if (funnode) {
         // if custom function found then evaluate it
         // put all parameters into local variables
         map<string,AtomEnt *> localvars;
         map<string,AtomEnt *>::iterator it;
         TreeNode *varnamenode;
         TreeNode *varvaluenode;
         AtomEnt *varvalue;
         for (int i=0; i<node->GetParamCount(); i++) {
            varnamenode = funnode->GetParam(i+1);
            varvaluenode = node->GetParam(i);
            varvalue = new AtomEnt();
            EvalTreeNode(varvaluenode,varvalue,locvars);
            localvars[varnamenode->GetVarName()] = varvalue;
            if (debug) {
               cout << "function param: " << varnamenode->GetVarName() << endl;
               cout << "value: ";
               varvalue->PrintValue();
            }
         }
         // the last parameter is always function body (no anonymous functions yet!)
         TreeNode *funbodynode=funnode->GetParam(funnode->GetParamCount()-1);
         EvalTreeNode(funbodynode,param1,&localvars);
         atom->Assign(param1);
         // delete all created local variables
         // good discussion about deleting local variables is here
         // https://stackoverflow.com/questions/17861423/proper-way-to-destroy-a-map-that-has-pointer-values
         // https://stackoverflow.com/questions/33825560/c-delete-map-of-pointers-free-invalid-size
         // so we have to delete each value to avoid memory leaks
         // another solution is to use smart pointers
         ClearLocalVars(&localvars);
         // localvars.clear(); // this will delete all elements.
         // but it's not needed because they will be removed during automatic map deletion
      } else {
         // not defined operation
         atom->SetAtomType(atInt);
         atom->SetInt(0);
      }
      break;
   } // switch
   delete param1;
   delete param2;
   }
}

void ByteCode::DeleteTreeNode(TreeNode *node)
{
   if (node->GetNodeType()==ntFun) {
      TreeNode *param;
      for (int i=0; i<node->GetParamCount(); i++) {
         param = node->GetParam(i);
         DeleteTreeNode(param);
         delete param;
      }
      node->ClearParams();
   }
}

void ByteCode::Process(const string& ACode)
{
   clock_t stime = clock() / (CLOCKS_PER_SEC / 1000);

   StringBuffer = "";

   MainPos = 0;
   code = ACode;
   if (debug) cout << code << endl;

   TreeNode *node = new TreeNode();
   ParseTreeNode(node);
   AtomEnt *atom = new AtomEnt();
   // to-do: probably make a command line parameter -r to display tree node
   if (ShowTree) PrintTreeNode(node);
   // should we create "local" global variables and pass it was 3rd element to EvalTreeNode?
   // it will mean that the local variables will be used only in main body of the program
   // but the local variables will be not accessible inside custom functions
   // so it make sense to create such variables
   // what's a better name for such variables: local or private?
   map<string,AtomEnt *> localvars;
   map<string,AtomEnt *>::iterator it;
   EvalTreeNode(node,atom,&localvars);
   ClearLocalVars(&localvars);
   // and we probably call it localvars.clear()
   // while we work in interactive mode. should be even clear local vars here?

   if (debug) cout << "result:" << endl;
   if (debug) atom->PrintValue();

   DeleteTreeNode(node);
   delete node;
   delete atom;

   // if we run in interactive mode then we don't need to clear vars.
   // in current version we clean vars only in destructor of ByteCode

   // note: no need to clean custcodes because they contain only references to nodes
   // and nodes are cleaned by DeleteTreeNode

   clock_t etime = clock() / (CLOCKS_PER_SEC / 1000);
   if (ShowRunTime) {
      cout << endl << "done in " << (etime-stime)<< " msecs" << endl;
   }
}

void ByteCode::ParseCommandLineParams(const string& params)
{
   // 0 char is - skip it
   // for example, may receive params -td
   for (unsigned int i=1; i<params.size(); i++) {
      switch (params[i]) {
      case 't': SetShowRunTime(true); break;
      case 'd': SetDebug(true); break;
      case 'r': ShowTree=true; break;
      }
   }
}
