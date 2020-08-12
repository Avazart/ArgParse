#include <gtest/gtest.h>
#include <iostream>


TEST(test_case_name, test_name)
{
  ASSERT_EQ(1, 1) << "1 is not equal 0";
}

TEST(test_case_name2, test_name2)
{
  ASSERT_EQ(1, 0) << "1 is not equal 0";
}

TEST(test_case_name3, test_name3)
{
  ASSERT_EQ(1, 1) << "1 is not equal 0";
}


int main(int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
