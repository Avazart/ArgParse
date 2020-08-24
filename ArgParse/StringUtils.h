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
  template<typename F, typename WF, typename T, typename D=T>
  class Converter
  {
      using FT1 =  T(const char *, char **, int);  // std::strtol,
      using FT2 =  T(const char *, char **);       // std::strtof,

      using WFT1 = T(const wchar_t *, wchar_t **, int); // std::wcstol
      using WFT2 = T(const wchar_t *, wchar_t **);      // std::wcstof

      static_assert(std::is_same_v<F,FT1>   ||  std::is_same_v<F,FT2>);
      static_assert(std::is_same_v<WF,WFT1> ||  std::is_same_v<WF,WFT2>);

    public:
       Converter(F* f,WF* wf)
         :f_(f),wf_(wf)
       {}

       template<typename CharT>
       D operator()(const std::basic_string<CharT>& s)const
       {
         CharT* last = nullptr;
         const CharT* end = s.c_str()+s.length();

         T value;
         if constexpr(std::is_same_v<CharT,char>)
         {
           if constexpr(std::is_same_v<F,FT1>)
             value= f_(s.c_str(), &last, 10); // FT1
           else
             value= f_(s.c_str(), &last);     // FT2
         }
         else
         {
           if constexpr(std::is_same_v<WF,WFT1>)
             value= wf_(s.c_str(), &last, 10); // WFT1
           else
             value= wf_(s.c_str(), &last);     // WFT2
         }

         if(last!=end)
           throw std::invalid_argument("invalid argument");

         if constexpr(std::is_same_v<T,D>)
         {
            return value;
         }
         else
         {
           if(value < std::numeric_limits<D>::lowest() &&
              value > std::numeric_limits<D>::max())
             throw std::out_of_range("out of range");

           return static_cast<D>(value);
         }
       }

   private:
      F*  f_ = nullptr;
      WF* wf_= nullptr;
  };

  template<typename T, typename D=T,typename F, typename WF>
  auto makeConverter(F* f, WF* wf)
  {
    return Converter<F,WF,T,D>(f,wf);
  }
}
//----------------------------------------------------------------------------
template<typename CharT>
bool strToBool(const std::basic_string<CharT>& s)
{
  using namespace literals;
  if(s=="true"_lv  || s=="1"_lv)
    return true;
  else if(s=="false"_lv || s=="0"_lv)
    return false;

  throw std::invalid_argument("Can not convert string to bool");
}

template<typename CharT>
auto strToInt(const std::basic_string<CharT>& s)
{
  return detail::makeConverter<long,int>(std::strtol,std::wcstol)(s);
};

template<typename CharT>
auto strToUInt(const std::basic_string<CharT>& s)
{
  return
     detail::makeConverter<unsigned long,
                           unsigned int>(std::strtoul,
                                         std::wcstoul)(s);
};

template<typename CharT>
auto strToLong(const std::basic_string<CharT>& s)
{
  return
     detail::makeConverter<long>(std::strtol,
                                 std::wcstol)(s);
};

template<typename CharT>
auto strToULong(const std::basic_string<CharT>& s)
{
  return
   detail::makeConverter<unsigned long>(std::strtoul,
                                        std::wcstoul)(s);
};

template<typename CharT>
auto strToLongLong(const std::basic_string<CharT>& s)
{
  return detail::makeConverter<long long>(std::strtoll,
                                          std::wcstoll)(s);
};

template<typename CharT>
auto strToULongLong(const std::basic_string<CharT>& s)
{
  return
    detail::makeConverter<unsigned long long>(std::strtoull,
                                              std::wcstoull)(s);
};

template<typename CharT>
auto strToFloat(const std::basic_string<CharT>& s)
{
  return
    detail::makeConverter<float>(std::strtof,
                                 std::wcstof)(s);
};

template<typename CharT>
auto strToDouble(std::basic_string<CharT> s)
{
  return
    detail::makeConverter<double>(std::strtod,
                                  std::wcstod)(s);
};

template<typename CharT>
auto strToLongDouble(const std::basic_string<CharT>& s)
{
  return
     detail::makeConverter<long double>(std::strtold,
                                        std::wcstold)(s);
};
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
