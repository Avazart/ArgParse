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

TEST(positional,zero_or_more) //  '*'
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'*'>("p1");
  auto p2 = parser.addArgument<int,'*'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");

  ASSERT_TRUE(parser.parseCmdLine("1")) << parser.errorString();
  ASSERT_EQ(p1->values().size(),1);
  ASSERT_EQ(p2->values().size(),0);
  ASSERT_EQ(p3->values().size(),0);
  parser.reset();

  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_EQ(p1->values().size(),3);
  ASSERT_EQ(p2->values().size(),0);
  ASSERT_EQ(p3->values().size(),0);
  parser.reset();

  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_EQ(p1->values().size(),5);
  ASSERT_EQ(p2->values().size(),0);
  ASSERT_EQ(p3->values().size(),0);
}

TEST(positional,optional) //  '?'
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'?'>("p1");
  auto p2 = parser.addArgument<int,'?'>("p2");
  auto p3 = parser.addArgument<int,'?'>("p3");

  ASSERT_TRUE(parser.parseCmdLine("1")) << parser.errorString();
  ASSERT_TRUE(p1->hasValue());
  ASSERT_FALSE(p2->hasValue());
  ASSERT_FALSE(p3->hasValue());
  parser.reset();

  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_TRUE(p1->hasValue());
  ASSERT_TRUE(p2->hasValue());
  ASSERT_TRUE(p3->hasValue());
  parser.reset();

  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_TRUE(p1->hasValue());
  ASSERT_TRUE(p2->hasValue());
  ASSERT_TRUE(p3->hasValue());
}


TEST(positional,one_or_more) //  '+'
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");
  auto p3 = parser.addArgument<int,'+'>("p3");

  // 1
  ASSERT_TRUE(parser.parseCmdLine("1")) << parser.errorString();
  ASSERT_EQ(p1->values().size(),1);
  ASSERT_EQ(p2->values().size(),0);
  ASSERT_EQ(p3->values().size(),0);
  parser.reset();

  // 3
  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_EQ(p1->values().size(),1);
  ASSERT_EQ(p2->values().size(),1);
  ASSERT_EQ(p3->values().size(),1);
  parser.reset();

  // 5
  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_EQ(p1->values().size(),3);
  ASSERT_EQ(p2->values().size(),1);
  ASSERT_EQ(p3->values().size(),1);
}

TEST(positional,mixed1) // **+
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'*'>("p1");
  auto p2 = parser.addArgument<int,'*'>("p2");
  auto p3 = parser.addArgument<int,'+'>("p3");

  // 1
  ASSERT_TRUE(parser.parseCmdLine("1")) << parser.errorString();
  ASSERT_EQ(p1->values().size(),0);
  ASSERT_EQ(p2->values().size(),0);
  ASSERT_EQ(p3->values().size(),1);
  ASSERT_EQ(p3->values(),(std::vector<int>{1}));
  parser.reset();

  // 3
  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_EQ(p1->values().size(),2);
  ASSERT_EQ(p3->values(),(std::vector<int>{1,2}));

  ASSERT_EQ(p2->values().size(),0);

  ASSERT_EQ(p3->values().size(),1);
  ASSERT_EQ(p3->values(),(std::vector<int>{3}));
  parser.reset();

  // 5
  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_EQ(p1->values().size(),4);
  ASSERT_EQ(p3->values(),(std::vector<int>{1,2,3,4}));

  ASSERT_EQ(p2->values().size(),0);
  ASSERT_EQ(p3->values().size(),1);
  ASSERT_EQ(p3->values(),(std::vector<int>{5}));
}


TEST(positional,mixed2) // *++
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'*'>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");
  auto p3 = parser.addArgument<int,'+'>("p3");

  // 1
  ASSERT_FALSE(parser.parseCmdLine("1"));
  parser.reset();

  // 3
  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1}));
  ASSERT_EQ(p2->values(),(std::vector<int>{2}));
  ASSERT_EQ(p3->values(),(std::vector<int>{3}));
  parser.reset();

  // 5
  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2,3}));
  ASSERT_EQ(p2->values(),(std::vector<int>{4}));
  ASSERT_EQ(p3->values(),(std::vector<int>{5}));
}


TEST(positional,mixed3) // +**
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");
  auto p2 = parser.addArgument<int,'*'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");

  // 1
  ASSERT_FALSE(parser.parseCmdLine("1"));
  parser.reset();

  // 2
  ASSERT_TRUE(parser.parseCmdLine("1 2")) << parser.errorString();
  ASSERT_FALSE(p1->hasValue());
  ASSERT_EQ(p2->values(),(std::vector<int>{1}));
  ASSERT_EQ(p3->values(),(std::vector<int>{2}));
  parser.reset();

  // 3
  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2,3}));
  ASSERT_FALSE(p2->hasValue());
  ASSERT_FALSE(p3->hasValue());
  parser.reset();

  // 5
  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2,3,4,5}));
  ASSERT_FALSE(p2->hasValue());
  ASSERT_FALSE(p3->hasValue());
}

TEST(positional,mixed4) // ++*
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");

  // 1
  ASSERT_FALSE(parser.parseCmdLine("1"));
  parser.reset();

  // 2
  ASSERT_TRUE(parser.parseCmdLine("1 2")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1}));
  ASSERT_EQ(p2->values(),(std::vector<int>{2}));
  ASSERT_FALSE(p1->hasValue());
  parser.reset();

  // 3
  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2}));
  ASSERT_EQ(p2->values(),(std::vector<int>{3}));
  ASSERT_FALSE(p3->hasValue());
  parser.reset();

  // 5
  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2,3,4}));
  ASSERT_EQ(p2->values(),(std::vector<int>{5}));
  ASSERT_FALSE(p3->hasValue());
}

TEST(positional,mixed5) // +*+
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'+'>("p1");
  auto p2 = parser.addArgument<int,'*'>("p2");
  auto p3 = parser.addArgument<int,'+'>("p3");

  // 1
  ASSERT_FALSE(parser.parseCmdLine("1"));
  parser.reset();

  // 2
  ASSERT_TRUE(parser.parseCmdLine("1 2")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1}));
  ASSERT_EQ(p2->values(),(std::vector<int>{}));
  ASSERT_EQ(p3->values(),(std::vector<int>{2}));
  parser.reset();

  // 3
  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2}));
  ASSERT_EQ(p2->values(),(std::vector<int>{}));
  ASSERT_EQ(p3->values(),(std::vector<int>{3}));
  parser.reset();

  // 5
  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2,3,4}));
  ASSERT_EQ(p2->values(),(std::vector<int>{}));
  ASSERT_EQ(p3->values(),(std::vector<int>{5}));
}

TEST(positional,mixed6) // *+*
{
  ArgumentParser parser;
  auto p1 = parser.addArgument<int,'*'>("p1");
  auto p2 = parser.addArgument<int,'+'>("p2");
  auto p3 = parser.addArgument<int,'*'>("p3");

  // 1
  ASSERT_FALSE(parser.parseCmdLine("1"));
  parser.reset();

  // 2
  ASSERT_TRUE(parser.parseCmdLine("1 2")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1}));
  ASSERT_EQ(p2->values(),(std::vector<int>{2}));
  ASSERT_EQ(p3->values(),(std::vector<int>{}));
  parser.reset();

  // 3
  ASSERT_TRUE(parser.parseCmdLine("1 2 3")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2}));
  ASSERT_EQ(p2->values(),(std::vector<int>{3}));
  ASSERT_EQ(p3->values(),(std::vector<int>{}));
  parser.reset();

  // 5
  ASSERT_TRUE(parser.parseCmdLine("1 2 3 4 5")) << parser.errorString();
  ASSERT_EQ(p1->values(),(std::vector<int>{1,2,3,4}));
  ASSERT_EQ(p2->values(),(std::vector<int>{5}));
  ASSERT_EQ(p3->values(),(std::vector<int>{}));
}

int main(int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
