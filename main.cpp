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
  auto a1 = parser.addArgument<bool,0,3>("first");
  auto a2 = parser.addArgument<std::string,'?'>("second","s");
  auto a3 = parser.addArgument<std::string,0,3>("-t");
  auto a4 = parser.addArgument<float,0,3>("-z","--z");

  // bool
  // int
  // unsigned short
  // string

  //a1->setRange(0,10);
  //a2->setMinLength(0);
  //a2->setMaxLength(4);

//  a4->setRange(0,1);

  std::string cmdLine = R"(1 true 0 -t a b c -z 0 0.5 1.0 5.0 5.0 )";
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



