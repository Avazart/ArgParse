#include <string>
#include <iostream>
//------------------------------------------------------------------
#include "../ArgParse/ArgumentParser.h"
//------------------------------------------------------------------
template<typename T>
void printArg(T* arg)
{
  if(!arg->exists())
  {
    std::cout << "Arg '"<< arg->makeOptions() << "' not exists!"<<std::endl;
    return;
  }

  std::cout << "Arg '"<< arg->makeOptions() << "' : ";
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
  using namespace ArgParse;

  ArgumentParser parser;
  auto prog = parser.addArgument<std::string,1,1>("prog");

  std::cout << prog->typeId() << std::endl;

//  auto p1 = parser.addArgument<bool,'?'>("p1");
//  auto p2 = parser.addArgument<int,'+'>("p2");
//  auto o = parser.addArgument<int>("-o","--optional");

//  auto subParser1 = parser.addSubParser("cmd1");
//  auto p4 = subParser1->addArgument<int,'?'>("p3");
//  auto p5 = subParser1->addArgument<int,2,2>("p4");

//  auto subParser2 = parser.addSubParser("cmd2");
//  auto p6 = subParser2->addArgument<int,'?'>("a6");
//  auto p7 = subParser2->addArgument<int,2,2>("a7");

//  //if( !parser.parseArgs(argc,argv) )
//  if(!parser.parseCmdLine("1 2 3 4 5 -o 1 cmd2 8 9 10"))
//  {
//    std::cerr<< parser.errorString() << std::endl;

//    std::cout<<"usage: "<< parser.usage() << std::endl;
//    std::cout<< parser.help() << std::endl;
//    return 1;
//  }

//  printArg(p1);
//  printArg(p2);

//  if(o->exists() && o->hasValue())
//    printArg(o);

//  if(subParser1->exists())
//  {
//    printArg(p4);
//    printArg(p5);
//  }

//  if(subParser2->exists())
//  {
//    printArg(p6);
//    printArg(p7);
//  }

  return 0;
}
//------------------------------------------------------------------



