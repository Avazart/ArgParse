#include <string>
#include <iostream>
//------------------------------------------------------------------
#include "../../ArgParse/ArgumentParser.h"
//------------------------------------------------------------------
int main(/*int argc, char *argv[]*/)
{
  using namespace std;
  using namespace std::literals;

  ArgParse::ArgumentParser<char> parser;
  try
  {
    // First positional arg expected one or more values(minCount=1,maxCount=max)
    ArgParse::StringArg<'+'>
        inputFiles =
           parser.addArgument<string,
                              ArgParse::NArgs::oneOrMore>("input_files");

    // '+' is analog NArgs::oneOrMore
    // '*' is analog NArgs::zeroOrMore
    // '?' is analog NArgs::zeroOrOne

    // Second positional arg expected one value(minCount=1,maxCount=1)
    // You can use auto
    auto outFile = parser.addArgument<string,1,1>("out_file");

    parser.parseCmdLine("\"C:\\Program Files\\in1.txt\"  "
                        "\"C:\\Program Files\\in2.txt\"  "
                        "\"C:\\Program Files\\in3.txt\"  "
                        "\"C:\\Program Files\\out.txt\"  "
                         );

    //  Or use:
    //  parser.parseArgs(argc,argv);

    // For access use method value(),values() or use operator*
    cout<< "Input files:" << endl;
    for(const string& inFile: *inputFiles)
       cout<< '\t'  << inFile << endl;

    cout<< "Output file:" << endl;
    cout<< '\t'  << *outFile << endl;
  }
  catch (const ArgParse::Exception<char>& e)
  {
     cerr << e.what()  << endl<< endl;
     cerr << "Usage: " << parser.usage() << endl;
     cerr << parser.help() << endl;
  }
  return 0;
}
