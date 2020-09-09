#include <string>
#include <iostream>
//------------------------------------------------------------------
#include "../../ArgParse/ArgumentParser.h"
//------------------------------------------------------------------
template<typename T>
void printArg(const T& arg)
{
  using namespace std;
  using namespace ArgParse;

  cout<<(arg.info()->argType()==ArgType::positional
          ? "Positional ": "Optional ")
       << "arg '" << arg.info()->fullName()<< "':\t";

  if(!arg.exists())
  {
    cout<< " not exists!"<<std::endl;
    return;
  }

  if(arg) cout<< arg.info()->valueAsString() <<std::endl;
  else    cout<<"hasn't value!"<<std::endl;
}
//------------------------------------------------------------------
int main(/*int argc, char *argv[]*/)
{
  using namespace std;
  using namespace std::literals;
  using namespace StringUtils::literals;

  ArgParse::ArgumentParser<char> parser;
  try
  {
    auto p1 = parser.addPositional<string,4,4>("p1");
    p1.setHelp("positional #1");

    auto subParser1 = parser.addSubParser("cmd1");
    subParser1->setSubParserHelp("First subparser");

       auto p3 = subParser1->addPositional<int>("p3");
       p3.setHelp("first subparser p3");

    auto subParser2 = parser.addSubParser("cmd2");
    subParser2->setSubParserHelp("Second subparser");

       auto p4 = subParser2->addPositional<int,2,3>("p4");
       p4.setHelp("second subparser p4");

       auto p5 = subParser2->addOptional<int>("-p5");
       p5.setHelp("second subparser p5");

    parser.parseCmdLine("a b c d cmd2 1 2 3 -p5");

    printArg(p1);

    if(subParser1->exists())
       printArg(p3);

    if(subParser2->exists())
    {
       printArg(p4);
       printArg(p5);
    }
  }
  catch (const ArgParse::Exception<char>& e)
  {
     cerr << "Error: " << e.what()          << endl << endl;
     cerr << "Usage: " << parser.usage()    << endl;
     cerr << "Help:\n" << parser.help(true) << endl;
  }
  return 0;
}
//------------------------------------------------------------------



