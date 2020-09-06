#ifndef STRINGUTILS_H
#define STRINGUTILS_H
//----------------------------------------------------------------------------
#include <string>
#include <vector>
#include <limits>
#include <stdexcept>
#include <locale>
#include <algorithm>
#include <numeric>
#include <cctype>
#include <cerrno>
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
  using FT1 =  T(const char *, char **, int);  // std::strtol, std::strtoll, ...
  using FT2 =  T(const char *, char **);       // std::strtof,  std::strtod, ...

  using WFT1 = T(const wchar_t *, wchar_t **, int); // std::wcstol, ...
  using WFT2 = T(const wchar_t *, wchar_t **);      // std::wcstof, ...

  static_assert(std::is_same_v<F,FT1>   ||  std::is_same_v<F,FT2>);
  static_assert(std::is_same_v<WF,WFT1> ||  std::is_same_v<WF,WFT2>);

public:
  Converter(F* f,WF* wf)
    :f_(f),wf_(wf)
  {}

  template<typename CharT>
  D operator()(const std::basic_string<CharT>& s)const
  {
    errno= 0;
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

    if((value == std::numeric_limits<T>::max() ||
        value == std::numeric_limits<T>::lowest()) &&
        errno == ERANGE)
    {
      throw std::out_of_range("out of range(errno=ERANGE)");
    }

    if(last!=end)
      throw std::invalid_argument("invalid argument");

    if(std::is_unsigned_v<T> && s[0] == CharT('-'))
      throw std::out_of_range("out of range");

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
//----------------------------------------------------------------------------
template<typename T, typename D=T,typename F, typename WF>
auto makeConverter(F* f, WF* wf)
{
  return Converter<F,WF,T,D>(f,wf);
}
//----------------------------------------------------------------------------
}  // end namespace detail
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
template<typename CharT,
         typename Strings=std::vector<std::basic_string<CharT>>>
Strings split(const std::basic_string<CharT> &str)
{
  using namespace std;
  using String = basic_string<CharT>;

  Strings result;
  String  current;
  bool quoted= false;

  auto first=
    find_if_not(begin(str),end(str),isspace);

  while(true)
  {
    if(first==end(str))
    {
      if(!current.empty())
        result.push_back(current);
      return result;
    }
    else if(!quoted && isspace(*first))
    {
      result.push_back(current);
      current.clear();
      first= find_if_not(first, end(str), isspace);
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
template <typename String>
bool hasPrefix(const String& str,const String& prefixChars)
{
  return str.size()>=2 &&
         prefixChars.find(str[0])!=String::npos &&
         !std::isdigit(str[1]);
}
//----------------------------------------------------------------------------
template <typename Iter,
          typename OutIter,
          typename D,
          typename Q,
          typename F>
void joinAlg(Iter first, Iter last,
             OutIter out,
             D delemiter,
             Q leftQuote,
             Q rightQuote,
             F f)
{
 using namespace std;

 if(first==last)
   return;

 *out++ = leftQuote;
 const auto tmp = f(*first);
 copy(begin(tmp),end(tmp),out);
 *out++ = rightQuote;

 for(++first; first!=last; ++first)
 {
   copy(strBegin(delemiter),strEnd(delemiter),out);

   *out++ = leftQuote;
   const auto tmp = f(*first);
   copy(begin(tmp),end(tmp),out);
   *out++ = rightQuote;
 }
}
//----------------------------------------------------------------------------
template <typename Iter,
          typename OutIter,
          typename D,
          typename F>
void joinAlg(Iter first, Iter last,
             OutIter out,
             D delemiter,
             F f)
{
 using namespace std;

 if(first==last)
   return;

 const auto tmp = f(*first);
 copy(begin(tmp),end(tmp),out);

 for(++first; first!=last; ++first)
 {
   copy(strBegin(delemiter),strEnd(delemiter),out);
   const auto tmp = f(*first);
   copy(begin(tmp),end(tmp),out);
 }
}
//----------------------------------------------------------------------------
template <typename Iter,
          typename OutIter,
          typename D,
          typename Q>
void joinAlg(Iter first, Iter last,
             OutIter out,
             D delemiter,
             Q leftQuote,
             Q rightQuote)
{
  using namespace std;

  if(first==last)
    return;

  *out++ = leftQuote;
  copy(begin(*first),end(*first),out);
  *out++ = rightQuote;

  for(++first; first!=last; ++first)
  {
    copy(strBegin(delemiter),strEnd(delemiter),out);

    *out++ = leftQuote;
    copy(begin(*first),end(*first),out);
    *out++ = rightQuote;
  }
}
//----------------------------------------------------------------------------
template <typename String,
          typename Strings,
          typename D,
          typename Q>
void appendJoined(String& out,
                  const Strings& strings,
                  D delemiter,
                  Q leftQuote,
                  Q rightQuote)
{
  using namespace std;

  const size_t totalSize =
    accumulate(begin(strings),
               end(strings),
               (size(strings)-1)+strLength(delemiter)+2,
               [](size_t l,const auto& r)
               {
                 return l+size(r);
               });
  out.reserve(out.size()+totalSize);

  joinAlg(cbegin(strings),cend(strings),
          back_inserter(out),
          delemiter,
          leftQuote, rightQuote);
}
//----------------------------------------------------------------------------
template <typename String,
          typename Container,
          typename D,
          typename Q,
          typename F>
void appendJoined(String& out,
                  const Container& container,
                  D delemiter,
                  Q leftQuote,
                  Q rightQuote,
                  F f)
{
  using namespace std;
  joinAlg(cbegin(container),cend(container),
          back_inserter(out),
          delemiter,
          leftQuote, rightQuote,
          f);
}
//----------------------------------------------------------------------------
template <typename String,
          typename Container,
          typename D,
          typename F>
void appendJoined(String& out,
                  const Container& container,
                  D delemiter,
                  F f)
{
  using namespace std;
  joinAlg(cbegin(container),cend(container),
          back_inserter(out),
          delemiter,
          f);
}
//----------------------------------------------------------------------------
template <typename Strings,
          typename D,
          typename Q>
auto join(const Strings& strings,
          D delemiter,
          Q leftQuote,
          Q rightQuote)
{
  typename Strings::value_type out;
  appendJoined(out,strings,delemiter,leftQuote,rightQuote);
  return out;
}
//----------------------------------------------------------------------------
template <typename CharT,
          typename Container,
          typename D,
          typename Q,
          typename F>
auto join(const Container& container,
          D delemiter,
          Q leftQuote,
          Q rightQuote,
          F f)
{
  std::basic_string<CharT> out;
  appendJoined(out,container,delemiter,leftQuote,rightQuote,f);
  return out;
}
//----------------------------------------------------------------------------
template <typename CharT,
          typename Container,
          typename D,
          typename F>
auto join(const Container& container,
          D delemiter,
          F f)
{
  std::basic_string<CharT> out;
  appendJoined(out,container,delemiter,f);
  return out;
}
//----------------------------------------------------------------------------
template <typename String, typename D= LatinView>
auto repeatString(const String& str,
                  std::size_t count,
                  std::size_t maxCount= 5,
                  D delemiter= LatinView(" "))
{
   String out;
   out.reserve( (count-1)*(size(str)+size(delemiter))
                +count>maxCount?3:0);
   out= str;

   for(size_t i=1; i<std::min(count,maxCount); ++i)
     out+= delemiter+str;

   if(count>maxCount)
   {
     out+= delemiter;
     out+= LatinView("...");
   }

   return out;
};
//----------------------------------------------------------------------------
}
//----------------------------------------------------------------------------
#endif // STRINGUTILS_H
