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
template<typename T, typename D=T,typename CharT, typename F, typename WF>
D convert(const std::basic_string<CharT>& s,
   [[maybe_unused]] F f,
   [[maybe_unused]] WF wf)
{
  using namespace  std;

  using FT1= T(*)(const char *, char **, int);  // std::strtol, std::strtoll, ...
  using FT2= T(*)(const char *, char **);       // std::strtof,  std::strtod, ...

  using WFT1= T(*)(const wchar_t *, wchar_t **, int); // std::wcstol, ...
  using WFT2= T(*)(const wchar_t *, wchar_t **);      // std::wcstof, ...

  static_assert(is_same_v<F,FT1>   || is_same_v<F,FT2>,
                "f expected signature T(const char *, char **[, int])");
  static_assert(is_same_v<WF,WFT1> || is_same_v<WF,WFT2>,
                "wf expected signature T(const wchar_t *, wchar_t **[, int])");

  errno= 0;
  CharT* last = nullptr;
  const CharT* end = s.c_str()+s.length();
  T value;
  if constexpr(is_same_v<CharT,char>)
  {
    if constexpr( is_same_v<F,FT1>)
      value= f(s.c_str(), &last, 10); // FT1
    else
      value= f(s.c_str(), &last);     // FT2   floating
  }
  else
  {
    if constexpr(is_same_v<WF,WFT1>)
      value= wf(s.c_str(), &last, 10); // WFT1
    else
      value= wf(s.c_str(), &last);     // WFT2 floating
  }

  if((value==numeric_limits<T>::max() ||
      value==numeric_limits<T>::lowest()) &&
     errno==ERANGE)
  {
    throw out_of_range("out of range(errno=ERANGE)");
  }

  if(last!=end)
    throw invalid_argument("invalid argument");

  if(is_unsigned_v<T> && s[0] == CharT('-'))
    throw out_of_range("out of range");

  if constexpr(is_same_v<T,D>)
  {
    return value;
  }
  else
  {
    if(value < numeric_limits<D>::lowest() &&
       value > numeric_limits<D>::max())
      throw out_of_range("out of range");

    return static_cast<D>(value);
  }
};
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
  return detail::convert<long,int>(s,std::strtol,std::wcstol);
};

template<typename CharT>
auto strToUInt(const std::basic_string<CharT>& s)
{
  return
    detail::convert<unsigned long,unsigned int>(s,std::strtoul,std::wcstoul);
};

template<typename CharT>
auto strToLong(const std::basic_string<CharT>& s)
{
  return
    detail::convert<long>(s,std::strtol,std::wcstol);
};

template<typename CharT>
auto strToULong(const std::basic_string<CharT>& s)
{
  return detail::convert<unsigned long>(s,std::strtoul,std::wcstoul);
};

template<typename CharT>
auto strToLongLong(const std::basic_string<CharT>& s)
{
  return detail::convert<long long>(s,std::strtoll,std::wcstoll);
};

template<typename CharT>
auto strToULongLong(const std::basic_string<CharT>& s)
{
  return detail::convert<unsigned long long>(s,std::strtoull,std::wcstoull);
};

template<typename CharT>
auto strToFloat(const std::basic_string<CharT>& s)
{
  return detail::convert<float>(s,std::strtof,std::wcstof);
};

template<typename CharT>
auto strToDouble(std::basic_string<CharT> s)
{
  return detail::convert<double>(s,std::strtod,std::wcstod);
};

template<typename CharT>
auto strToLongDouble(const std::basic_string<CharT>& s)
{
  return
     detail::convert<long double>(s,std::strtold,std::wcstold);
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
             F f,
             bool skipEmptyParts= true)
{
 using namespace std;

 if(first==last)
   return;

 const auto tmp = f(*first);
 if(!empty(tmp) || !skipEmptyParts)
 {
   *out++ = leftQuote;
   copy(begin(tmp),end(tmp),out);
   *out++ = rightQuote;
 }

 for(++first; first!=last; ++first)
 {
   const auto tmp = f(*first);
   if(!empty(tmp) || !skipEmptyParts)
   {
     copy(strBegin(delemiter),strEnd(delemiter),out);
     *out++ = leftQuote;
     copy(begin(tmp),end(tmp),out);
     *out++ = rightQuote;
   }
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
             F f,
             bool skipEmptyParts= true)
{
 using namespace std;

 if(first==last)
   return;

 const auto tmp = f(*first);
 if(!empty(tmp) || !skipEmptyParts)
   copy(begin(tmp),end(tmp),out);

 for(++first; first!=last; ++first)
 {
   const auto tmp = f(*first);
   if(!empty(tmp) || !skipEmptyParts)
   {
     copy(strBegin(delemiter),strEnd(delemiter),out);
     copy(begin(tmp),end(tmp),out);
   }
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
             Q rightQuote,
             bool skipEmptyParts= true)
{
  using namespace std;

  if(first==last)
    return;

  if(!empty(*first) || !skipEmptyParts)
  {
    *out++ = leftQuote;
    copy(begin(*first),end(*first),out);
    *out++ = rightQuote;
  }

  for(++first; first!=last; ++first)
  {
    if(!empty(*first) || !skipEmptyParts)
    {
      copy(strBegin(delemiter),strEnd(delemiter),out);
      *out++ = leftQuote;
      copy(begin(*first),end(*first),out);
      *out++ = rightQuote;
    }
  }
}
//----------------------------------------------------------------------------
template <typename Iter,
          typename OutIter,
          typename D>
void joinAlg(Iter first, Iter last, OutIter out, D delemiter,
             bool skipEmptyParts= true)
{
  using namespace std;

  if(first==last)
    return;

  if(!empty(*first) || !skipEmptyParts)
    copy(begin(*first),end(*first),out);

  for(++first; first!=last; ++first)
  {
    if(!empty(*first) || !skipEmptyParts)
    {
      copy(strBegin(delemiter),strEnd(delemiter),out);
      copy(begin(*first),end(*first),out);
    }
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
                  Q rightQuote,
                  bool skipEmptyParts= true)
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
          leftQuote, rightQuote,
          skipEmptyParts);
}
//----------------------------------------------------------------------------
template <typename String,
          typename Strings,
          typename D>
void appendJoined(String& out,
                  const Strings& strings,
                  D delemiter,
                  bool skipEmptyParts= true)
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
          skipEmptyParts);
}
//----------------------------------------------------------------------------
template <typename String,
          typename Container, /* necessarily */
          typename D,
          typename Q,
          typename F>
void appendJoined(String& out,
                  const Container& container,
                  D delemiter,
                  Q leftQuote,
                  Q rightQuote,
                  F f,
                  bool skipEmptyParts= true)
{
  using namespace std;
  joinAlg(cbegin(container),cend(container),
          back_inserter(out),
          delemiter,
          leftQuote, rightQuote,
          f,
          skipEmptyParts);
}
//----------------------------------------------------------------------------
template <typename String, /* necessarily */
          typename Container,
          typename D,
          typename F>
void appendJoined(String& out,
                  const Container& container,
                  D delemiter,
                  F f,
                  bool skipEmptyParts= true)
{
  using namespace std;
  joinAlg(cbegin(container),cend(container),
          back_inserter(out),
          delemiter,
          f,
          skipEmptyParts);
}
//----------------------------------------------------------------------------
template <typename Strings,
          typename D,
          typename Q>
auto join(const Strings& strings,
          D delemiter,
          Q leftQuote,
          Q rightQuote,
          bool skipEmptyParts= true)
{
  typename Strings::value_type out;
  appendJoined(out,strings,delemiter,leftQuote,rightQuote,skipEmptyParts);
  return out;
}
//----------------------------------------------------------------------------
template <typename Strings, typename D>
auto join(const Strings& strings,D delemiter,
          bool skipEmptyParts= true)
{
  typename Strings::value_type out;
  appendJoined(out,strings,delemiter,skipEmptyParts);
  return out;
}
//----------------------------------------------------------------------------
template <typename String, /* necessarily */
          typename Container,
          typename D,
          typename Q,
          typename F>
auto join(const Container& container,
          D delemiter,
          Q leftQuote,
          Q rightQuote,
          F f,
          bool skipEmptyParts= true)
{
  String out;
  appendJoined(out,container,delemiter,leftQuote,rightQuote,f,skipEmptyParts);
  return out;
}
//----------------------------------------------------------------------------
template <typename String, /* necessarily */
          typename Container,
          typename D,
          typename F>
auto join(const Container& container,
          D delemiter,
          F f,
          bool skipEmptyParts= true)
{
  String out;
  appendJoined(out,container,delemiter,f,skipEmptyParts);
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
   out.reserve((count-1)*(size(str)+size(delemiter))
                +count>maxCount?3:0);
   out= str;

   const std::size_t m= count<=maxCount? count : 2;
   for(size_t i=1; i<m; ++i)
     out+= delemiter+str;

   if(count>maxCount)
   {
     out+= delemiter;
     out+= LatinView("...");
     out+= delemiter+str;
   }

   return out;
};
//----------------------------------------------------------------------------
}
//----------------------------------------------------------------------------
#endif // STRINGUTILS_H
