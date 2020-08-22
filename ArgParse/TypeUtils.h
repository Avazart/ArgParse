#ifndef TYPEUTILS_H
#define TYPEUTILS_H
//----------------------------------------------------------------
#include <type_traits>
#include <string>
//----------------------------------------------------------------
#include "StringUtils.h"
//----------------------------------------------------------------
namespace ArgParse::TypeUtils
{
//----------------------------------------------------------------
template<typename T,typename CharT>
constexpr bool IsBasicStringV=
    std::is_same_v<T,std::basic_string<CharT>>;
//----------------------------------------------------------------
template<typename T>
struct TypeInfo
{
   static constexpr const bool isRegistred = false;
};
//----------------------------------------------------------------
template<unsigned int num>
struct TypeCounter
{
   static constexpr const std::size_t value= TypeCounter<num-1>::value;
};

template<>
struct TypeCounter<0>
{
   static constexpr const std::size_t value= 0;
};
//----------------------------------------------------------------
#define TI_REGISTER_TYPE(TYPE, TYPE_NAME, ASSIGN_FROM_STRING)  \
    template<> \
    struct TypeCounter<__LINE__> \
    { \
       static constexpr const std::size_t value= TypeCounter<__LINE__-1>::value+1; \
    }; \
    \
    template <>  \
    struct TypeInfo<TYPE> \
    {\
       template <typename StringT> \
       static TYPE assignFromString(StringT &&s) \
       { \
         return (ASSIGN_FROM_STRING)(std::forward<StringT>(s)); \
       } \
       static constexpr const char * name = TYPE_NAME; \
       static constexpr const bool isRegistred = true; \
       static constexpr const std::size_t id = TypeCounter<__LINE__>::value;\
    };
//----------------------------------------------------------------
TI_REGISTER_TYPE(bool, "bool", StringUtils::strToBool);

TI_REGISTER_TYPE(int,      "int",      StringUtils::strToInt);
TI_REGISTER_TYPE(unsigned, "unsigned", StringUtils::strToUInt);

TI_REGISTER_TYPE(long,          "long",          StringUtils::strToLong);
TI_REGISTER_TYPE(unsigned long, "unsinged long", StringUtils::strToULong);

TI_REGISTER_TYPE(long long,          "long long",          StringUtils::strToLongLong);
TI_REGISTER_TYPE(unsigned long long, "unsinged long long", StringUtils::strToULongLong);

TI_REGISTER_TYPE(float,       "float",      StringUtils::strToFloat);
TI_REGISTER_TYPE(double,      "double",     StringUtils::strToDouble);
TI_REGISTER_TYPE(long double, "long double",StringUtils::strToLongDouble);

TI_REGISTER_TYPE(std::string,  "string",  [](auto s){ return s; } );
TI_REGISTER_TYPE(std::wstring, "wstring", [](auto s){ return s; } );
//----------------------------------------------------------------
#undef TI_REGISTER_TYPE
//----------------------------------------------------------------
enum class Case { number, numbers, string, strings };
enum class NArgs{ optional, zeroOrMore, oneOrMore  };
//----------------------------------------------------------------
template<typename T,typename CharT,std::size_t maxCount>
constexpr Case caseOfMaxCount()
{
   return (maxCount>1)
            ? (IsBasicStringV<T,CharT> ?Case::strings :Case::numbers)
            : (IsBasicStringV<T,CharT> ?Case::string  :Case::number);
};
//----------------------------------------------------------------
template<typename T,typename CharT,NArgs nargs>
constexpr Case caseOfNArgs()
{
   return (nargs!=NArgs::optional)
             ? (IsBasicStringV<T,CharT> ?Case::strings :Case::numbers)
             : (IsBasicStringV<T,CharT> ?Case::string  :Case::number);
};
//----------------------------------------------------------------
template<typename T,typename CharT,char nargs>
constexpr Case caseOfNArgs()
{
   return (nargs!='?')
           ? (IsBasicStringV<T,CharT> ?Case::strings :Case::numbers)
           : (IsBasicStringV<T,CharT> ?Case::string  :Case::number);
};
//----------------------------------------------------------------
template<typename T, typename = std::void_t<> >
struct HasValueType: std::false_type{};

template<typename T>
struct HasValueType<T,std::void_t<typename T::value_type>>: std::true_type{};

template<typename T>
inline constexpr bool HasValueTypeV= HasValueType<T>::value;
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
//----------------------------------------------------------------
} // namespase ArgParse::detail
//----------------------------------------------------------------
#endif // TYPEUTILS_H
