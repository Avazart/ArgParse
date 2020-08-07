#ifndef ARG_H
#define ARG_H
//----------------------------------------------------------------------------
#include <string>
#include <sstream>
#include <vector>
#include <limits>
#include <algorithm>
#include <optional>
#include <type_traits>
//----------------------------------------------------------------------------
#include "ArgumentParserDetail.h"
//----------------------------------------------------------------------------
namespace argparse
{
//----------------------------------------------------------------------------
template<typename CharT=char> class ArgumentParser;
//----------------------------------------------------------------------------
template<typename CharT>
class BaseArg
{
  public:
      typedef std::basic_string<CharT> String;

      BaseArg(const String& shortKey,
              const String& longKey,
              bool required)
        :shortKey_(shortKey),
         longKey_(longKey),
         required_(required)
      {}

      virtual ~BaseArg()=default;

      void setDescription(const String& description){ description_= description; }
      virtual bool hasValue()const=0;

      virtual bool exists()const { return exists_; } ;
      virtual const char* typeName()const= 0;

      String name()
      {
        using namespace detail::literals;

        if(shortKey_.empty() || longKey_.empty())
          return shortKey_+longKey_;

        return  shortKey_+"/"_lv+longKey_;
      }

  protected:
      friend ArgumentParser<CharT>;

      const String shortKey_;
      const String longKey_;
      bool required_ = false;
      String description_;

      bool exists_ = false;

      std::size_t minCount_= 0;
      std::size_t maxCount_= std::numeric_limits<std::size_t>::max();

      virtual bool tryAssignOrAppend(const String& str, String& error)=0;
};
//---------------------------------------------------------------------------------------
template< typename T, typename CharT,detail::Case case_>
class ArgImpl
    : public BaseArg<CharT>
{
  public:
     static constexpr bool isContainer=
        case_==detail::Case::numbers || case_==detail::Case::strings;

     using String = std::basic_string<CharT>;
     using RangeValueType = std::conditional_t<
                            detail::IsBasicStringV<T,CharT>,
                            detail::RangeTypeT<T,CharT>,
                            T>;

     using StorageType = std::conditional_t<isContainer,
                                            std::vector<T>,
                                            std::optional<T>>;

     ArgImpl(const String& shortKey,
            const String& longKey,
            bool required)
            :BaseArg<CharT>(shortKey,longKey,required)
            {}

     virtual bool hasValue()const override
     {
       if constexpr(isContainer)
           return !storage_.empty();
       else
           return storage_.has_value();
     };

     virtual const char* typeName()const override
     {
       return typeid (T).name();
     };

     virtual bool tryAssignOrAppend(const String& str, String& error) override;

   protected:
       std::pair<RangeValueType,RangeValueType> range_ =
             std::make_pair(std::numeric_limits<RangeValueType>::lowest(),
                            std::numeric_limits<RangeValueType>::max());

       StorageType storage_;
};
//---------------------------------------------------------------------------------------
template< typename T, typename CharT,detail::Case case_>
bool ArgImpl<T,CharT,case_>::tryAssignOrAppend(
      const ArgImpl<T,CharT,case_>::String& str,
      ArgImpl<T,CharT,case_>::String& error)
{
  using namespace detail::literals;
  try
  {
    const T value = detail::convert<T>(str);

    if constexpr(case_==detail::Case::string || case_==detail::Case::strings)
    {
      if(value.length()< range_.first || value.length()> range_.second)
        throw std::out_of_range("");
        // std::length_error()
    }
    else
    {
      if(value< range_.first || value> range_.second)
        throw std::out_of_range("");
        // std::range_error()
    }

    if constexpr(case_==detail::Case::numbers ||  case_==detail::Case::strings)
        storage_.push_back(value);
    else
        storage_ = value;
    return true;
  }
  catch (const std::out_of_range&)
  {
    error = "Error: argument '"_lv + this->name()+"' value: '"_lv+str+"' "_lv;

    if constexpr(case_==detail::Case::string ||
                 case_==detail::Case::strings)
       error += "string length "_lv;

    error+= "out of range ["_lv+
               detail::toStringT<CharT>(std::to_string(this->range_.first))+
               ".."_lv+
               detail::toStringT<CharT>(std::to_string(this->range_.second))+
               "]"_lv;

    return false;
  }
  catch (const std::invalid_argument& )
  {
     error= "Error: argument '"_lv + this->name()+
            "': invalid "_lv       + detail::toStringT<CharT>(typeName())+
            " value: '"_lv + str+"'"_lv;;
    return false;
  }
  return true;
}
//---------------------------------------------------------------------------------------
// Arg
//---------------------------------------------------------------------------------------
template< typename T, typename CharT, detail::Case case_>
class Arg: public ArgImpl<T,CharT,case_>
{
};
//---------------------------------------------------------------------------------------
template< typename T, typename CharT>
class Arg<T,CharT,detail::Case::numbers>
    :public ArgImpl<T,CharT,detail::Case::numbers>
{
  public:
     using String = std::basic_string<CharT>;
     using Base = ArgImpl<T,CharT,detail::Case::numbers>;
     using RangeValueType = typename Base::RangeValueType;

     Arg(const String& shortKey,
         const String& longKey,
         bool required)
     :Base(shortKey,longKey,required)
     {}

     const std::vector<T>& values()const
     {
       return this->storage_;
     }

     void setRange(RangeValueType minValue,RangeValueType maxValue)
     {
       this->range_ = std::make_pair(minValue,maxValue);
     }
};
//---------------------------------------------------------------------------------------
template< typename T, typename CharT>
class Arg<T,CharT,detail::Case::strings>
    :public ArgImpl<T,CharT,detail::Case::strings>
{
  public:
    using String = std::basic_string<CharT>;
    using Base = ArgImpl<T,CharT,detail::Case::strings>;
    using RangeValueType = typename Base::RangeValueType;

     Arg(const String& shortKey,
         const String& longKey,
         bool required)
     :Base(shortKey,longKey,required)
     {}

     const std::vector<String>& values()const
     {
       return this->storage_;
     }

     void setMinLength(RangeValueType minLength){ this->range_.first = minLength; }
     void setMaxLength(RangeValueType maxLength){ this->range_.second= maxLength; }
};
//---------------------------------------------------------------------------------------
template< typename T, typename CharT>
class Arg<T,CharT,detail::Case::number>:
     public ArgImpl<T,CharT,detail::Case::number>
{
  public:
     using String = std::basic_string<CharT>;
     using Base = ArgImpl<T,CharT,detail::Case::number>;
     using RangeValueType = typename Base::RangeValueType;

     Arg(const String& shortKey,
         const String& longKey,
         bool required)
     :BaseArg<CharT>(shortKey,longKey,required)
     {}

     T value() const
     {
       return *this->storage_;
     }

     void setRange(RangeValueType minValue,RangeValueType maxValue)
     {
       this->range_ = std::make_pair(minValue,maxValue);
     }
};
//---------------------------------------------------------------------------------------
template< typename T, typename CharT>
class Arg<T,CharT,detail::Case::string>:
     public ArgImpl<T,CharT,detail::Case::string>
{
  public:
     using String = std::basic_string<CharT>;
     using Base = ArgImpl<T,CharT,detail::Case::string>;
     using RangeValueType = typename Base::RangeValueType;

     Arg(const String& shortKey,
         const String& longKey,
         bool required)
     :Base(shortKey,longKey,required)
     {}

     const String& value() const
     {
       return *this->storage_;
     }

     void setMinLength(RangeValueType minLength){ this->range_.first = minLength; }
     void setMaxLength(RangeValueType maxLength){ this->range_.second= maxLength; }
};
//-------------------------------------------------------------------
}
//-------------------------------------------------------------------
#endif // ARG_H
