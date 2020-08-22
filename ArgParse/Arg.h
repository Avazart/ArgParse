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
#include <iomanip>
//----------------------------------------------------------------------------
#include "StringUtils.h"
#include "TypeUtils.h"
//----------------------------------------------------------------------------
namespace ArgParse
{
//----------------------------------------------------------------------------
template<typename CharT=char> class ArgumentParser;
//----------------------------------------------------------------------------
template<typename CharT>
class BaseArg
{
  public:
      typedef std::basic_string<CharT> String;
      //typedef std::basic_string_view<CharT> StringView;

      BaseArg(const String& shortOption,
              const String& longOption,
              bool required)
        :shortOption_(shortOption),
         longOption_(longOption),
         required_(required)
      {}

      virtual ~BaseArg()=default;

      const String& shortOption()const { return shortOption_;  }
      const String& longOption() const { return longOption_;  }

      std::size_t maxCount()const{ return maxCount_; }
      std::size_t minCount()const{ return minCount_; }

      const String& help()const { return help_;  }
      void setHelp(const String& help){ help_= help; }

      //
      virtual bool exists()const { return exists_; } ;

      virtual bool hasValue()const= 0;
      virtual void reset()= 0;

      virtual std::size_t typeId()const= 0;
      virtual const char* typeName()const= 0;
      virtual bool isSequence()const=0;

  protected:
      friend ArgumentParser<CharT>;

      const String shortOption_;
      const String longOption_;
      bool required_ = false;
      String help_;

      bool exists_ = false;

      std::size_t minCount_= 0;
      std::size_t maxCount_= std::numeric_limits<std::size_t>::max();

      virtual bool tryAssignOrAppend(const String& str,
                                     const String& prefixChars,
                                     String& error)=0;
};
//---------------------------------------------------------------------------------------
template<typename CharT>
auto makeOptions(BaseArg<CharT>* arg);

template<typename CharT>
auto makeOptions(BaseArg<CharT>* arg);

template<typename CharT>
auto makeHelpLine(BaseArg<CharT>* arg,
                  const std::basic_string<CharT>& prefixChars);
//---------------------------------------------------------------------------------------
template< typename T, typename CharT, TypeUtils::Case case_>
class ArgImpl
    : public BaseArg<CharT>
{
     static constexpr const bool isSequence_=
         case_==TypeUtils::Case::numbers ||
         case_==TypeUtils::Case::strings;

  public:
     using String = std::basic_string<CharT>;
     using RangeValueType = std::conditional_t<
                            TypeUtils::IsBasicStringV<T,CharT>,
                            TypeUtils::RangeTypeT<T,CharT>,
                            T>;
     using StorageType = std::conditional_t<isSequence_,
                                            std::vector<T>,
                                            std::optional<T>>;

     ArgImpl(const String& shortKey,
            const String& longKey,
            bool required)
            :BaseArg<CharT>(shortKey,longKey,required)
            {}

     virtual bool hasValue()const override
     {
       if constexpr(isSequence_)
          return !storage_.empty();
       else
          return storage_.has_value();
     }

     virtual void reset()override
     {
       if constexpr(isSequence_)
          storage_.clear();
       else
          storage_.reset();
     }

     virtual std::size_t typeId() const override
     {
       return TypeUtils::TypeInfo<T>::id;
     }

     virtual const char* typeName()const override
     {
       return TypeUtils::TypeInfo<T>::name;
     }

     virtual bool isSequence()const override
     {
       return isSequence_;
     }

     virtual bool tryAssignOrAppend(const String& str,
                                    const String& prefixChars,
                                    String& error) override;

   protected:
       std::pair<RangeValueType,RangeValueType> range_ =
             std::make_pair(std::numeric_limits<RangeValueType>::lowest(),
                            std::numeric_limits<RangeValueType>::max());

       StorageType storage_;
};
//---------------------------------------------------------------------------------------
template< typename T, typename CharT, TypeUtils::Case case_>
bool ArgImpl<T,CharT,case_>::tryAssignOrAppend(
      const ArgImpl<T,CharT,case_>::String& str,
      const ArgImpl<T,CharT,case_>::String& prefixChars,
      ArgImpl<T,CharT,case_>::String& error)
{
  using namespace StringUtils::literals;
  try
  {
    const T value = TypeUtils::TypeInfo<T>::assignFromString(str);
    if constexpr(case_==TypeUtils::Case::string ||
                 case_==TypeUtils::Case::strings)
    {
      if(value.length()< range_.first ||
         value.length()> range_.second)
        throw std::out_of_range("out of range");
    }
    else
    {
      if(value< range_.first || value> range_.second)
        throw std::out_of_range("out of range");
    }

    if constexpr(case_==TypeUtils::Case::numbers ||
                 case_==TypeUtils::Case::strings)
        storage_.push_back(value);
    else
        storage_ = value;
    return true;
  }
  catch (const std::out_of_range&)
  {
    error= "Error: argument '"_lv +
           makeUsage(this,prefixChars)+ // !!!
           "' value: '"_lv+str+"' "_lv;

    if constexpr(case_==TypeUtils::Case::string ||
                 case_==TypeUtils::Case::strings)
       error += "string length "_lv;

    error+= "out of range ["_lv+
            StringUtils::toString<CharT>(this->range_.first)+
            ".."_lv+
            StringUtils::toString<CharT>(this->range_.second)+
            "]"_lv;

    return false;
  }
  catch (const std::invalid_argument& )
  {
    error=  "Error: argument '"_lv+
            makeOptions(this)+
            "': invalid "_lv      +
            StringUtils::LatinView(typeName())+
            " value: '"_lv+str+"'"_lv;
    return false;
  }
  return true;
}
//---------------------------------------------------------------------------------------
// Arg
//---------------------------------------------------------------------------------------
template< typename T, typename CharT, TypeUtils::Case case_>
class Arg: public ArgImpl<T,CharT,case_>
{

};
//---------------------------------------------------------------------------------------
template< typename T, typename CharT>
class Arg<T,CharT,TypeUtils::Case::numbers>
    :public ArgImpl<T,CharT,TypeUtils::Case::numbers>
{
  public:
     using String = std::basic_string<CharT>;
     using Base = ArgImpl<T,CharT,TypeUtils::Case::numbers>;
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
class Arg<T,CharT,TypeUtils::Case::strings>
    :public ArgImpl<T,CharT,TypeUtils::Case::strings>
{
  public:
     using String = std::basic_string<CharT>;
     using Base = ArgImpl<T,CharT,TypeUtils::Case::strings>;
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
class Arg<T,CharT,TypeUtils::Case::number>:
     public ArgImpl<T,CharT,TypeUtils::Case::number>
{
  public:
     using String = std::basic_string<CharT>;
     using Base = ArgImpl<T,CharT,TypeUtils::Case::number>;
     using RangeValueType = typename Base::RangeValueType;

     Arg(const String& shortKey,
         const String& longKey,
         bool required)
     :Base(shortKey,longKey,required)
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
class Arg<T,CharT,TypeUtils::Case::string>:
     public ArgImpl<T,CharT,TypeUtils::Case::string>
{
  public:
     using String = std::basic_string<CharT>;
     using Base = ArgImpl<T,CharT,TypeUtils::Case::string>;
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
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
template<typename CharT>
auto makeOptions(BaseArg<CharT>* arg)
{
  using namespace StringUtils::literals;

  if(arg->shortOption().empty())
    return arg->longOption();

  if(arg->longOption().empty())
    return arg->shortOption();

  return arg->shortOption()+"/"_lv+arg->longOption();
}
//---------------------------------------------------------------------------------------
template<typename CharT>
auto makeUsage(BaseArg<CharT>* arg,
           const std::basic_string<CharT>& prefixChars)
{
  using String = std::basic_string<CharT>;
  using namespace StringUtils::literals;

  String name_ = arg->longOption().empty()
                 ? arg->shortOption()
                 : arg->longOption();

  auto it = std::find_if_not(std::begin(name_),
                             std::next(std::begin(name_),
                                       std::min(2u,name_.size())),
                             [&prefixChars](CharT c)
                             {
                               return prefixChars.find(c)!= String::npos;
                             });

  name_.erase(std::begin(name_),it);
  if(arg->maxCount()>1)
     name_ += " ["_lv+name_+" ...]"_lv;
  return  name_;
}
//---------------------------------------------------------------------------------------
template<typename CharT>
auto makeHelpLine(BaseArg<CharT>* arg,
                  const std::basic_string<CharT>& prefixChars)
{
  using namespace StringUtils::literals;
  using String = std::basic_string<CharT>;

  String helpLine;
  if(!arg->shortOption().empty())
    helpLine += arg->shortOption()+" "_lv+makeUsage(prefixChars);

  if(!arg->longOption().empty())
  {
    if(!helpLine.empty())
       helpLine +=", "_lv;
    helpLine += arg->longOption()+" "_lv+makeUsage(prefixChars);
  }

  if(!arg->help().empty())
    helpLine += " "_lv+ arg->help();

  return helpLine;
}
//-------------------------------------------------------------------
}
//-------------------------------------------------------------------
#endif // ARG_H
