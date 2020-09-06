#ifndef ARGUMENTPARSER_H
#define ARGUMENTPARSER_H
//----------------------------------------------------------------------------
#include <algorithm>
#include <numeric>
#include <string>
#include <vector>
#include <sstream>
#include <type_traits>
#include <memory>
#include <functional>
#include <limits>
#include <optional>
#include <type_traits>
#include <iomanip>
#include <cassert>
//----------------------------------------------------------------------------
#include "StringUtils.h"
#include "TypeUtils.h"
//----------------------------------------------------------------------------
namespace ArgParse
{
//----------------------------------------------------------------------------
using NArgs = TypeUtils::NArgs;
using StringUtils::split;
using TypeGroup = TypeUtils::Group;

template <typename T>
using TypeInfo = TypeUtils::TypeInfo<T>;

template <typename T>
using SequenceT = std::vector<T>;

template <typename StringT>
using StringsT= std::vector<StringT>;

enum class ArgType{ invalid, positional, optional };
//----------------------------------------------------------------------------
// pre define for friend access

template<typename CharT=char>
class ArgumentParser;

template<typename T, TypeGroup group, typename CharT>
class Arg;
//----------------------------------------------------------------------------
//                      ArgInfo
//----------------------------------------------------------------------------
template<typename CharT>
class ArgInfo
{
public:
  using String  = std::basic_string<CharT>;
  using Strings = StringsT<String>;

  ArgInfo(const String& shortOption,
          const String& longOption,
          bool required)
    :shortOption_(shortOption),
     longOption_(longOption),
     required_(required)
  {}

  virtual ~ArgInfo()=default;

  const String& shortOption()const { return shortOption_;  }
  const String& longOption() const { return longOption_;  }
  String options() const;

  std::size_t maxCount()const{ return maxCount_; }
  std::size_t minCount()const{ return minCount_; }

  virtual String maxValueString()const=0;
  virtual String minValueString()const=0;

  const String& help()const { return help_;  }
  void setHelp(const String& help){ help_= help; }

  virtual bool exists()const { return exists_; }
  virtual ArgType argType()const { return argType_; }

  virtual bool hasValue()const= 0;
  virtual void reset()= 0;

  virtual std::size_t typeId()const= 0;
  virtual const char* typeName()const= 0;
  virtual TypeGroup typeGroup()const=0;

protected:
  friend ArgumentParser<CharT>;

  const String shortOption_;
  const String longOption_;
  bool required_ = false;
  String help_;

  bool exists_ = false;

  std::size_t minCount_= 0;
  std::size_t maxCount_= std::numeric_limits<std::size_t>::max();

  ArgType argType_ = ArgType::invalid;

  virtual void assingOrAppendFromString(const String& str)=0;
};
//---------------------------------------------------------------------------------------
template<typename CharT>
typename ArgInfo<CharT>::String ArgInfo<CharT>::options() const
{
  if(shortOption_.empty())
    return longOption_;

  if(longOption_.empty())
    return shortOption_;

  using namespace StringUtils::literals;
  return shortOption_+"/"_lv+longOption_;
}
//---------------------------------------------------------------------------------------
//                       ArgImpl
//---------------------------------------------------------------------------------------
template<typename T, TypeGroup group, typename CharT>
class ArgImpl
    : public ArgInfo<CharT>
{
  friend Arg<T,group,CharT>;

  static constexpr const bool isSequence=
      group==TypeGroup::numbers || group==TypeGroup::strings;

public:
  using Base= ArgInfo<CharT>;
  using typename Base::String;
  using typename Base::Strings;
  using StringsConstIter= typename Base::Strings::const_iterator;

  using RangeValueType =
     std::conditional_t< TypeUtils::IsBasicStringV<T,CharT>,
                         TypeUtils::RangeTypeT<T,CharT>,
                         T>;

  using StorageType =
        std::conditional_t< isSequence,
                            SequenceT<T>,
                            std::optional<T>>;

  using ValueType =
        std::conditional_t< group==TypeGroup::number,
                            T,
                            std::conditional_t< group==TypeGroup::string,
                                                const T&,
                                                const SequenceT<T>&
                                              >
                          >;

  ArgImpl(const String& shortKey,
          const String& longKey,
          bool required)
    :Base(shortKey,longKey,required)
  {}

  ValueType storage()const
  {
    if constexpr(isSequence)
      return storage_;
    else
      return *storage_;
  };


  void assign(ValueType value)
  {
    storage_= value;
  }

  // ArgInfo

  virtual void reset() override
  {
    this->exists_ = false;
    if constexpr(isSequence)
      storage_.clear();
    else
      storage_.reset();
  }

  virtual void assingOrAppendFromString(const String& str)override;

  virtual std::size_t typeId() const  override{ return TypeInfo<T>::id; }
  virtual const char* typeName()const override{ return TypeInfo<T>::name; }
  virtual TypeGroup typeGroup()const  override{ return group; }

  virtual String minValueString()const override
  {
    return StringUtils::toString<CharT>(range_.first);
  }

  virtual String maxValueString()const override
  {
    return StringUtils::toString<CharT>(range_.second);
  }

  virtual bool hasValue()const override
  {
    if constexpr(isSequence)
      return !storage_.empty();
    else
      return storage_.has_value();
  }

private:
  std::pair<RangeValueType,RangeValueType> range_ =
      std::make_pair(std::numeric_limits<RangeValueType>::lowest(),
                     std::numeric_limits<RangeValueType>::max());

  StorageType storage_;
};
//---------------------------------------------------------------------------------------
template< typename T, TypeGroup group, typename CharT>
void ArgImpl<T, group, CharT>::
   assingOrAppendFromString(const String& str)

{
  const T value = TypeInfo<T>::assignFromString(str);
  if constexpr(group==TypeGroup::string || group==TypeGroup::strings)
  {
    if(value.length() < range_.first || value.length() > range_.second)
      throw std::length_error("length error");
  }
  else
  {
    if(value< range_.first || value> range_.second)
       throw std::range_error("range error");
  }

  if constexpr(group==TypeGroup::number || group==TypeGroup::string)
    this->storage_ = value;
  else
    this->storage_.push_back(value);
}
//---------------------------------------------------------------------------------------
//             BaseArg
//---------------------------------------------------------------------------------------
template< typename T, TypeGroup group,typename CharT>
class BaseArg
{
public:
  using Impl = ArgImpl<T,group,CharT>;
  using ValueType= typename Impl::ValueType;
  using RangeValueType= typename Impl::RangeValueType;
  using String= std::basic_string<CharT>;

  explicit BaseArg(std::shared_ptr<Impl> impl):impl_(impl){}

  bool exists()const   { return impl_->exists();   }

  bool hasValue()const { return impl_->hasValue(); }
  operator bool()const { return hasValue(); }

  ValueType operator*()const    { return   impl_->storage(); }
  const auto* operator->()const { return &(impl_->storage());}

  static constexpr TypeGroup typeGroup(){ return group; }

  void setHelp(const String& help){ impl_->setHelp(help); }

  const std::shared_ptr<Impl>& info()const{ return impl_; }

protected:
  std::shared_ptr<Impl> impl_;
};
//---------------------------------------------------------------------------------------
//                     Arg
//---------------------------------------------------------------------------------------
template< typename T, TypeGroup group, typename CharT=char>
class Arg:public BaseArg<T,group,CharT>
{
};
//---------------------------------------------------------------------------------------
template<typename T,typename CharT>
class Arg<T, TypeGroup::numbers, CharT>
    :public BaseArg<T,TypeGroup::numbers,CharT>
{
  using Base= BaseArg<T,TypeGroup::numbers,CharT>;
  using typename Base::RangeValueType;
  using typename Base::ValueType;

public:
  explicit Arg(std::shared_ptr<typename Base::Impl> impl):Base(impl){}

  ValueType values()const
  {
    return this->impl_->storage();
  }

  Arg& operator=(ValueType value)
  {
    this->impl_->assign(value);
    return *this;
  }

  void setRange(RangeValueType minValue,RangeValueType maxValue)
  {
    this->impl_->range_= std::make_pair(minValue,maxValue);
  }
};
//---------------------------------------------------------------------------------------
template<typename T,typename CharT>
class Arg<T,TypeGroup::strings,CharT>
    :public BaseArg<T,TypeGroup::strings,CharT>
{
  using Base= BaseArg<T,TypeGroup::strings,CharT>;
  using typename Base::RangeValueType;
  using typename Base::ValueType;

public:
  explicit Arg(std::shared_ptr<typename Base::Impl> impl):Base(impl){}

  ValueType values()const
  {
    return this->impl_->storage();
  }

  Arg& operator=(ValueType value)
  {
    this->impl_->assign(value);
    return *this;
  }

  void setMinLength(RangeValueType minLength)
  {
    this->impl_->range_.first = minLength;
  }

  void setMaxLength(RangeValueType maxLength)
  {
    this->impl_->range_.second= maxLength;
  }
};
//---------------------------------------------------------------------------------------
template<typename T,typename CharT>
class Arg<T,TypeGroup::number,CharT>
    :public BaseArg<T,TypeGroup::number,CharT>
{
  using Base= BaseArg<T,TypeGroup::number,CharT>;
  using typename Base::RangeValueType;
  using typename Base::ValueType;

public:
  explicit Arg(std::shared_ptr<typename Base::Impl> impl):Base(impl){}

  ValueType value()const
  {
    return this->impl_->storage();
  }

  Arg& operator=(ValueType value)
  {
    this->impl_->assign(value);
    return *this;
  }

  void setRange(RangeValueType minValue,
                RangeValueType maxValue)
  {
    this->impl_->range_= std::make_pair(minValue,maxValue);
  }
};
//---------------------------------------------------------------------------------------
template<typename T,typename CharT>
class Arg<T,TypeGroup::string,CharT>
    :public BaseArg<T,TypeGroup::string,CharT>
{
  using Base= BaseArg<T,TypeGroup::string,CharT>;
  using typename Base::RangeValueType;
  using typename Base::ValueType;

public:
  explicit Arg(std::shared_ptr<typename Base::Impl> impl):Base(impl){}

  ValueType value()const
  {
    return this->impl_->storage();
  }

  Arg& operator=(ValueType value)
  {
    this->impl_->assign(value);
    return *this;
  }

  void setMinLength(RangeValueType minLength)
  {
    this->impl_->range_.first= minLength;
  }

  void setMaxLength(RangeValueType maxLength)
  {
    this->impl_->range_.second= maxLength;
  }
};
//---------------------------------------------------------------------------------------
template<typename T, char nargs='?', typename CharT= char>
using ArgT = Arg<T,
                 TypeUtils::groupOfNArgs<T,nargs,CharT>(),
                 CharT>;

template<char nargs='?',typename CharT= char>
using IntArg = ArgT<int, nargs, CharT>;

template<char nargs='?',typename CharT= char>
using StringArg = ArgT<std::basic_string<CharT>, nargs, CharT>;
//---------------------------------------------------------------------------------------
//   Detail
//---------------------------------------------------------------------------------------
namespace detail
{
//----------------------------------------------------------------
template<typename CharT>
ArgType getArgType(const std::basic_string<CharT>& shortOption,
                   const std::basic_string<CharT>& longOption,
                   const std::basic_string<CharT>& prefixChars)
{
  using namespace std;
  using StringUtils::hasPrefix;

  if(shortOption.empty() && longOption.empty())
    return ArgType::invalid;

  const bool shortHasPrefix= hasPrefix(shortOption, prefixChars);
  const bool longHasPrefix = hasPrefix(longOption, prefixChars);

  if(!shortHasPrefix && !longHasPrefix)
     return ArgType::positional;

  if((shortHasPrefix      && longHasPrefix) ||
     (shortOption.empty() && longHasPrefix) ||
     (shortHasPrefix      && longOption.empty()))
     return ArgType::optional;

  return ArgType::invalid;
}
//----------------------------------------------------------------
template<typename CharT>
auto optionNameWithoutPrefix(const std::shared_ptr<ArgInfo<CharT>>& arg,
                             const std::basic_string<CharT>& prefixChars)
{
  using namespace std;
  using String = std::basic_string<CharT>;
  String name= arg->longOption().empty()? arg->shortOption()
                                        : arg->longOption();
  auto it = find_if_not(begin(name),
                        next(begin(name),
                        std::min(2u,name.size())),
                        [&prefixChars](CharT c)
  {
     return prefixChars.find(c)!= String::npos;
  });
  name.erase(begin(name),it);
  return name;
}
//----------------------------------------------------------------
template<typename CharT>
auto makeUsage(const std::shared_ptr<ArgInfo<CharT>>& arg,
               const std::basic_string<CharT>& prefixChars)
{
  using String= std::basic_string<CharT>;
  using namespace std;
  using namespace StringUtils::literals;
  using StringUtils::repeatString;

  String name = optionNameWithoutPrefix(arg,prefixChars);
  if(arg->argType()==ArgType::optional)
  {
    transform(begin(name),end(name),begin(name),toupper);
    return "["_lv
             +(arg->shortOption().empty()?arg->longOption()
                                         :arg->shortOption())
             +" "_lv
             +repeatString(name,arg->minCount())+
            "]"_lv;
  }
  else
  {
    return repeatString(name,arg->minCount());
  }
}
//-----------------------------------------------------------------------
template<typename CharT>
auto makeHelpLine(const std::shared_ptr<ArgInfo<CharT>>& arg,
                  const std::basic_string<CharT>& prefixChars)
{
  using namespace StringUtils::literals;
  using String = std::basic_string<CharT>;
  using detail::makeUsage;

  String helpLine;
  if(!arg->shortOption().empty())
    helpLine += arg->shortOption()+" "_lv+makeUsage(arg,prefixChars);

  if(!arg->longOption().empty())
  {
    if(!helpLine.empty())
      helpLine +=", "_lv;
    helpLine += arg->longOption()+" "_lv+makeUsage(arg,prefixChars);
  }

  if(!arg->help().empty())
    helpLine += " "_lv+ arg->help();

  return helpLine;
}
//----------------------------------------------------------------------------------
} // end namespace detail
//----------------------------------------------------------------------------------
//                        Exception<CharT>
//----------------------------------------------------------------------------------
template <typename CharT>
class Exception
{
public:
   using String = std::basic_string<CharT>;
   using ArgInfoPtr = std::shared_ptr<ArgInfo<CharT>>;

   virtual String what()const=0;
};
//----------------------------------------------------------------------------------
template <typename CharT>
class WrongCountException: public Exception<CharT>
{
public:
  using String= typename Exception<CharT>::String;
  using ArgInfoPtr= typename Exception<CharT>::ArgInfoPtr;

  WrongCountException(ArgInfoPtr arg)
    :Exception<CharT>(),
      arg_(arg)
  {
  }

  virtual String what()const override
  {
    using namespace StringUtils::literals;
    String msg=
        "argument '"_lv+arg()->options()+
        "': expected values count: "_lv+
        StringUtils::toString<CharT>(arg()->minCount());

    if(arg()->minCount() != arg()->maxCount())
      msg+= ".."_lv+ StringUtils::toString<CharT>(arg()->maxCount());

    return msg;
  }

  ArgInfoPtr arg()const{ return arg_; };
private:
  ArgInfoPtr arg_;
};
//----------------------------------------------------------------------------------
template <typename CharT>
class InvalidChoiceException: public Exception<CharT>
{
public:
  using String= typename Exception<CharT>::String;
  using Strings= StringsT<String>;
  using ArgInfoPtr= typename Exception<CharT>::ArgInfoPtr;

  InvalidChoiceException(const String& value,
                         const Strings& possibleChoice)
    :Exception<CharT>(),
     value_(value),
     possibleChoice_(possibleChoice)
  {
  }

  virtual String what()const override
  {
    using namespace std::literals;
    using namespace StringUtils::literals;
    using StringUtils::join;
    return
      "invalid choice: '"_lv+value_+"' "
      "(choose from "_lv+join(possibleChoice_,", ",'\'','\'')+")"_lv;
  }

  const String&  value()const{ return value_; }
  const Strings& possibleChoice()const{ return possibleChoice_; }

private:
  String value_;
  Strings possibleChoice_;
};
//----------------------------------------------------------------------------------
template <typename CharT>
class UnrecognizedArgumentsException: public Exception<CharT>
{
public:
  using String= typename Exception<CharT>::String;
  using Strings= StringsT<String>;

  explicit UnrecognizedArgumentsException(const Strings& values)
    :Exception<CharT>(),
     values_(values)
  {
  }

  virtual String what()const override
  {
    using namespace StringUtils::literals;
    using StringUtils::join;
    return "unrecognized arguments: "_lv +join(values_,", ",'\'','\'');
  }

  const Strings& values()const{ return values_; }

private:
  Strings values_;
};
//----------------------------------------------------------------------------------
template <typename CharT>
class ArgumentRequiredException: public Exception<CharT>
{
public:
  using typename Exception<CharT>::String;
  using typename Exception<CharT>::ArgInfoPtr;

  ArgumentRequiredException(ArgInfoPtr arg)
    :Exception<CharT>(),
     arg_(arg)
  {
  }

  virtual String what()const override
  {
    using namespace StringUtils::literals;
    return "the following argument are required: '"_lv+
            arg_->options()+"'"_lv;
  }

  ArgInfoPtr arg()const{ return arg_; };

private:
  ArgInfoPtr arg_;
};
//----------------------------------------------------------------------------------
template <typename CharT>
class ValueException: public Exception<CharT>
{
public:
   using typename Exception<CharT>::String;
   using typename Exception<CharT>::ArgInfoPtr;

   ValueException(const String& value,
                  ArgInfoPtr arg)
    :Exception<CharT>(),
     value_(value),
     arg_(arg)
    {
    }

   virtual String what()const=0;

   const String& value()const{ return value_; };
   ArgInfoPtr arg()const{ return arg_; };

private:
   String value_;
   ArgInfoPtr arg_;
};
//----------------------------------------------------------------------------------
template <typename CharT>
class OutOfRangeException: public ValueException<CharT>
{
public:
   using typename ValueException<CharT>::String;
   using typename ValueException<CharT>::ArgInfoPtr;

  OutOfRangeException(const String& value,
                      ArgInfoPtr arg)
    :ValueException<CharT>(value,arg)
  {
  }

  virtual String what()const override
  {
    using namespace StringUtils::literals;
    return
        "argument '"_lv +
        this->arg()->options() +
        "' value: '"_lv+this->value()+"' out of range ["_lv+
        this->arg()->minValueString()+".."_lv+this->arg()->maxValueString()+
        "]"_lv;
  }
};
//----------------------------------------------------------------------------------
template <typename CharT>
class InvalidArgumentException: public ValueException<CharT>
{
public:
  using typename ValueException<CharT>::String;
  using typename ValueException<CharT>::ArgInfoPtr;

  InvalidArgumentException(const String& value,
                           ArgInfoPtr arg)
    :ValueException<CharT>(value,arg)
  {
  }

  virtual String what()const override
  {
     using namespace StringUtils::literals;
     return
          "argument '"_lv+
          this->arg()->options()+
          "': invalid "_lv+
          StringUtils::LatinView(this->arg()->typeName())+
          " value: '"_lv+this->value()+"'"_lv;
  }
};
//----------------------------------------------------------------------------------
template <typename CharT>
class LengthErrorException: public ValueException<CharT>
{
public:
  using typename ValueException<CharT>::String;
  using typename ValueException<CharT>::ArgInfoPtr;

  LengthErrorException(const String& value,
                       ArgInfoPtr arg)
    :ValueException<CharT>(value,arg)
  {
  }

  virtual String what()const override
  {
    using namespace StringUtils::literals;
    return
        "argument '"_lv + this->arg()->options() +
        "' value: '"_lv + this->value()+"' "_lv+
        "string length out of range ["_lv+
        this->arg()->minValueString()+".."_lv+this->arg()->maxValueString()+
        "]"_lv;
  }
};
//----------------------------------------------------------------------------
//                        ArgumentParser
//----------------------------------------------------------------------------
template <typename CharT>
class ArgumentParser
{
public:
  using String  = std::basic_string<CharT>;
  using Strings = StringsT<String>;
  using ArgInfoPtr= std::shared_ptr<ArgInfo<CharT>>;
  using ArgumentParserPtr= std::shared_ptr<ArgumentParser<CharT>>;

  explicit ArgumentParser(const String& prefixChars=
      StringUtils::LatinView("-/"))
    :prefixChars_(prefixChars)
  {}

  virtual ~ArgumentParser()=default;

  ArgumentParserPtr addSubParser(const String& name);

  // minCount, maxCount
  template <typename T, std::size_t minCount, std::size_t maxCount>
  Arg<T, TypeUtils::groupOfMaxCount<T,maxCount,CharT>(), CharT>
    addArgument(const String& shortOption,
                const String& longOption=String(),
                bool required= false);

  // nargs
  template <typename T, NArgs nargs= NArgs::optional>
  Arg<T, TypeUtils::groupOfNArgs<T,nargs,CharT>(), CharT>
    addArgument(const String& shortKey,
                const String& longKey=String(),
                bool required = false);

  // char nargs
  template <typename T,char nargs>
  Arg<T, TypeUtils::groupOfNArgs<T,nargs,CharT>(), CharT>
    addArgument(const String& shortOption,
                const String& longOption=String(),
                bool required= false);

  void removeAllArguments();
  void removeSubParsers();
  void clear(){ removeAllArguments(); removeSubParsers(); }

  void reset();
  String help(bool recursive= false,std::size_t level=0)const;
  String usage()const;

  void parseArgs(int argc, CharT *argv[]);
  void parseArgs(int argc, const CharT *argv[]);
  void parseArgs(const Strings& args);
  void parseCmdLine(const String& str);

  void setSubParserHelp(const String& help){ help_= help; };
  bool exists() const { return exists_; }

  const String& prefixChars()const{ return prefixChars_; }

  bool argExists(const String& shortOption,
                 const String& longOption)const;

private:
  template <typename Iter>
  void parse(Iter first,Iter last);

  template <typename Iter>
  Iter pasrePositional(Iter first,Iter last);

  template <typename Iter>
  Iter parseOptional(Iter first,Iter last);

  std::shared_ptr<ArgInfo<CharT>>
     findOptionalArg(std::basic_string_view<CharT> argOption);

  String subParsersUsage()const;

  template <typename Iter>
  void assignValues(std::shared_ptr<ArgInfo<CharT>> arg,Iter first,Iter last);

private:
  std::vector<ArgInfoPtr> positional_;
  std::vector<ArgInfoPtr> optional_;
  std::vector<ArgumentParserPtr> subParsers_;

  String name_;
  String help_;
  bool exists_ = false;
  String prefixChars_;
};
//------------------------------------------------------------------
template<typename CharT>
std::shared_ptr<ArgInfo<CharT>>
ArgumentParser<CharT>::findOptionalArg(std::basic_string_view<CharT> argOption)
{
  auto it = std::find_if(std::begin(optional_), std::end(optional_),
                         [&](auto arg)
  {
    return (!arg->longOption_.empty()  && arg->longOption_ == argOption) ||
           (!arg->shortOption_.empty() && arg->shortOption_== argOption);
  });
  return it==std::end(optional_) ? nullptr : *it;
}
//------------------------------------------------------------------
template<typename CharT>
template <typename Iter>
void ArgumentParser<CharT>::assignValues(
    typename ArgumentParser<CharT>::ArgInfoPtr arg,
    Iter first, Iter last)
{
  const std::size_t count = std::distance(first,last);
  if(count < arg->minCount() || count > arg->maxCount())
  {   
    throw WrongCountException<CharT>(arg);
  }

  if(count==0)
    return;

  try
  {
     if(arg->typeGroup()==TypeGroup::number ||
        arg->typeGroup()==TypeGroup::string)
     {
        arg->assingOrAppendFromString(*first);
     }
     else
     {
       for(;first!=last; ++first)
         arg->assingOrAppendFromString(*first);
     }
  }
  catch (const std::out_of_range&)
  {
    throw OutOfRangeException<CharT>(*first,arg);
  }
  catch (const std::range_error&)
  {
    throw OutOfRangeException<CharT>(*first,arg);
  }
  catch (const std::length_error&)
  {
    throw LengthErrorException<CharT>(*first,arg);
  }
  catch (const std::invalid_argument&)
  {
   throw InvalidArgumentException<CharT>(*first,arg);
  }
}
//------------------------------------------------------------------
template<typename CharT>
template<typename Iter>
Iter ArgumentParser<CharT>::pasrePositional(Iter first, Iter last)
{
  using namespace std;

  size_t totalCount= distance(first,last);

  for(auto argIt=begin(positional_); argIt!=end(positional_); ++argIt)
  {
    (*argIt)->exists_= true;

    const size_t shouldRemain=
        accumulate(next(argIt), end(positional_), 0u,
           [](size_t sum,auto arg){ return sum + arg->minCount_; });

    const size_t available= (shouldRemain > totalCount)
        ? totalCount
        : totalCount-shouldRemain;

    const size_t count= std::min((*argIt)->maxCount(),available);
    auto lastValue= next(first,count);

    assignValues((*argIt),first,lastValue);

    first= lastValue;
    totalCount -= count;
  }

  if(first!=last)
    throw UnrecognizedArgumentsException<CharT>(Strings(first,last));

  return first;
}
//------------------------------------------------------------------
template<typename CharT>
template<typename Iter>
Iter ArgumentParser<CharT>::parseOptional(Iter first, Iter last)
{
  using namespace std;
  using namespace std::placeholders;
  using StringUtils::hasPrefix;

  while(first!=last)
  {
    auto arg = findOptionalArg(*first);
    if(!arg)
    {
      /* throw UnrecognizedArgumentsException<CharT>(Strings(first,last)); */
      return first;
    }

    arg->exists_= true;
    first= next(first);

    Iter nextOption=
       find_if(first, last, bind(hasPrefix<String>, _1, prefixChars_));

    const size_t count= distance(first,nextOption);
    const size_t currentArgCount= std::min(count,arg->maxCount());
    Iter lastValue= next(first,currentArgCount);

    assignValues(arg,first,lastValue);
    first= lastValue;
  }

  for(auto arg:optional_)
  {
    if(arg->required_ && !arg->exists_)
       throw ArgumentRequiredException<CharT>(arg);
  }
  return first;
}
//------------------------------------------------------------------
template<typename CharT>
template<typename Iter>
void ArgumentParser<CharT>::parse(Iter first, Iter last)
{
  using namespace std;
  using namespace std::placeholders;
  using StringUtils::hasPrefix;

  // find sub parser
  auto subParserIt= end(subParsers_);
  auto endOfMainParser= find_if(first,last,
      [&](const auto& s)
      {
        subParserIt= find_if(begin(subParsers_), end(subParsers_),
            [&s](auto parser)
            {
              return parser->name_== s;
            });
        return subParserIt != end(subParsers_);
      });

  // find end positional-s args
  auto endOfPositional=
    find_if(first, endOfMainParser, bind(hasPrefix<String>, _1, prefixChars_));

  pasrePositional(first, endOfPositional);
  auto it= parseOptional(endOfPositional, endOfMainParser);

  // Problems
  if(it!=endOfMainParser)
  {
    if(subParsers_.empty())
    {
      throw UnrecognizedArgumentsException<CharT>(Strings(it,endOfMainParser));
    }
    else
    {
      Strings subParsersNames;
      subParsersNames.reserve(subParsers_.size());
      transform(begin(subParsers_),
                end(subParsers_),
                back_inserter(subParsersNames),
                [](auto parser){ return parser->name_; });

      throw InvalidChoiceException<CharT>(*it,subParsersNames);
    }
  }
  // sub parser
  if(endOfMainParser!=last)
  {
    assert(subParserIt != end(subParsers_));

    (*subParserIt)->exists_= true;
    (*subParserIt)->parse(next(endOfMainParser),last);
  }
}
//------------------------------------------------------------------
template <typename CharT>
void ArgumentParser<CharT>::parseArgs(int argc, CharT *argv[])
{
  parse(argv,argv+argc);
}
//------------------------------------------------------------------
template<typename CharT>
void ArgumentParser<CharT>::parseArgs(int argc, const CharT *argv[])
{
  parse(argv,argv+argc);
}
//------------------------------------------------------------------
template<typename CharT>
void ArgumentParser<CharT>::parseArgs(const ArgumentParser::Strings &args)
{
  parse(std::begin(args),std::end(args));
}
//------------------------------------------------------------------
template<typename CharT>
void ArgumentParser<CharT>::parseCmdLine(const ArgumentParser::String &str)
{
  parseArgs(StringUtils::split<CharT,Strings>(str));
}
//------------------------------------------------------------------
template <typename CharT>
typename ArgumentParser<CharT>::ArgumentParserPtr
ArgumentParser<CharT>::addSubParser(const ArgumentParser::String &name)
{
  ArgumentParserPtr parser(new ArgumentParser);
  parser->name_= name;
  subParsers_.push_back(parser);
  return parser;
}
//----------------------------------------------------------------------------
template<typename CharT>
void ArgumentParser<CharT>::removeAllArguments()
{
  optional_.clear();
  positional_.clear();
}
//----------------------------------------------------------------------------
template<typename CharT>
void ArgumentParser<CharT>::removeSubParsers()
{
  subParsers_.clear();
}
//----------------------------------------------------------------------------
template<typename CharT>
template<typename T,  std::size_t minCount, std::size_t maxCount>
Arg<T, TypeUtils::groupOfMaxCount<T,maxCount,CharT>(), CharT>
ArgumentParser<CharT>::addArgument(
    const ArgumentParser<CharT>::String & shortOption,
    const ArgumentParser<CharT>::String & longOption,
    bool required)
{ 
  using namespace std;
  using detail::getArgType;

  static_assert(TypeUtils::TypeInfo<T>::isRegistred,
                "Not allowed type for arg!");

  static_assert(minCount<=maxCount,
               "minCount must be less or equal maxCount!");

  const ArgType argType= getArgType(shortOption, longOption, prefixChars_);

  assert(("Invalid argument!", argType!=ArgType::invalid) );
  assert(("Arg already exists!", !argExists(shortOption,longOption) ));

  constexpr const TypeGroup group=
      TypeUtils::groupOfMaxCount<T,maxCount,CharT>();

  std::shared_ptr<ArgImpl<T,group,CharT>>
      argImplPtr(new ArgImpl<T,group,CharT>(shortOption,longOption,required));
  argImplPtr->minCount_= minCount;
  argImplPtr->maxCount_= maxCount;
  argImplPtr->argType_= argType;

  if(argType == ArgType::optional)
    optional_.push_back(argImplPtr);
  else if(argType == ArgType::positional)
    positional_.push_back(argImplPtr);

  return Arg<T,group,CharT>(argImplPtr);
}
//----------------------------------------------------------------------------
template<typename CharT>
template <typename T, NArgs nargs>
Arg<T, TypeUtils::groupOfNArgs<T,nargs,CharT>(),CharT>
ArgumentParser<CharT>::addArgument(
    const ArgumentParser<CharT>::String& shortOption,
    const ArgumentParser<CharT>::String& longOption,
    bool required)
{
  [[maybe_unused]]
  constexpr const std::size_t maxCount=
       std::numeric_limits<std::size_t>::max();

  if constexpr(nargs==NArgs::optional)
      return addArgument<T,0,1>(shortOption,longOption,required);
  else if constexpr(nargs==NArgs::zeroOrMore)
      return addArgument<T,0,maxCount>(shortOption,longOption,required);
  else if constexpr(nargs==NArgs::oneOrMore)
      return addArgument<T,1,maxCount>(shortOption,longOption,required);
}
//----------------------------------------------------------------------------
template<typename CharT>
template <typename T,char nargs>
Arg<T, TypeUtils::groupOfNArgs<T,nargs,CharT>(), CharT>
ArgumentParser<CharT>::
  addArgument(const ArgumentParser<CharT>::String& shortOption,
              const ArgumentParser<CharT>::String& longOption,
              bool required)
{
  constexpr const std::size_t maxCount=
      std::numeric_limits<std::size_t>::max();

  if constexpr(nargs=='?')
      return addArgument<T,0,1>(shortOption,longOption,required);
  else if constexpr(nargs=='*')
      return addArgument<T,0,maxCount>(shortOption,longOption,required);
  else if constexpr(nargs=='+')
      return addArgument<T,1,maxCount>(shortOption,longOption,required);

  static_assert( nargs=='?' || nargs=='*' || nargs=='+',
      "Error: wrong nargs option!");
}
//------------------------------------------------------------------
template<typename CharT>
typename ArgumentParser<CharT>::String
ArgumentParser<CharT>::subParsersUsage() const
{
  using namespace StringUtils::literals;
  using StringUtils::join;

  return  "{"_lv+
          join<CharT>(subParsers_,", ",'\'','\'',[](auto p){ return p->name_; })
          +"}"_lv;
}
//---------------------------------------------------------------------------------------
template<typename CharT>
typename ArgumentParser<CharT>::String
ArgumentParser<CharT>::help(bool recursive, std::size_t level) const
{
  using namespace std;
  using namespace StringUtils::literals;

  using StringUtils::join;
  using detail::makeHelpLine;

  auto indent= [](size_t level){ return String(4*level,CharT(' ')); };

  String helpStr;
  if(!positional_.empty() || !subParsers_.empty())
  {
    helpStr+= indent(level)+"positional arguments:\n"_lv;
    for(auto arg: positional_)
      helpStr+= indent(level+1)+makeHelpLine(arg,prefixChars_)+"\n"_lv;
  }

  if(!subParsers_.empty())
  {
    helpStr+= indent(level+1)+subParsersUsage()+"\n"_lv;
    for(auto parser: subParsers_)
    {
      if(!parser->help_.empty())
        helpStr+=
           indent(level+1)+ parser->name_ +" "_lv
                          + parser->help_ +"\n"_lv;
      if(recursive)
        helpStr+= parser->help(recursive,level+2);
    }
  }

  if(!optional_.empty())
  {
    helpStr += indent(level)+"optional arguments:\n"_lv;
    for(auto arg: optional_)
      helpStr+= indent(level+1)+makeHelpLine(arg,prefixChars_)+"\n"_lv;
  }

  return helpStr;
}
//----------------------------------------------------------------------------
template<typename CharT>
typename ArgumentParser<CharT>::String
  ArgumentParser<CharT>::usage() const
{
  using namespace std;
  using namespace std::placeholders;
  using namespace StringUtils::literals;
  using StringUtils::appendJoined;  
  using detail::makeUsage;

  String usageStr;

  if(!optional_.empty())
  {
    appendJoined(usageStr,optional_," ",
                 bind(makeUsage<CharT>,_1,prefixChars_));
  }

  if(!positional_.empty())
  {
    if(!optional_.empty())
       usageStr+= " "_lv;

    appendJoined(usageStr,positional_," ",
                 bind(makeUsage<CharT>,_1,prefixChars_));
  }

  if(subParsers_.empty())
    return usageStr;

  if(!optional_.empty() || !positional_.empty())
    usageStr+= " "_lv;

  usageStr+= subParsersUsage();
  return usageStr;
}
//----------------------------------------------------------------------------
template<typename CharT>
void ArgumentParser<CharT>::reset()
{
  for(auto optPtr:optional_)
    optPtr->reset();
  for(auto posPtr:positional_)
    posPtr->reset();
  for(auto parserPtr:subParsers_)
    parserPtr->reset();
}
//-----------------------------------------------------------------
template<typename CharT>
bool ArgumentParser<CharT>::argExists(
    const ArgumentParser::String &shortOption,
    const ArgumentParser::String &longOption) const
{
  using namespace std;

  auto isMatched=
    [&shortOption,&longOption](auto arg)->bool
    {
      return
        (!arg->longOption_.empty()  &&
           (arg->longOption_==shortOption || arg->longOption_==longOption)) ||
        (!arg->shortOption_.empty() &&
           (arg->shortOption_==shortOption || arg->shortOption_==longOption));
    };

  return
     find_if(cbegin(positional_),
             cend(positional_),
             isMatched)!= cend(positional_) ||
     find_if(cbegin(optional_),
             cend(optional_),
             isMatched) != cend(optional_);
}
//----------------------------------------------------------------------------
}
//----------------------------------------------------------------------------
#endif // ARGUMENTPARSER_H
