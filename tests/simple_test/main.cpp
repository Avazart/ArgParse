#include <gtest/gtest.h>

#include <iostream>
#include <vector>
#include <string>

#include "../../ArgParse/ArgumentParser.h"

using namespace ArgParse;
using namespace std::literals;

TEST(common, default_value)
{
  using namespace StringUtils;

  ArgumentParser parser;
  auto p0 = parser.addPositional<int,'+'>("p1");
  p0 = std::vector<int>{1,2}; // default valus

  ASSERT_NO_THROW(parser.parseCmdLine("3 4"));

  ASSERT_EQ(*p0,(std::vector<int>{3,4}));
}


TEST(StringUtils, strToBool)
{
  using namespace StringUtils;

  ASSERT_EQ(strToBool<char>("0"), 0);
  ASSERT_EQ(strToBool<char>("1"), 1);

  ASSERT_EQ(strToBool<char>("true"), true);
  ASSERT_EQ(strToBool<char>("false"), false);

  ASSERT_THROW(strToBool<char>("00"), std::invalid_argument);
  ASSERT_THROW(strToBool<char>("2"),  std::invalid_argument);
  ASSERT_THROW(strToBool<char>("-1"), std::invalid_argument);
  ASSERT_THROW(strToBool<char>("true1"), std::invalid_argument);
  ASSERT_THROW(strToBool<char>("1true"), std::invalid_argument);
}

TEST(StringUtils, strToInt)
{
  using namespace StringUtils;

  ASSERT_LT(sizeof(int),sizeof(long long));

  ASSERT_EQ(strToInt<char>("10"), 10);
  ASSERT_EQ(strToInt<char>("-10"), -10);
  ASSERT_THROW(strToInt<char>("abc"),       std::invalid_argument);
  ASSERT_THROW(strToInt<char>("10abc"),     std::invalid_argument);

  const long long maxInt = std::numeric_limits<int>::max();
  const long long lowestInt= std::numeric_limits<int>::lowest();

  ASSERT_THROW(strToInt<char>(std::to_string(maxInt+1)),std::out_of_range);
  ASSERT_THROW(strToInt<char>(std::to_string(lowestInt-1)),std::out_of_range);
}

TEST(StringUtils, strToUInt)
{
  using namespace StringUtils;

  ASSERT_LT(sizeof(int),sizeof(long long));

  ASSERT_EQ(strToUInt<char>("10"), 10);
  ASSERT_THROW(strToUInt<char>("-10"),   std::out_of_range);
  ASSERT_THROW(strToUInt<char>("abc"),   std::invalid_argument);
  ASSERT_THROW(strToUInt<char>("10abc"), std::invalid_argument);

  const long long maxUInt = std::numeric_limits<unsigned int>::max();
  const long long lowestUInt= std::numeric_limits<unsigned int>::lowest();

  ASSERT_THROW(strToUInt<char>(std::to_string(maxUInt+1)),std::out_of_range);
  ASSERT_THROW(strToUInt<char>(std::to_string(lowestUInt-1)),std::out_of_range);
}

TEST(StringUtils, strToLong)
{
  using namespace StringUtils;

  ASSERT_EQ(strToLong<char>("10"), 10);
  ASSERT_EQ(strToLong<char>("-10"), -10);
  ASSERT_THROW(strToLong<char>("abc"),   std::invalid_argument);
  ASSERT_THROW(strToLong<char>("10abc"), std::invalid_argument);

  ASSERT_LT(sizeof(long),sizeof(long long));

  const long long maxLong = std::numeric_limits<long>::max();
  const long long lowestLong= std::numeric_limits<long>::lowest();

  ASSERT_THROW(strToLong<char>(std::to_string(maxLong+1)),std::out_of_range);
  ASSERT_THROW(strToLong<char>(std::to_string(lowestLong-1)),std::out_of_range);
}


TEST(split_cmd_line, test_quote)
{
  auto args = split(R"(1 2 "31 32 33" 4 "5"6" 7" 8)"s);
  ASSERT_EQ(args.size(), 6) << "split result size must be 6";
  ASSERT_EQ(args[0],"1"s);
  ASSERT_EQ(args[1],"2"s);
  ASSERT_EQ(args[2],"31 32 33"s)<< "quote escape spaces";
  ASSERT_EQ(args[3],"4"s);
  ASSERT_EQ(args[4],"56 7"s) << "inner quote ignored until next quote-space combination";
  ASSERT_EQ(args[5],"8"s);
}

TEST(common,LatinView)
{
  using namespace StringUtils::literals;

  ASSERT_EQ("12"_lv+"3"s, "123"s);
  ASSERT_EQ("1"s+"23"_lv, "123"s);

  ASSERT_EQ("12"_lv+ L"3"s,  L"123"s);
  ASSERT_EQ(L"1"s  +"23"_lv, L"123"s);

  ASSERT_TRUE("123"_lv == "123"s);
  ASSERT_TRUE("123"_lv == L"123"s);

  ASSERT_FALSE("123"_lv != "123"s);
  ASSERT_FALSE("123"_lv != L"123"s);
}

TEST(common,Exceptions)
{
  ArgumentParser parser;
  auto p0 = parser.addPositional<int,2,2>("p1");

  ASSERT_THROW(parser.parseCmdLine("1"),
               WrongCountException<char>);
  parser.reset();

  ASSERT_THROW(parser.parseCmdLine("1 2 3 4 5"),
               UnrecognizedArgumentsException<char>);
  parser.clear();

  auto p1 = parser.addPositional<int,1,1>("p1");
  const long long maxInt = std::numeric_limits<int>::max();
  ASSERT_THROW(parser.parseCmdLine(std::to_string(maxInt+1)),
                                   OutOfRangeException<char>);
  parser.reset();

  const long long lowestInt = std::numeric_limits<int>::lowest();
  ASSERT_THROW(parser.parseCmdLine(std::to_string(lowestInt-1)),
                                   OutOfRangeException<char>);
  parser.reset();

  p1.setRange(0,10);
  ASSERT_THROW(parser.parseCmdLine("20"),OutOfRangeException<char>);
  parser.reset();

  ASSERT_THROW(parser.parseCmdLine("-20"),OutOfRangeException<char>);
  parser.clear();

  auto p2 = parser.addPositional<std::string,1,1>("p2");
  p2.setMinLength(5);
  ASSERT_THROW(parser.parseCmdLine("abc"),LengthErrorException<char>);
  parser.reset();

  p2.setMinLength(1);
  p2.setMaxLength(2);
  ASSERT_THROW(parser.parseCmdLine("abc"),LengthErrorException<char>);
  parser.reset();

  ASSERT_NO_THROW(parser.parseCmdLine("ab"));
  auto p3 = parser.addOptional<int>("-r","--req");
  p3.setRequired(true);

  ASSERT_NO_THROW(parser.parseCmdLine("ab --req"));
  parser.reset();

  ASSERT_THROW(parser.parseCmdLine("ab"),
               ArgumentRequiredException<char>);
  parser.reset();

  ASSERT_THROW(parser.parseCmdLine("ab --c"),
               UnrecognizedArgumentsException<char>);

  parser.removeAllArguments();

  // Optional

  parser.addOptional<int>("-x","--y");
  // already added
  ASSERT_THROW(parser.addOptional<int>("--y","-z"),
               ArgumentException<char>);

  ASSERT_THROW(parser.addOptional<int>("-z", "-x"),
               ArgumentException<char>);

  // wrong prefix
  ASSERT_THROW(parser.addOptional<int>("--k","withoutPrefix"),
               ArgumentException<char>);

  // Invalid chose
}

TEST(common,parseArgs)
{
  const char* argv[] =
  {
    "1",
    "2",
    "3"
  };
  const int argc = 3;

  ArgumentParser parser;
  parser.addPositional<int,0,1>("p1");
  parser.addPositional<int,NArgs::oneOrMore>("p2");

  ASSERT_NO_THROW(parser.parseArgs(argc,argv));
}


TEST(common,nargs)
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int>("p1");
  p1= 0;

  auto p2 = parser.addPositional<int,3,5>("p2");
  p2 = (std::vector<int>{});

  auto p3 = parser.addPositional<int,NArgs::optional>("p3");
  auto p4 = parser.addPositional<int,NArgs::zeroOrMore>("p4");
  auto p5 = parser.addPositional<int,NArgs::oneOrMore>("p5");
}

TEST(common,wchar)
{
  ArgumentParser<wchar_t> parser;

  ArgT<double,'?',wchar_t> p0 = parser.addPositional<double>(L"p0");
  IntArg<'?',wchar_t> p1 = parser.addPositional<int>(L"p1");

  auto p2 = parser.addPositional<std::wstring,2,3>(L"p2");

  StringArg<'?',wchar_t>
      p3 = parser.addPositional<std::wstring,NArgs::optional>(L"p3");

  StringArg<'*',wchar_t>
      p4 = parser.addPositional<std::wstring,NArgs::zeroOrMore>(L"p4");

  StringArg<'+',wchar_t>
      p5 = parser.addPositional<std::wstring,NArgs::oneOrMore>(L"p5");

  ASSERT_NO_THROW(parser.parseCmdLine(LR"(1.5 2 "31 32 33" 4 "5"6" 7" 8 9)"s));

  ASSERT_TRUE(p0);
  ASSERT_DOUBLE_EQ(*p0, 1.5);

  ASSERT_TRUE(p1);
  ASSERT_DOUBLE_EQ(*p1, 2);

  ASSERT_EQ(*p2,(std::vector<std::wstring>{L"31 32 33"s,L"4"s,L"56 7"s}));

  ASSERT_EQ(*p3,L"8"s);
}

TEST(positional,types)
{
  ArgumentParser parser;
  auto boolValues = parser.addPositional<bool,4,4>("boolValues");
  auto intValue = parser.addPositional<int,1,1>("intValue");
  //auto uIntValue = parser.addArgument<unsigned int>("uIntValue");
  auto doubleValue1 = parser.addPositional<double,1,1>("doubleValue1");
  auto doubleValue2 = parser.addPositional<double,1,1>("doubleValue2");

  ASSERT_NO_THROW(parser.parseCmdLine("1 0 true false -10 3.14 -7.6e-8"s));

  ASSERT_TRUE(boolValues);
  ASSERT_EQ(*boolValues,(std::vector<bool>{true,false,true,false}))<< "check bool 1/0 true/false";

  ASSERT_TRUE(intValue);
  ASSERT_EQ(*intValue,-10) << "check negative int value";

  ASSERT_TRUE(doubleValue1);
  ASSERT_DOUBLE_EQ(*doubleValue1,3.14) << "check double";

  ASSERT_TRUE(doubleValue2);
  ASSERT_DOUBLE_EQ(doubleValue2.value(),-7.6e-8) << "check double exponent";
}

TEST(positional_zero_or_more,n1) //  '*'
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'*'>("p1");
  auto p2 = parser.addPositional<int,'*'>("p2");
  auto p3 = parser.addPositional<int,'*'>("p3");
  // 1
  ASSERT_NO_THROW(parser.parseCmdLine("1"));
  ASSERT_EQ(p1.values().size(),1);
  ASSERT_EQ(p2.values().size(),0);
  ASSERT_EQ(p3.values().size(),0);
}

TEST(positional_zero_or_more,n3) //  '*'
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'*'>("p1");
  auto p2 = parser.addPositional<int,'*'>("p2");
  auto p3 = parser.addPositional<int,'*'>("p3");
  // 3
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3"));
  ASSERT_EQ(p1.values().size(),3);
  ASSERT_EQ(p2.values().size(),0);
  ASSERT_EQ(p3.values().size(),0);
}

TEST(positional_zero_or_more,n5) //  '*'
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'*'>("p1");
  auto p2 = parser.addPositional<int,'*'>("p2");
  auto p3 = parser.addPositional<int,'*'>("p3");
  // 5
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3 4 5"));
  ASSERT_EQ(p1.values().size(),5);
  ASSERT_EQ(p2.values().size(),0);
  ASSERT_EQ(p3.values().size(),0);
}

TEST(positional_optional,n1) //  '?'
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'?'>("p1");
  auto p2 = parser.addPositional<int,'?'>("p2");
  auto p3 = parser.addPositional<int,'?'>("p3");
  // 1
  ASSERT_NO_THROW(parser.parseCmdLine("1")) ;
  ASSERT_TRUE(p1)  << *p1;
  ASSERT_FALSE(p2) << *p2;
  ASSERT_FALSE(p3) << *p3;
}

TEST(positional_optional,n2) //  '?'
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'?'>("p1");
  auto p2 = parser.addPositional<int,'?'>("p2");
  auto p3 = parser.addPositional<int,'?'>("p3");
  // 1
  ASSERT_NO_THROW(parser.parseCmdLine("1 2")) ;
  ASSERT_TRUE(p1)  << *p1;
  ASSERT_TRUE(p2)  << *p2;
  ASSERT_FALSE(p3) << *p3;
}

TEST(positional_optional,n3) //  '?'
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'?'>("p1");
  auto p2 = parser.addPositional<int,'?'>("p2");
  auto p3 = parser.addPositional<int,'?'>("p3");
  // 3
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3")) ;
  ASSERT_TRUE(p1.hasValue());
  ASSERT_TRUE(p2.hasValue());
  ASSERT_TRUE(p3.hasValue());
}

TEST(positional_optional,n5) //  '?'
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'?'>("p1");
  auto p2 = parser.addPositional<int,'?'>("p2");
  auto p3 = parser.addPositional<int,'?'>("p3");
  // 5
  ASSERT_THROW(parser.parseCmdLine("1 2 3 4 5"),
               UnrecognizedArgumentsException<char>);
  ASSERT_TRUE(p1.hasValue());
  ASSERT_TRUE(p2.hasValue());
  ASSERT_TRUE(p3.hasValue());
}


TEST(positional_one_or_more, n1) //  "+++"
{
  ArgumentParser parser;
  parser.addPositional<int,'+'>("p1");
  parser.addPositional<int,'+'>("p2");
  parser.addPositional<int,'+'>("p3");
  // 1
  ASSERT_THROW(parser.parseCmdLine("1"),
               WrongCountException<char>);
}

TEST(positional_one_or_more, n3) //  "+++"
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'+'>("p1");
  auto p2 = parser.addPositional<int,'+'>("p2");
  auto p3 = parser.addPositional<int,'+'>("p3");
  // 3
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3")) ;
  ASSERT_EQ(*p1,(std::vector<int>{1}));
  ASSERT_EQ(*p2,(std::vector<int>{2}));
  ASSERT_EQ(*p3,(std::vector<int>{3}));
}

TEST(positional_one_or_more, n5) //  "+++"
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'+'>("p1");
  auto p2 = parser.addPositional<int,'+'>("p2");
  auto p3 = parser.addPositional<int,'+'>("p3");
  // 5
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3 4 5")) ;
  ASSERT_EQ(*p1,(std::vector<int>{1,2,3}));
  ASSERT_EQ(*p2,(std::vector<int>{4}));
  ASSERT_EQ(*p3,(std::vector<int>{5}));
}

TEST(positional_one_or_more, n5v2) //  "+++"
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'+'>("p1");
  auto p2 = parser.addPositional<int,'+'>("p2");
  auto p3 = parser.addPositional<int,2,3>("p3");
  // 5
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3 4 5")) ;
  ASSERT_EQ(*p1,(std::vector<int>{1,2}));
  ASSERT_EQ(*p2,(std::vector<int>{3}));
  ASSERT_EQ(*p3,(std::vector<int>{4,5}));
}


TEST(positional_mixed1,n1) // **+
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'*'>("p1");
  auto p2 = parser.addPositional<int,'*'>("p2");
  auto p3 = parser.addPositional<int,'+'>("p3");
  // 1
  ASSERT_NO_THROW(parser.parseCmdLine("1")) ;
  ASSERT_EQ(p1.values(),(std::vector<int>{}));
  ASSERT_EQ(p2.values(),(std::vector<int>{}));
  ASSERT_EQ(p3.values(),(std::vector<int>{1}));
}

TEST(positional_mixed1,n3) // **+
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'*'>("p1");
  auto p2 = parser.addPositional<int,'*'>("p2");
  auto p3 = parser.addPositional<int,'+'>("p3");
  // 3
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3")) ;
  ASSERT_EQ(p1.values(),(std::vector<int>{1,2}));
  ASSERT_EQ(p2.values(),(std::vector<int>{}));
  ASSERT_EQ(p3.values(),(std::vector<int>{3}));
}

TEST(positional_mixed1,n5) // **+
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'*'>("p1");
  auto p2 = parser.addPositional<int,'*'>("p2");
  auto p3 = parser.addPositional<int,'+'>("p3");
  // 5
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3 4 5")) ;
  ASSERT_EQ(p1.values(),(std::vector<int>{1,2,3,4}));
  ASSERT_EQ(p2.values(),(std::vector<int>{}));
  ASSERT_EQ(p3.values(),(std::vector<int>{5}));
}


TEST(positional_mixed2,n1) // *++
{
  ArgumentParser parser;
  parser.addPositional<int,'*'>("p1");
  parser.addPositional<int,'+'>("p2");
  parser.addPositional<int,'+'>("p3");
  // 1
  ASSERT_THROW(parser.parseCmdLine("1"),WrongCountException<char>);
}

TEST(positional_mixed2,n3) // *++
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'*'>("p1");
  auto p2 = parser.addPositional<int,'+'>("p2");
  auto p3 = parser.addPositional<int,'+'>("p3");
  // 3
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3")) ;
  ASSERT_EQ(p1.values(),(std::vector<int>{1}));
  ASSERT_EQ(p2.values(),(std::vector<int>{2}));
  ASSERT_EQ(p3.values(),(std::vector<int>{3}));
}

TEST(positional_mixed2,n5) // *++
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'*'>("p1");
  auto p2 = parser.addPositional<int,'+'>("p2");
  auto p3 = parser.addPositional<int,'+'>("p3");
  // 5
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3 4 5")) ;
  ASSERT_EQ(p1.values(),(std::vector<int>{1,2,3}));
  ASSERT_EQ(p2.values(),(std::vector<int>{4}));
  ASSERT_EQ(p3.values(),(std::vector<int>{5}));
}


TEST(positional_mixed3,n1) // +**
{
  ArgumentParser parser;
  parser.addPositional<int,'+'>("p1");
  parser.addPositional<int,'*'>("p2");
  parser.addPositional<int,'*'>("p3");

  // 1
  ASSERT_NO_THROW(parser.parseCmdLine("1"));
}

TEST(positional_mixed3,n2) // +**
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'+'>("p1");
  auto p2 = parser.addPositional<int,'*'>("p2");
  auto p3 = parser.addPositional<int,'*'>("p3");
  // 2
  ASSERT_NO_THROW(parser.parseCmdLine("1 2")) ;;
  ASSERT_EQ(p1.values(),(std::vector<int>{1,2}));
  ASSERT_EQ(p2.values(),(std::vector<int>{}));
  ASSERT_EQ(p3.values(),(std::vector<int>{}));
}

TEST(positional_mixed3,n3) // +**
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'+'>("p1");
  auto p2 = parser.addPositional<int,'*'>("p2");
  auto p3 = parser.addPositional<int,'*'>("p3");
  // 3
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3")) ;
  ASSERT_EQ(p1.values(),(std::vector<int>{1,2,3}));
  ASSERT_EQ(p2.values(),(std::vector<int>{}));
  ASSERT_EQ(p3.values(),(std::vector<int>{}));
}

TEST(positional_mixed3,n5) // +**
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'+'>("p1");
  auto p2 = parser.addPositional<int,'*'>("p2");
  auto p3 = parser.addPositional<int,'*'>("p3");
  // 5
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3 4 5")) ;
  ASSERT_EQ(p1.values(),(std::vector<int>{1,2,3,4,5}));
  ASSERT_EQ(p2.values(),(std::vector<int>{}));
  ASSERT_EQ(p3.values(),(std::vector<int>{}));
}

TEST(positional_mixed4,n1) // ++*
{
  ArgumentParser parser;
  parser.addPositional<int,'+'>("p1");
  parser.addPositional<int,'+'>("p2");
  parser.addPositional<int,'*'>("p3");
  // 1
  ASSERT_THROW(parser.parseCmdLine("1"),WrongCountException<char>);
}

TEST(positional_mixed4,n2) // ++*
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'+'>("p1");
  auto p2 = parser.addPositional<int,'+'>("p2");
  auto p3 = parser.addPositional<int,'*'>("p3");
  // 2
  ASSERT_NO_THROW(parser.parseCmdLine("1 2")) ;
  ASSERT_EQ(p1.values(),(std::vector<int>{1}));
  ASSERT_EQ(p2.values(),(std::vector<int>{2}));
  ASSERT_EQ(p3.values(),(std::vector<int>{}));
}

TEST(positional_mixed4,n3) // ++*
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'+'>("p1");
  auto p2 = parser.addPositional<int,'+'>("p2");
  auto p3 = parser.addPositional<int,'*'>("p3");
  // 3
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3")) ;
  ASSERT_EQ(p1.values(),(std::vector<int>{1,2}));
  ASSERT_EQ(p2.values(),(std::vector<int>{3}));
  ASSERT_EQ(p3.values(),(std::vector<int>{}));
}

TEST(positional_mixed4,n5) // ++*
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'+'>("p1");
  auto p2 = parser.addPositional<int,'+'>("p2");
  auto p3 = parser.addPositional<int,'*'>("p3");
  // 5
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3 4 5")) ;
  ASSERT_EQ(p1.values(),(std::vector<int>{1,2,3,4}));
  ASSERT_EQ(p2.values(),(std::vector<int>{5}));
  ASSERT_EQ(p3.values(),(std::vector<int>{}));
}

TEST(positional_mixed5,n1) // +*+
{
  ArgumentParser parser;
  parser.addPositional<int,'+'>("p1");
  parser.addPositional<int,'*'>("p2");
  parser.addPositional<int,'+'>("p3");
  // 1
  ASSERT_THROW(parser.parseCmdLine("1"),WrongCountException<char>);
}

TEST(positional_mixed5,n2) // +*+
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'+'>("p1");
  auto p2 = parser.addPositional<int,'*'>("p2");
  auto p3 = parser.addPositional<int,'+'>("p3");
  // 2
  ASSERT_NO_THROW(parser.parseCmdLine("1 2")) ;
  ASSERT_EQ(p1.values(),(std::vector<int>{1}));
  ASSERT_EQ(p2.values(),(std::vector<int>{}));
  ASSERT_EQ(p3.values(),(std::vector<int>{2}));
}


TEST(positional_mixed5,n3) // +*+
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'+'>("p1");
  auto p2 = parser.addPositional<int,'*'>("p2");
  auto p3 = parser.addPositional<int,'+'>("p3");
  // 3
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3")) ;
  ASSERT_EQ(p1.values(),(std::vector<int>{1,2}));
  ASSERT_EQ(p2.values(),(std::vector<int>{}));
  ASSERT_EQ(p3.values(),(std::vector<int>{3}));
}

TEST(positional_mixed5,n5) // +*+
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'+'>("p1");
  auto p2 = parser.addPositional<int,'*'>("p2");
  auto p3 = parser.addPositional<int,'+'>("p3");
  // 5
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3 4 5")) ;
  ASSERT_EQ(p1.values(),(std::vector<int>{1,2,3,4}));
  ASSERT_EQ(p2.values(),(std::vector<int>{}));
  ASSERT_EQ(p3.values(),(std::vector<int>{5}));
}

TEST(positional_mixed6,n1) // *+*
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'*'>("p1");
  auto p2 = parser.addPositional<int,'+'>("p2");
  auto p3 = parser.addPositional<int,'*'>("p3");
  // 1
  ASSERT_NO_THROW(parser.parseCmdLine("1")) ;
  ASSERT_EQ(p1.values(),(std::vector<int>{}));
  ASSERT_EQ(p2.values(),(std::vector<int>{1}));
  ASSERT_EQ(p3.values(),(std::vector<int>{}));
}

TEST(positional_mixed6,n2) // *+*
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'*'>("p1");
  auto p2 = parser.addPositional<int,'+'>("p2");
  auto p3 = parser.addPositional<int,'*'>("p3");
  // 2
  ASSERT_NO_THROW(parser.parseCmdLine("1 2")) ;
  ASSERT_EQ(*p1,(std::vector<int>{1}));
  ASSERT_EQ(*p2,(std::vector<int>{2}));
  ASSERT_EQ(*p3,(std::vector<int>{}));
}

TEST(positional_mixed6,n3) // *+*
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'*'>("p1");
  auto p2 = parser.addPositional<int,'+'>("p2");
  auto p3 = parser.addPositional<int,'*'>("p3");
  // 3
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3")) ;
  ASSERT_EQ(*p1,(std::vector<int>{1,2}));
  ASSERT_EQ(*p2,(std::vector<int>{3}));
  ASSERT_EQ(*p3,(std::vector<int>{}));
}

TEST(positional_mixed6,n5) // *+*
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<int,'*'>("p1");
  auto p2 = parser.addPositional<int,'+'>("p2");
  auto p3 = parser.addPositional<int,'*'>("p3");
  // 5
  ASSERT_NO_THROW(parser.parseCmdLine("1 2 3 4 5")) ;
  ASSERT_EQ(*p1,(std::vector<int>{1,2,3,4}));
  ASSERT_EQ(*p2,(std::vector<int>{5}));
  ASSERT_EQ(*p3,(std::vector<int>{}));
}

TEST(subParsers,t1)
{
  ArgumentParser parser;
  auto p1 = parser.addPositional<std::string,'+'>("p1");
     auto subParser1 = parser.addSubParser("cmd1");
     auto p2 = parser.addPositional<int>("p2");

  ASSERT_NO_THROW(parser.parseCmdLine("a b c d"));

  ASSERT_NO_THROW(parser.parseCmdLine("a b c d cmd1"));

//  ArgumentParser parser;
//  auto p1 = parser.addArgument<std::string,'+'>("p1");
//  auto p2 = parser.addArgument<int>("-p2");

//  auto subParser1 = parser.addSubParser("cmd1");
//  auto p3 = subParser1->addArgument<int>("p3");

//  auto subParser2 = parser.addSubParser("cmd2");
//  auto p4 = subParser2->addArgument<int,'+'>("p4");
//  auto p5 = subParser2->addArgument<int,'+'>("-p5");

//  parser.parseCmdLine("a b c -p2 1 cmd2 5 6 -p5 7 8");
}



int main(int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  // // --gtest_filter="positional.one_or_more"
  //::testing::GTEST_FLAG(filter) = "positional.types";
  return RUN_ALL_TESTS();
}
