#include <string>
#include <array>
#include <iostream>
//------------------------------------------------------------------
#include "../../ArgParse/ArgumentParser.h"
//------------------------------------------------------------------
#include <Windows.h>
//------------------------------------------------------------------
int main(/*int argc, char *argv[]*/)
{
  SetConsoleCP(CP_UTF8);
  SetConsoleOutputCP(CP_UTF8);

  using namespace std;
  using namespace std::literals;

  const array cmdLines=
  {
    "a -o 1 cmd1",          // ArgParse::WrongCountException<char>
    "abc d -o 10 cmd1",     // ArgParse::LengthErrorException<char>
    "a b c -o x cmd1",      // ArgParse::InvalidArgumentException<char>
    "a b c -o 20 cmd1",     // ArgParse::OutOfRangeException<char>
    "a b c d e -o 10 cmd1", // ArgParse::UnrecognizedArgumentsException<char>
    "a b c -o 10 wrongCmd"  // ArgParse::InvalidChoiceException<char>
  };

  ArgParse::ArgumentParser<char> parser;
  auto p = parser.addArgument<string,2,3>("p");
  p.setMinLength(1);
  p.setMaxLength(2);
  auto o = parser.addArgument<int,'+'>("-o","--opt");
  o.setRange(5,15);
  auto subParser1 = parser.addSubParser("cmd1");
  auto subParser2 = parser.addSubParser("cmd2");

  cout<<"usage: "<< parser.usage() << endl << endl;

  for(const string& cmdLine: cmdLines)
  {
    try
    {
      try
      {
        parser.parseCmdLine(cmdLine);
      }
      catch(const ArgParse::Exception<char>& e)
      {
        cout<<"\""<<cmdLine<<"\"  "<< endl;
        cout<<"original: "<<  e.what() << endl;
        throw;
      }
    }
    catch (const ArgParse::WrongCountException<char>& e)
    {
      //  Error: argument 'p': expected values count: 2..3
      cout<< "translated: "
          <<  u8"Ошибка: аргумент '"   << e.arg()->options()<< "':"
          <<  u8" должен содержать от "<< e.arg()->minCount()
          <<  u8" до " << e.arg()->maxCount() <<  u8" значений."
          << endl;
      cout<< endl;
    }
    catch (const ArgParse::LengthErrorException<char>& e)
    {
      // Error: argument 'p [p ...]' value:
      // 'abc' string length out of range [0..2]
      cout<< "translated: "
          <<  u8"Ошибка: строковый аргумент '"
          <<  e.arg()->options()<< "':"
          <<  u8" должен быть длиной от "<< e.arg()->minValueString()
          <<  u8" до " << e.arg()->maxValueString()
          <<  u8" символов."
          << endl;
      cout<< endl;
    }
    catch (const ArgParse::InvalidArgumentException<char>& e)
    {
      // original: Error: argument '-o': invalid int value: 'x'
      cout<< "translated: "
          <<  u8"Ошибка: аргумент '"
          <<  e.arg()->options()<< "':"
          <<  u8" некорректное значение для типа "
          <<  e.arg()->typeName()
          <<  ": '"
          <<  e.value()
          <<  "'"
          <<  endl;
      cout<< endl;
    }
    catch (const ArgParse::OutOfRangeException<char>& e)
    {
      // Error: argument 'o' value: '20' out of range [5..15]
      cout<< "translated: "
          <<  u8"Ошибка: аргумент '"
          <<  e.arg()->options()<< "':"
          <<  u8" значение: '"
          <<  e.value()
          <<  u8"' выходит за границы ["
          <<  e.arg()->minValueString() << ".."
          <<  e.arg()->maxValueString() << "]"
          <<  endl;
      cout<< endl;
    }
    catch (const ArgParse::UnrecognizedArgumentsException<char>& e)
    {
      // Error: unrecognized arguments: 'd'
      cout<< "translated: "
          <<  u8"Ошибка: непредусмотренные аргументы: "
          <<  StringUtils::join(e.values(),", ",'\'','\'')
          <<  endl;
      cout<< endl;
    }
    catch (const ArgParse::InvalidChoiceException<char>& e)
    {
      // Error: invalid choice: 'cmd' (choose from 'cmd1', 'cmd2')
      cout<<  "translated: "
          <<  u8"Ошибка: неправильный выбор: '"
          <<  e.value()
          <<  u8"' (выбирете из "
          <<  StringUtils::join(e.possibleChoice(),", ",'\'','\'')
          <<  ")"
          <<  endl;
      cout<< endl;
    }
    parser.reset();
  }
  return 0;
}
//------------------------------------------------------------------



