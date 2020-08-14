#include <gtest/gtest.h>
#include <iostream>

#include "../../ArgParse/ArgumentParser.h"

using namespace argparse;
using namespace std::literals;

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
  auto p1 = parser.addArgument<int,0,1>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");

  ASSERT_TRUE(parser.parseArgs(argc,argv) )
      << parser.errorString();
}


TEST(common,nargs)
{
  ArgumentParser parser;
//  auto p1 = parser.addArgument<int>("p1");

//  auto p2 = parser.addArgument<int,3,5>("p2");

//    auto p3 = parser.addArgument<int,NArgs::optional>("p3");
//  auto p4 = parser.addArgument<int,NArgs::zeroOrMore>("p4");
//  auto p5 = parser.addArgument<int,NArgs::oneOrMore>("p5");
}

TEST(positional,types)
{
  ArgumentParser parser;
  auto boolValues = parser.addArgument<bool,4,4>("boolValues");
  auto intValue = parser.addArgument<int,1,1>("intValue");
  //auto uIntValue = parser.addArgument<unsigned int>("uIntValue");
  auto doubleValue1 = parser.addArgument<double,1,1>("doubleValue1");
  auto doubleValue2 = parser.addArgument<double,1,1>("doubleValue2");

  ASSERT_TRUE(parser.parseCmdLine("1 0 true false -10 3.14 -7.6e-8"s))
      << parser.errorString();

  ASSERT_TRUE(boolValues->hasValue());
  ASSERT_EQ(boolValues->values(),(std::vector<bool>{true,false,true,false}))<< "check bool 1/0 true/false";

  ASSERT_TRUE(intValue->hasValue());
  ASSERT_EQ(intValue->value(),-10) << "check negative int value";

  ASSERT_TRUE(doubleValue1->hasValue());
  ASSERT_DOUBLE_EQ(doubleValue1->value(),3.14) << "check double";

  ASSERT_TRUE(doubleValue2->hasValue());
  ASSERT_DOUBLE_EQ(doubleValue2->value(),-7.6e-8) << "check double exponent";
}

TEST(positional_zero_or_more,n1) //  '*'
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'*'>("p1");
  auto p2 = parser.addArgument<int,'*'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");
  // 1
  ASSERT_TRUE(parser.parseCmdLine("1")) << parser.errorString();
  ASSERT_EQ(p1->values().size(),1);
  ASSERT_EQ(p2->values().size(),0);
  ASSERT_EQ(p3->values().size(),0);
}

TEST(positional_zero_or_more,n3) //  '*'
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'*'>("p1");
  auto p2 = parser.addArgument<int,'*'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");
  // 3
  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_EQ(p1->values().size(),3);
  ASSERT_EQ(p2->values().size(),0);
  ASSERT_EQ(p3->values().size(),0);
}

TEST(positional_zero_or_more,n5) //  '*'
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'*'>("p1");
  auto p2 = parser.addArgument<int,'*'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");
  // 5
  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_EQ(p1->values().size(),5);
  ASSERT_EQ(p2->values().size(),0);
  ASSERT_EQ(p3->values().size(),0);
}

TEST(positional_optional,n1) //  '?'
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'?'>("p1");
  auto p2 = parser.addArgument<int,'?'>("p2");
  auto p3 = parser.addArgument<int,'?'>("p3");
  // 1
  ASSERT_TRUE(parser.parseCmdLine("1")) << parser.errorString();
  ASSERT_TRUE(p1->hasValue());
  ASSERT_FALSE(p2->hasValue());
  ASSERT_FALSE(p3->hasValue());
}

TEST(positional_optional,n3) //  '?'
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'?'>("p1");
  auto p2 = parser.addArgument<int,'?'>("p2");
  auto p3 = parser.addArgument<int,'?'>("p3");
  // 3
  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_TRUE(p1->hasValue());
  ASSERT_TRUE(p2->hasValue());
  ASSERT_TRUE(p3->hasValue());
}

TEST(positional_optional,n5) //  '?'
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'?'>("p1");
  auto p2 = parser.addArgument<int,'?'>("p2");
  auto p3 = parser.addArgument<int,'?'>("p3");
  // 5
  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_TRUE(p1->hasValue());
  ASSERT_TRUE(p2->hasValue());
  ASSERT_TRUE(p3->hasValue());
}


TEST(positional_one_or_more, n1) //  "+++"
{
  ArgumentParser parser;
  parser.addArgument<int,'+'>("p1");
  parser.addArgument<int,'+'>("p2");
  parser.addArgument<int,'+'>("p3");
  // 1
  ASSERT_FALSE(parser.parseCmdLine("1"));
}

TEST(positional_one_or_more, n3) //  "+++"
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");
  auto p3 = parser.addArgument<int,'+'>("p3");
  // 3
  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1}));
  ASSERT_EQ(p2->values(),(std::vector<int>{2}));
  ASSERT_EQ(p3->values(),(std::vector<int>{3}));
}

TEST(positional_one_or_more, n5) //  "+++"
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");
  auto p3 = parser.addArgument<int,'+'>("p3");
  // 5
  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2,3}));
  ASSERT_EQ(p2->values(),(std::vector<int>{4}));
  ASSERT_EQ(p3->values(),(std::vector<int>{5}));
}


TEST(positional_mixed1,n1) // **+
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'*'>("p1");
  auto p2 = parser.addArgument<int,'*'>("p2");
  auto p3 = parser.addArgument<int,'+'>("p3");
  // 1
  ASSERT_TRUE(parser.parseCmdLine("1")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{}));
  ASSERT_EQ(p2->values(),(std::vector<int>{}));
  ASSERT_EQ(p3->values(),(std::vector<int>{1}));
}

TEST(positional_mixed1,n3) // **+
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'*'>("p1");
  auto p2 = parser.addArgument<int,'*'>("p2");
  auto p3 = parser.addArgument<int,'+'>("p3");
  // 3
  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2}));
  ASSERT_EQ(p2->values(),(std::vector<int>{}));
  ASSERT_EQ(p3->values(),(std::vector<int>{3}));
}

TEST(positional_mixed1,n5) // **+
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'*'>("p1");
  auto p2 = parser.addArgument<int,'*'>("p2");
  auto p3 = parser.addArgument<int,'+'>("p3");
  // 5
  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2,3,4}));
  ASSERT_EQ(p2->values(),(std::vector<int>{}));
  ASSERT_EQ(p3->values(),(std::vector<int>{5}));
}


TEST(positional_mixed2,n1) // *++
{
  ArgumentParser parser;
  parser.addArgument<int,'*'>("p1");
  parser.addArgument<int,'+'>("p2");
  parser.addArgument<int,'+'>("p3");
  // 1
  ASSERT_FALSE(parser.parseCmdLine("1"));
}

TEST(positional_mixed2,n3) // *++
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'*'>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");
  auto p3 = parser.addArgument<int,'+'>("p3");
  // 3
  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1}));
  ASSERT_EQ(p2->values(),(std::vector<int>{2}));
  ASSERT_EQ(p3->values(),(std::vector<int>{3}));
}

TEST(positional_mixed2,n5) // *++
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'*'>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");
  auto p3 = parser.addArgument<int,'+'>("p3");
  // 5
  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2,3}));
  ASSERT_EQ(p2->values(),(std::vector<int>{4}));
  ASSERT_EQ(p3->values(),(std::vector<int>{5}));
}


TEST(positional_mixed3,n1) // +**
{
  ArgumentParser parser;
  parser.addArgument<int,'+'>("p1");
  parser.addArgument<int,'*'>("p2");
  parser.addArgument<int,'*'>("p3");

  // 1
  ASSERT_TRUE(parser.parseCmdLine("1"));
}

TEST(positional_mixed3,n2) // +**
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");
  auto p2 = parser.addArgument<int,'*'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");
  // 2
  ASSERT_TRUE(parser.parseCmdLine("1 2")) << parser.errorString();;
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2}));
  ASSERT_EQ(p2->values(),(std::vector<int>{}));
  ASSERT_EQ(p3->values(),(std::vector<int>{}));
}

TEST(positional_mixed3,n3) // +**
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");
  auto p2 = parser.addArgument<int,'*'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");
  // 3
  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2,3}));
  ASSERT_EQ(p2->values(),(std::vector<int>{}));
  ASSERT_EQ(p3->values(),(std::vector<int>{}));
}

TEST(positional_mixed3,n5) // +**
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");
  auto p2 = parser.addArgument<int,'*'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");
  // 5
  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2,3,4,5}));
  ASSERT_EQ(p2->values(),(std::vector<int>{}));
  ASSERT_EQ(p3->values(),(std::vector<int>{}));
}

TEST(positional_mixed4,n1) // ++*
{
  ArgumentParser parser;
  parser.addArgument<int,'+'>("p1");
  parser.addArgument<int,'+'>("p2");
  parser.addArgument<int,'*'>("p3");
  // 1
  ASSERT_FALSE(parser.parseCmdLine("1"));
  parser.reset();
}

TEST(positional_mixed4,n2) // ++*
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");
  // 2
  ASSERT_TRUE(parser.parseCmdLine("1 2")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1}));
  ASSERT_EQ(p2->values(),(std::vector<int>{2}));
  ASSERT_EQ(p3->values(),(std::vector<int>{}));
}

TEST(positional_mixed4,n3) // ++*
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");
  // 3
  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2}));
  ASSERT_EQ(p2->values(),(std::vector<int>{3}));
  ASSERT_EQ(p3->values(),(std::vector<int>{}));
}

TEST(positional_mixed4,n5) // ++*
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");
  // 5
  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2,3,4}));
  ASSERT_EQ(p2->values(),(std::vector<int>{5}));
  ASSERT_EQ(p3->values(),(std::vector<int>{}));
}

TEST(positional_mixed5,n1) // +*+
{
  ArgumentParser parser;
  parser.addArgument<int,'+'>("p1");
  parser.addArgument<int,'*'>("p2");
  parser.addArgument<int,'+'>("p3");
  // 1
  ASSERT_FALSE(parser.parseCmdLine("1"));
}

TEST(positional_mixed5,n2) // +*+
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");
  auto p2 = parser.addArgument<int,'*'>("p2");
  auto p3 = parser.addArgument<int,'+'>("p3");
  // 2
  ASSERT_TRUE(parser.parseCmdLine("1 2")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1}));
  ASSERT_EQ(p2->values(),(std::vector<int>{}));
  ASSERT_EQ(p3->values(),(std::vector<int>{2}));
}


TEST(positional_mixed5,n3) // +*+
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");
  auto p2 = parser.addArgument<int,'*'>("p2");
  auto p3 = parser.addArgument<int,'+'>("p3");
  // 3
  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2}));
  ASSERT_EQ(p2->values(),(std::vector<int>{}));
  ASSERT_EQ(p3->values(),(std::vector<int>{3}));
}

TEST(positional_mixed5,n5) // +*+
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");
  auto p2 = parser.addArgument<int,'*'>("p2");
  auto p3 = parser.addArgument<int,'+'>("p3");
  // 5
  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2,3,4}));
  ASSERT_EQ(p2->values(),(std::vector<int>{}));
  ASSERT_EQ(p3->values(),(std::vector<int>{5}));
}

TEST(positional_mixed6,n1) // *+*
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'*'>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");
  // 1
  ASSERT_TRUE(parser.parseCmdLine("1")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{}));
  ASSERT_EQ(p2->values(),(std::vector<int>{1}));
  ASSERT_EQ(p3->values(),(std::vector<int>{}));
}

TEST(positional_mixed6,n2) // *+*
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'*'>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");
  // 2
  ASSERT_TRUE(parser.parseCmdLine("1 2")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1}));
  ASSERT_EQ(p2->values(),(std::vector<int>{2}));
  ASSERT_EQ(p3->values(),(std::vector<int>{}));
}

TEST(positional_mixed6,n3) // *+*
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'*'>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");
  // 3
  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2}));
  ASSERT_EQ(p2->values(),(std::vector<int>{3}));
  ASSERT_EQ(p3->values(),(std::vector<int>{}));
}

TEST(positional_mixed6,n5) // *+*
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'*'>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");
  // 5
  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2,3,4}));
  ASSERT_EQ(p2->values(),(std::vector<int>{5}));
  ASSERT_EQ(p3->values(),(std::vector<int>{}));
}

int main(int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  // // --gtest_filter="positional.one_or_more"
  //::testing::GTEST_FLAG(filter) = "positional_mixed3.*";
  return RUN_ALL_TESTS();
}
