#include <string>
#include <iostream>

#include <assert.h>

#include "../ArgParse/ArgumentParser.h"

using namespace argparse;
//------------------------------------------------------------------
template<typename T>
void printArg(T* arg)
{
  if(!arg->exists())
  {
    std::cout << "Arg '"<< arg->options() << "' not exists!"<<std::endl;
    return;
  }

  std::cout << "Arg '"<< arg->options() << "' : ";
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
void test1()
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");
  auto p3 = parser.addArgument<int,2,2>("p3");

  const std::string cmdLine = R"(1 2 3 4 5 6)";
  assert(parser.parseCmdLine(cmdLine));

  assert( (p1->values()== std::vector<int>{ 1, 2, 3 }));
  assert( (p2->values()== std::vector<int>{ 4 }));
  assert( (p3->values()== std::vector<int>{ 5,6 }));
}
//------------------------------------------------------------------
void test2()
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");

  auto sub = parser.addSubParser("cmd");
  auto c = sub->addArgument<int>("c");

  const std::string cmdLine = R"(1 2 3 cmd 4)";
  assert(!parser.parseCmdLine(cmdLine));
  assert(parser.errorString()=="Error: argument 'p1': invalid int value: 'cmd'");
}
//------------------------------------------------------------------
void test3()
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,3,3>("p1");
  auto o1 = parser.addArgument<int>("-o1");

  auto sub = parser.addSubParser("cmd1");
  auto c = sub->addArgument<int>("c");

  const std::string cmdLine = R"(1 2 3 -o1 4 cmd1 5)";

  assert(parser.parseCmdLine(cmdLine));
  assert((p1->values()== std::vector<int>{ 1, 2, 3 }));
  assert(o1->value()== 4);
  assert(c->value()== 5 );
}
//------------------------------------------------------------------
void test4()
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1","");
  p1->setHelp("positional");

  auto o1 = parser.addArgument<int,'+'>("-o","--opt");
  o1->setHelp("optional");

  auto sub1 = parser.addSubParser("cmd1");
  sub1->setSubParserHelp("sub parser #1");
  auto c1 = sub1->addArgument<int>("c1");

  auto sub2 = parser.addSubParser("cmd2");
  auto c2 = sub2->addArgument<int>("c2");
  sub2->setSubParserHelp("sub parser #2");

  std::cerr<< parser.usage() << std::endl;
  std::cerr<< parser.help()  << std::endl;

  return;

//  const std::string cmdLine = R"(1 2 3 -o1 4 cmd3 5)";

//  if(!parser.parseCmdLine(cmdLine))
//     std::cerr<< parser.errorString()<< std::endl;



////  assert((p1->values()== std::vector<int>{ 1, 2, 3 }));
////  assert(o1->value()== 4);
////  assert(c->value()== 5 );

//  printArg(p1);
//  printArg(o1);
//  printArg(c1);
//  printArg(c2);
}
//------------------------------------------------------------------
int main(/*int argc, char *argv[]*/)
{
  using namespace std::literals;

  test1();
  test2();
  test3();
  test4();

//  ArgumentParser parser;
//  auto a1 = parser.addArgument<int,1,1>("a1");
//  auto a2 = parser.addArgument<int,'+'>("a2");
//  auto a3 = parser.addArgument<int,'+'>("-a2");

//  auto sub = parser.addSubParser("cmd");

//  auto a4 = sub->addArgument<int,'?'>("a3");
//  auto a5 = sub->addArgument<int,2,2>("a4");

//  std::string cmdLine = R"(1 2 3 -a2 3 4 cmd 5 6)";
//  std::cout << cmdLine << std::endl;

//  if(!parser.parseCmdLine(cmdLine))
//    std::cerr<< parser.errorString()<< std::endl;

//   printArg(a1);
//   printArg(a2);
//   printArg(a3);

//   printArg(a4);
//   printArg(a5);

  return 0;
}
//------------------------------------------------------------------



