#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include <stdlib.h>     /* atof, malloc, free */
#include "atoment.h"

using namespace std;

string strtrimleft(const string& str);

enum TFunCodes      {f_add,f_mult,f_sub,f_div,f_mod,f_inc,f_dec,f_set,f_get,f_unset,
   f_eq,f_noteq,f_and,f_or,f_not,f_less,f_lesseq,f_gr,f_greq,f_if,f_switch,f_while,f_do,f_print,f_input,
   f_cmd,f_exit,f_sys,f_puts,f_cat,f_append,f_len,f_pos,f_copy,f_erase,f_insert,f_replace,f_treplace,
   f_upper,f_lower,f_run,f_round,f_pow,f_sqrt,f_abs,f_floor,f_ceil,f_trunc,f_env,f_max,
   f_min,f_sin,f_cos,f_tan,f_asin,f_acos,f_atan,f_sinh,f_cosh,f_tanh,f_asinh,
   f_acosh,f_atanh,f_exp,f_log,f_log10,f_eval,f_include,f_includeonce,f_argc,f_argv,f_tok,
   f_rtok,f_frac,f_sqr,f_char,f_rand,f_trim,f_trimleft,f_trimright,f_fileread, f_fileexists,
   f_str,f_int,f_float,f_pi,f_e,f_sign,f_filewrite,f_fileappend,f_now,f_strdate,
   f_year,f_month,f_day,f_hour,f_minute,f_second,f_week,f_dayofweek,f_dayofyear,
   f_date,f_difftime,f_isleapyear,f_daysinyear,f_daysinmonth,f_format,f_printf,f_fun,f_var,
   f_list,f_listsize,f_listget,f_listset,f_listappend,f_listremove,f_listinsert,
   f_listcreate,f_listcopy};

// in second variant List may be defines here
enum TNodeType {ntAtom,ntVar,ntFun};

class TreeNode
{
protected:
   TNodeType NodeType;
   AtomEnt *atom;
   string VarName;
   TFunCodes FunCode;
   string CustFunName; // this is name of custom function
   vector<TreeNode *> params;
public:
   TreeNode();
   ~TreeNode();
   void SetNodeType(TNodeType ANodeType) { NodeType=ANodeType;};
   TNodeType GetNodeType() { return NodeType;};
   void SetVarName(string AVarName) { VarName=AVarName;};
   string GetVarName() { return VarName;};
   void SetFunCode(TFunCodes AFunCode) { FunCode = AFunCode;};
   TFunCodes GetFunCode() { return FunCode;}
   string GetFunName();
   void SetCustFunName(string ACustFunName) { CustFunName = ACustFunName;};
   string GetCustFunName() { return CustFunName;};
   AtomEnt *GetAtom() { return atom;};
   void AddParam(TreeNode *param) { params.push_back(param);};
   TreeNode *GetParam(int i) { return params[i];};
   int GetParamCount() {return params.size();};
   void ClearParams();
};

class ByteCode
{
protected:
   bool debug;
   bool ShowRunTime;
   bool ShowTree;
   string code;
   string::size_type MainPos;
   //int MainPos;
   // variables
   map<string,AtomEnt *> vars;
   map<string,TFunCodes> funcodes;
   map<string,TreeNode *> custcodes;
   unordered_set<string> IncludeList;
   TFunCodes GetFunCode(const string& funname);
   vector<string> arglist;
   void skip_spaces();
   void ParseTreeNode(TreeNode * & anode);
   bool GetVar(string VarName, AtomEnt *atom, map<string,AtomEnt *> * locvars = NULL);
   bool SetVar(string VarName, AtomEnt *atom, map<string,AtomEnt *> * locvars = NULL);
   bool GetVarName(TreeNode *node, string& VarName, map<string,AtomEnt *> * locvars = NULL);
   void EvalTreeNode(TreeNode *node, AtomEnt *atom, map<string,AtomEnt *> * locvars = NULL);
   void DeleteTreeNode(TreeNode *node);
   void PrintTreeNode(TreeNode *node, string align="");
   void ClearLocalVars(map<string,AtomEnt *> * locvars);
public:
   string StringBuffer;
   ByteCode();
   ~ByteCode();
   void SetArgList(int argc, char *argv[]);
   void Process(const string& ACode);
   void SetDebug(bool ADebug) {debug=ADebug;};
   void SetShowRunTime(bool AShowRunTime) { ShowRunTime=AShowRunTime;};
   void ParseCommandLineParams(const string& params);
};

bool file_read(const string& FileName, string& FileBody);
