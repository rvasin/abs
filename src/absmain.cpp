#include "abs.h"

#include <string.h> // strycpy()

int main(int argc, char *argv[])
{
   ByteCode *bcode = new ByteCode();
   bcode->SetArgList(argc,argv);
   string code="";

   //bcode->SetShowRunTime(true);
   //bcode->SetDebug(false);

   string params;
   string FileName = "";
   for (int i=1; i<argc; i++) {
      params = argv[i];
      // the first parameter which does not start with - is considered as file name
      // to-do: maybe exclude parameters started with - from numbering when using
      // internal abs argc(), argv() functions
      // so probably don't pass it to arglist
      if (params[0]=='-') bcode->ParseCommandLineParams(params);
      else if (FileName.empty()) FileName = params;
   }

   if (FileName.size()>0) {
      // to-do: check for file existence
      file_read(FileName,code);
      bcode->Process(code);
   } else {
      while (code!="exit") {
         cout << "abs> ";
         getline(cin,code); // correct way to read whole line until \n
         bcode->Process(code);
         // to-do: probably it should print evaluation result.
         // but it's better not do it.
      }
   }
   delete bcode;
   return 0;
}
