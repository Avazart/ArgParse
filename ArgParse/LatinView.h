#ifndef LATINVIEW_H
#define LATINVIEW_H
//-----------------------------------------------------------
#include <string>
#include <string_view>
#include <type_traits>
#include <cstring>
#include <algorithm>
//-----------------------------------------------------------
namespace StringUtils
{
//-----------------------------------------------------------
std::size_t strLength(const char* cstr)
{
  return std::strlen(cstr);
}

std::size_t strLength(const wchar_t* cstr)
{
  return std::wcslen(cstr);
}

//std::size_t strLength(const char32_t* cstr)
//{
//  const char32_t* end = cstr;
//  while(*end++ != 0);
//  return end - cstr - 1;
//}

template <typename T>
std::size_t strLength(const T& str)
{
  return std::size(str);
}
//-----------------------------------------------------------
template <typename T>
auto strBegin(T str)
{
  if constexpr(std::is_same_v<T,const char*> ||
               std::is_same_v<T,const wchar_t*>)
    return str;
  else
    return std::begin(str);
}

template <typename T>
auto strEnd(T str)
{
  if constexpr(std::is_same_v<T,const char*> ||
               std::is_same_v<T,const wchar_t*>)
    return str+strLength(str);
  else
    return std::end(str);
}
//-----------------------------------------------------------
class LatinView: public std::string_view
{
public:
  LatinView(const std::string_view& sv)
    :std::string_view(sv){}

  LatinView(const char* s, std::size_t count)
    :std::string_view(s,count){};


  LatinView(const LatinView& other)
    :std::string_view(other){};

  // String
  operator std::string() const
  {
    return std::string(begin(),end());
  }
  // WString
  template<typename CharT>
  std::basic_string<CharT> toString()const
  {
    return std::basic_string<CharT>(begin(),end());
  }

  template<typename CharT>
  operator std::basic_string<CharT>()const
  {
    return toString<CharT>();
  }

  // EQUAL  String
  template<typename CharT>
  friend bool operator==(const std::basic_string<CharT>& str,
                         const LatinView& lv);

  template<typename CharT>
  friend bool operator==(const LatinView& lv,
                         const std::basic_string<CharT>& str);

  template<typename CharT>
  friend bool operator!=(const std::basic_string<CharT>& str,
                         const LatinView& lv);

  template<typename CharT>
  friend bool operator!=(const LatinView& lv,
                         const std::basic_string<CharT>& str);

  // EQUAL  CharT
  template<typename CharT>
  friend bool operator==(const CharT* cstr,const LatinView& lv);

  template<typename CharT>
  friend bool operator==(const LatinView& lv,const CharT* cstr);

  template<typename CharT>
  friend bool operator!=(const CharT* cstr, const LatinView& lv);

  template<typename CharT>
  friend bool operator!=(const LatinView& lv,const CharT* cstr);

  // CONCAT
  template<typename CharT>
  friend std::basic_string<CharT>
  operator+(const LatinView& l,const std::basic_string<CharT>& r);

  template<typename CharT>
  friend std::basic_string<CharT>
  operator+(const std::basic_string<CharT>& l,const LatinView& r);

private:
  template< typename CharT1,typename CharT2>
  static bool equal(const CharT1* cstr1,std::size_t size1,
                    const CharT2* cstr2,std::size_t size2);
};
//----------------------------------------------------------------------------
// EQUAL
//----------------------------------------------------------------------------
template< typename CharT1,typename CharT2>
bool LatinView::equal(const CharT1* cstr1,std::size_t size1,
                      const CharT2* cstr2,std::size_t size2)
{
  if(size1!=size2)
     return false;
  return std::equal(cstr1,cstr1+std::min(size1,size2),cstr2);
}

         // STRING
template<typename CharT>
bool operator==(const LatinView& lv,
                const std::basic_string<CharT>& str)
{
  return LatinView::equal(lv.data(),lv.size(),
                          str.data(),str.size());
}

template<typename CharT>
bool operator==(const std::basic_string<CharT>& str,
                const LatinView& lv)
{
  return LatinView::equal(str.data(),str.size(),
                          lv.data(),lv.size());
}

template<typename CharT>
bool operator!=(const LatinView& lv,
                const std::basic_string<CharT>& str)
{
  return !(lv==str);
}

template<typename CharT>
bool operator!=(const std::basic_string<CharT>& str,
                const LatinView& lv)
{
  return !(lv==str);
}
        // CharT
template<typename CharT>
bool operator==(const CharT* cstr,const LatinView& lv)
{
  return LatinView::equal(cstr,strLength(cstr),
                          lv.data(),lv.size());
}

template<typename CharT>
bool operator==(const LatinView& lv,const CharT* cstr)
{
  return LatinView::equal(cstr,strLength(cstr),
                          lv.data(),lv.size());
}

template<typename CharT>
bool operator!=(const CharT* cstr,const LatinView& lv)
{
  return !(lv==cstr);
}

template<typename CharT>
bool operator!=(const LatinView& lv,const CharT* cstr)
{
  return !(lv==cstr);
}
//----------------------------------------------------------------------------
// CONCAT
//----------------------------------------------------------------------------
template<typename CharT>
std::basic_string<CharT> operator+(const LatinView &l,
                                   const std::basic_string<CharT> &r)
{
  std::basic_string<CharT> result;
  result.reserve(l.size()+r.size());
  std::copy(std::begin(l),std::end(l),std::back_inserter(result));
  std::copy(std::begin(r),std::end(r),std::back_inserter(result));
  return result;
}
//----------------------------------------------------------------------------
template<typename CharT>
std::basic_string<CharT> operator+(const std::basic_string<CharT>& l,
                                   const LatinView& r)
{
  std::basic_string<CharT> result;
  result.reserve(l.size()+r.size());
  std::copy(std::begin(l),std::end(l),std::back_inserter(result));
  std::copy(std::begin(r),std::end(r),std::back_inserter(result));
  return result;
}
//----------------------------------------------------------------------------
namespace literals
{
  LatinView operator "" _lv(const char *str, std::size_t len)
  {
    return LatinView(str,len);
  }
}
//----------------------------------------------------------------------------
}
//----------------------------------------------------------------------------
#endif // LATINVIEW_H
