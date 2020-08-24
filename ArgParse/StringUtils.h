#ifndef STRINGUTILS_H
#define STRINGUTILS_H
//----------------------------------------------------------------------------
#include <string>
#include <vector>
#include <limits>
#include <stdexcept>
#include <locale>
#include <algorithm>
#include <cctype>
//----------------------------------------------------------------------------
#include "LatinView.h"
//----------------------------------------------------------------------------
namespace StringUtils
{
//----------------------------------------------------------------------------
namespace detail
{
  template<typename CharT, typename T>
  auto makeStrTo( T(*f)(const CharT *, CharT **, int) )
  {
    auto func = [f](const std::basic_string<CharT>& str)
    {
      CharT* last = nullptr;
      const CharT* end = str.c_str()+str.length();
      const T value= f(str.c_str(), &last, 10);
      if(last != end)
        throw std::invalid_argument("invalid argument");
      return value;
    };
    return func;
  }

  template<typename CharT, typename T>
  auto makeStrTo( T(*f)(const CharT *, CharT **) )
  {
    auto func = [f](const std::basic_string<CharT>& str)
    {
      CharT* last = nullptr;
      const CharT* end = str.c_str()+str.length();
      const T value= f(str.c_str(), &last);
      if(last != end)
        throw std::invalid_argument("invalid argument");
      return value;
    };
    return func;
  }

  template<typename Dest,typename CharT,typename T>
  auto makeStrTo2(T(*f)(const CharT *, CharT **, int))
  {
    auto func = [f](const std::basic_string<CharT>& str)
    {
      CharT* last = nullptr;
      const CharT* end = str.c_str()+str.length();
      const T value= f(str.c_str(), &last, 10);
      if(last != end)
        throw std::invalid_argument("invalid argument");

      if(value < std::numeric_limits<Dest>::lowest() &&
         value > std::numeric_limits<Dest>::max())
        throw std::out_of_range("out of range");

      return static_cast<Dest>(value);
    };
  return func;
 }
}
//----------------------------------------------------------------------------
template <typename S>
inline bool strToBool(const S& str)
{
  using namespace literals;
  if(str=="true"_lv  || str=="1"_lv)
    return true;
  else if(str=="false"_lv || str=="0"_lv)
    return false;

  throw std::invalid_argument("Can not convert string to bool");
}

template<typename S>
inline auto strToInt(S str){ return detail::makeStrTo2<int>(std::strtol)(str);};

template<typename S>
inline auto strToUInt(S str){ return detail::makeStrTo2<unsigned int>(std::strtoul)(str);};

template<typename S>
inline auto strToLong(S str){ return detail::makeStrTo(std::strtol)(str); };

template<typename S>
inline auto strToULong(S str){ return detail::makeStrTo(std::strtoul)(str);};

template<typename S>
inline auto strToLongLong(S str) { return detail::makeStrTo(std::strtoll)(str); };

template<typename S>
inline auto strToULongLong(S str){ return detail::makeStrTo(std::strtoull)(str); };

template<typename S>
auto strToFloat(S str){ return detail::makeStrTo(std::strtof)(str); };

template<typename CharT>
inline auto strToDouble(std::basic_string<CharT> str)
{
  if constexpr(std::is_same_v<CharT,char>)
     return detail::makeStrTo(std::strtod)(str);
  else
     return detail::makeStrTo(std::wcstod)(str);
};

template<typename S>
inline auto strToLongDouble(S str){ return detail::makeStrTo(std::strtold)(str); };
//----------------------------------------------------------------------------
template <typename CharT,typename Number>
inline std::basic_string<CharT> toString(Number number)
{
  return LatinView(std::to_string(number));
}
//----------------------------------------------------------------------------
inline int isQuote(int c)
{
  return c=='"';
}
//----------------------------------------------------------------------------
template<typename CharT>
inline std::vector<std::basic_string<CharT>>
  split(const std::basic_string<CharT> &str)
{
  using String = std::basic_string<CharT>;
  using Strings= std::vector<String>;

  Strings result;
  String  current;
  bool quoted= false;

  auto first= std::find_if_not(std::begin(str),
                               std::end(str),
                               std::isspace);
  while(true)
  {
    if(first==std::end(str))
    {
      if(!current.empty())
        result.push_back(current);
      return result;
    }
    else if(!quoted && std::isspace(*first))
    {
      result.push_back(current);
      current.clear();
      first= std::find_if_not(first,
                              std::end(str),
                              std::isspace);
      continue;
    }
    else
    {
      if(isQuote(*first))
         quoted= !quoted;
      else
         current+= *first;
      ++first;
    }
  }
  return result;
}
//----------------------------------------------------------------------------
template <typename StringT>
inline bool isOption(const StringT& str,const StringT& prefixChars)
{
  return str.size()>=2 &&
         prefixChars.find(str[0])!=StringT::npos &&
         !std::isdigit(str[1]);
}
//----------------------------------------------------------------------------
}
//----------------------------------------------------------------------------
#endif // STRINGUTILS_H
