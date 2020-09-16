#include <string>
#include <iostream>
#include <numeric>
#include <functional>
//------------------------------------------------------------------
#include "../../ArgParse/ArgumentParser.h"
//------------------------------------------------------------------
int main(/*int argc, char *argv[]*/)
{
  using namespace std;
  using namespace std::literals;

  ArgParse::ArgumentParser<char> parser("-/");
  try
  {
    //  optional arg expected one or more values ('+')
    //  required= false
    auto sumValues = parser.addOptional<float,'+'>("-s","--sum","/s");
    // You can use:
    // sumValues.setRequired(true);
    // sumValues.setRange(0,1.0);
    auto multValues= parser.addOptional<float,'+'>("-m","--mult","/m");

    parser.parseCmdLine("/m 2 4.5 --sum 3.1 2.4");
    //  Or use:
    //  parser.parseArgs(argc,argv);

    if(sumValues.exists())
    {
      const float sum= accumulate(begin(sumValues.values()),
                                  end(sumValues.values()),
                                  0.f);
      cout << "sum= " << sum << endl;
    }

    if(multValues.exists())
    {
      const float mult=
          accumulate(begin(*multValues), // or begin(multValues.values()),
                     end(*multValues),
                     1.f,
                     multiplies<float>());

      cout << "mult= " << mult << endl;
    }
  }
  catch (const ArgParse::Exception<char>& e)
  {
     cerr<< "Error: " << e.what()       << endl << endl;
     cerr<< "Usage: " << parser.usage() << endl;
     cerr<< "Help:\n" << parser.help()  << endl;
  }
  return 0;
}
