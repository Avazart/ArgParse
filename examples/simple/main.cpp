#include <string>
#include <iostream>
//------------------------------------------------------------------
#include "../../ArgParse/ArgumentParser.h"
//------------------------------------------------------------------
template<typename T>
void printArg(const T& arg)
{
  std::cout << "Arg '"
            << arg.info()->options()
            << "' : ";

  if(!arg.exists())
  {
    std::cout << " not exists!"<<std::endl;
    return;
  }

  if(arg)
  {
     if constexpr(arg.typeGroup()==ArgParse::TypeGroup::numbers ||
                  arg.typeGroup()==ArgParse::TypeGroup::strings)
     {
       std::cout<< "[";
       using StringUtils::join;

//       std::cout <<
//         join(*arg,", ",'\'','\'',
//            [](auto arg)
//            {
//              arg->
//            });

//       for(std::size_t i=0; i< (*arg).size(); ++i)
//       {
//         if(i>0)
//            std::cout<< ", ";
//         std::cout << "'"<< (*arg)[i]<< "'";
//       }
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
  using namespace std;
  using namespace std::literals;
  using namespace StringUtils::literals;

  ArgParse::ArgumentParser<char> parser;
  try
  {
    auto p1 = parser.addArgument<string,4,4>("p1");
    p1.setHelp("positional #1");

    auto subParser1 = parser.addSubParser("cmd1");
    subParser1->setSubParserHelp("First subparser");

    auto p3 = subParser1->addArgument<int>("p3");
    p3.setHelp("first subparser p3");

    auto subParser2 = parser.addSubParser("cmd2");
    subParser2->setSubParserHelp("Second subparser");

    auto p4 = subParser2->addArgument<int,'+'>("p4");
    p4.setHelp("second subparser p4");

    auto p5 = subParser2->addArgument<int,'+'>("-p5");
    p5.setHelp("second subparser p5");

    cout << parser.usage() << endl;
    cout << parser.help();

    return 0;

    parser.parseCmdLine("a b c d cmd2");

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
     cerr << e.what()  << endl<< endl;
     cerr << "Usage: " << parser.usage() << endl;
     cerr << parser.help(true) << endl;
  }
  return 0;
}
//------------------------------------------------------------------



