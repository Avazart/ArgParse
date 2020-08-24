#include <string>
#include <iostream>
//------------------------------------------------------------------
#include "../ArgParse/ArgumentParser.h"
//------------------------------------------------------------------
template<typename T>
void printArg(const T& arg)
{
//  if(!arg.exists())
//  {
//    std::cout << "Arg not exists!"<<std::endl;
//    return;
//  }

//  std::cout << "Arg '"<< arg->makeOptions() << "' : ";
  if(arg)
  {
     if constexpr(arg.typeGroup()==ArgParse::TypeGroup::numbers ||
                  arg.typeGroup()==ArgParse::TypeGroup::strings)
     {
       std::cout<< "[";
       for(std::size_t i=0; i< arg->size(); ++i)
       {
         if(i>0)
            std::cout<< ", ";
         std::cout << "'"<< arg.values()[i]<< "'";
       }
       std::cout<< "] "<<std::endl;
     }
     else
     {
       std::cout<<"'"<< *arg<<"'"<<std::endl;
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

  IntArg<'?'> p1 = parser.addArgument<int>("p1");
  p1= 10;

  IntArg<'+'> p2 = parser.addArgument<int,'+'>("p2");
  std::vector<int> vi{0,1,2};
  p2 = vi;

  printArg(p2);

  StringArg<'?'> p3 = parser.addArgument<std::string>("p3");
  p3= "10";

  ArgumentParser<wchar_t> parser2;
  StringArg<'?',wchar_t> p4 = parser2.addArgument<std::wstring>(L"p4");
  p4= L"10";

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



