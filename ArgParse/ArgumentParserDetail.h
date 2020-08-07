#ifndef UTILS_H
#define UTILS_H
//----------------------------------------------------------------------------
#include <type_traits>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
//----------------------------------------------------------------------------
namespace argparse::detail
{
//----------------------------------------------------------------------------
template <typename CharT>
std::basic_string<CharT> toStringT(std::string_view sv)
{
  std::basic_string<CharT> r(sv.size(),0);
  for(std::size_t i=0; i<sv.size(); ++i)
     r[i]= static_cast<CharT>(sv[i]);
  return r;
}
//----------------------------------------------------------------------------
class LatinView: public std::string_view
{
  public:
      LatinView(const std::string_view& sv)
          :std::string_view(sv){}

      LatinView(const char* s, std::size_t count)
          :std::string_view(s,count){};

      LatinView(const LatinView& other)
          :std::string_view(other){};

      operator std::string() const { return std::string(begin(),end()); }

      template<typename CharT>
      operator std::basic_string<CharT>()const{ return toStringT<CharT>(*this); }

      template<typename CharT>
      friend std::basic_string<CharT> operator+(const LatinView& l,
                                                const std::basic_string<CharT>& r)
      {
        return static_cast<std::basic_string<CharT>>(l) + r;
      }

      template<typename CharT>
      friend std::basic_string<CharT> operator+(const std::basic_string<CharT>& l,
                                                const LatinView& r)
      {
        return l+static_cast<std::basic_string<CharT>>(r);
      }
};
//----------------------------------------------------------------------------
namespace literals
{
  LatinView operator "" _lv(const char *str, std::size_t len)
  {
    return LatinView(str,len);
  }
}
//----------------------------------------------------------------------------
template <typename CharT>
bool isSpace(CharT c)
{
  if constexpr(std::is_same_v<CharT,char>)
    return c==' ' || c=='\t';
  else
    return c==CharT(' ') || c==CharT('\t');
}
//----------------------------------------------------------------------------
template <typename CharT>
bool isQuote(CharT c)
{
  if constexpr(std::is_same_v<CharT,char>)
    return c=='"';
  else
    return c==CharT('"');
}
//----------------------------------------------------------------------------
template <typename CharT>
bool isOptionPrefix(CharT c)
{
  if constexpr(std::is_same_v<CharT,char>)
    return c=='-' || c=='/';
  else
    return c==CharT('-') || c==CharT('/');
}
//----------------------------------------------------------------------------
template <typename CharT>
bool isDigit(CharT c)
{
  return c>='0' && c<='9';
}
//----------------------------------------------------------------------------
template <typename StringT>
bool isOption(const StringT& str)
{
  return str.size()>=2 &&
         isOptionPrefix<typename StringT::value_type>(str[0]) &&
         !isDigit(str[1]);
}
//----------------------------------------------------------------------------
template<typename T, typename = std::void_t<> >
struct HasValueType: std::false_type{};

template<typename T>
struct HasValueType<T,std::void_t<typename T::value_type>>: std::true_type{};

template<typename T>
inline constexpr bool HasValueTypeV= HasValueType<T>::value;
//----------------------------------------------------------------------------
template<typename T, typename = std::void_t<> >
struct IsContainer: std::false_type{};

template<typename T>
struct IsContainer<T,
       std::void_t
       <
          std::void_t<typename T::value_type>,
          std::void_t<typename T::size_type>,
          decltype(std::declval<T&>().push_back(typename T::value_type())),
          decltype(std::declval<T&>().empty()),
          decltype(std::declval<T&>().size()),
          decltype(std::declval<T&>().back())
       >
    >
    :std::true_type
{
};

template<typename T>
inline constexpr bool IsContainerV= IsContainer<T>::value;
//----------------------------------------------------------------------------
template<typename T,typename CharT>
inline constexpr bool IsBasicStringV= std::is_same_v<T,std::basic_string<CharT>>;
//----------------------------------------------------------------------------
template<typename T,typename CharT,
         bool = HasValueTypeV<T> && !IsBasicStringV<T,CharT>>
struct Element
{
   using type= typename T::value_type;
};

template<typename T,typename CharT>
struct Element<T,CharT,false>
{
  using type= T;
};

template<typename T,typename CharT>
using ElementT= typename Element<T,CharT>::type;
//----------------------------------------------------------------------------
template<typename T,typename CharT,
         bool = IsBasicStringV<T,CharT>>
struct RangeType
{
   using type= typename T::size_type;
};

template<typename T,typename CharT>
struct RangeType<T,CharT,false>
{
  using type= T;
};

template<typename T,typename CharT>
using RangeTypeT= typename RangeType<T,CharT>::type;
//----------------------------------------------------------------------------
template<typename T>
struct IsNumber
     :std::bool_constant
     <
      std::is_same_v<T, bool> ||

      std::is_same_v<T, int> ||
      std::is_same_v<T, long> ||
      std::is_same_v<T, long long> ||

      std::is_same_v<T, float> ||
      std::is_same_v<T, double> ||
      std::is_same_v<T, long double>
     >
{
};
template<typename T>
inline constexpr bool IsNumberV= IsNumber<T>::value;
//----------------------------------------------------------------------------
template<typename T,typename CharT, bool = HasValueTypeV<T>>
struct IsAllowed
    : std::bool_constant
    <
       IsNumberV<T> || IsBasicStringV<T,CharT> ||
       (IsContainerV<T> && (IsNumberV<ElementT<T,CharT>> ||
                            IsBasicStringV<ElementT<T,CharT>,CharT>) )
    >
{
};

template<typename T,typename CharT>
inline constexpr bool IsAllowedV= IsAllowed<T,CharT>::value;
//----------------------------------------------------------------------------
inline bool stob(const std::string& str)
{
   if(str=="true" || str=="1")
     return true;
   else if(str=="false" || str=="0")
     return false;

  throw std::invalid_argument("Can not conver str to bool");
}
//----------------------------------------------------------------------------
enum class NArgs
{
   optional   = '?',  //  0,1
   zeroOrMore = '*',  //  0, max
   oneOrMore  = '+',  //  1, max
};
//----------------------------------------------------------------------------
enum class Case { number, numbers, string, strings };

template<typename T,typename CharT,std::size_t maxCount>
struct CaseOf
{
   static_assert(IsAllowedV<T,CharT>, "Not allowed type for arg!");

   static constexpr Case
          value =  (maxCount>1)
                   ? (IsBasicStringV<T,CharT> ? Case::strings: Case::numbers)
                   : (IsBasicStringV<T,CharT> ? Case::string: Case::number);   
};

template<typename T,typename CharT,std::size_t maxCount>
inline constexpr Case CaseOfV= CaseOf<T,CharT,maxCount>::value;
//----------------------------------------------------------------------------
template<typename T,typename CharT,NArgs nargs>
struct CaseOfNArg
{
   static_assert(IsAllowedV<T,CharT>, "Not allowed type for arg!");

   static constexpr Case
          value =  (nargs!=NArgs::optional)
                   ? (IsBasicStringV<T,CharT> ? Case::strings: Case::numbers)
                   : (IsBasicStringV<T,CharT> ? Case::string: Case::number);
};

template<typename T,typename CharT,NArgs nargs>
inline constexpr Case CaseOfNArgV= CaseOfNArg<T,CharT,nargs>::value;
//---------------------------------------------------------------------------
template<typename T,typename CharT,char nargs>
struct CaseOfCharNArg
{
   static_assert(IsAllowedV<T,CharT>, "Not allowed type for arg!");

   static constexpr Case
          value =  (nargs!='?')
                   ? (IsBasicStringV<T,CharT> ? Case::strings: Case::numbers)
                   : (IsBasicStringV<T,CharT> ? Case::string: Case::number);
};

template<typename T,typename CharT,char nargs>
inline constexpr Case CaseOfCharNArgV= CaseOfCharNArg<T,CharT,nargs>::value;

//---------------------------------------------------------------------------
template<typename CharT>
std::vector<std::basic_string<CharT>>
  split(const std::basic_string<CharT> &str)
{
  using String = std::basic_string<CharT>;
  using Strings= std::vector<String>;

  Strings result;
  String  current;
  bool quoted= false;

  auto first= std::find_if_not(std::begin(str),
                               std::end(str),
                               detail::isSpace<CharT>);
  while(true)
  {
    if(first==std::end(str))
    {
      if(!current.empty())
        result.push_back(current);
      return result;
    }
    else if(!quoted && detail::isSpace(*first))
    {
      result.push_back(current);
      current.clear();
      first= std::find_if_not(first,
                              std::end(str),
                              detail::isSpace<CharT>);
      continue;
    }
    else
    {
      if(detail::isQuote(*first))
         quoted= !quoted;
      else
         current+= *first;
      ++first;
    }
  }
  return result;
}
//---------------------------------------------------------------------------
template<typename T,typename StringT>
T convert(const StringT& str)
{
//  static_assert(IsNumberV<T> ||
//                IsBasicStringV<T,StringT::value_type>,
//                "Not allowed type for cast!");

  if constexpr(std::is_same_v<T, bool>)
     return stob(str);
  else if constexpr(std::is_same_v<T, int>)
     return std::stoi(str);
  else if constexpr(std::is_same_v<T, long>)
     return std::stol(str);
  else if constexpr(std::is_same_v<T, long long>)
     return std::stoll(str);
  else if constexpr(std::is_same_v<T, float>)
     return std::stof(str);
  else if constexpr(std::is_same_v<T, long double>)
     return std::stold(str);
  else if constexpr(std::is_same_v<T, double>)
     return std::stod(str);
  else if constexpr(std::is_same_v<T, StringT>)
     return str;
}

//----------------------------------------------------------------------------
}
//----------------------------------------------------------------------------
#endif
