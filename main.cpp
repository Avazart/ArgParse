#include <string>
#include <iostream>

#include <assert.h>

#include "ArgParse/ArgumentParser.h"

using namespace argparse;
//------------------------------------------------------------------
template<typename T>
void printArg(T* arg)
{
  if(!arg->exists())
  {
    std::cout << "Arg '"<< arg->name() << "' not exists!"<<std::endl;
    return;
  }

  std::cout << "Arg '"<< arg->name() << "' : ";
  if(arg->hasValue())
  {
    if constexpr(arg->isContainer)
    {
     std::cout<< "[";
     for(auto value: arg->values())
       std::cout << "'"<<value << "', ";
     std::cout<< "] "<<std::endl;
    }
    else
    {
     std::cout<<"'"<<arg->value()<<"'"<<std::endl;
    }
  }
  else
  {
    std::cout<<"has not value!"<<std::endl;
  }
}
//------------------------------------------------------------------
int main(/*int argc, char *argv[]*/)
{
  using namespace std::literals;

  ArgumentParser parser;
  auto a1 = parser.addArgument<int,1,1>("first");
  auto a2 = parser.addArgument<int,'*'>("second");
  auto a3 = parser.addArgument<int,'?'>("second2");
  auto a4 = parser.addArgument<int,2,2>("33");

  std::string cmdLine = R"(1 2 3 4 5)";
  std::cout << cmdLine << std::endl;

  if(!parser.parseCmdLine(cmdLine))
    std::cerr<< parser.errorString()<< std::endl;

   printArg(a1);
   printArg(a2);
   printArg(a3);
   printArg(a4);


  return 0;
}
//------------------------------------------------------------------



