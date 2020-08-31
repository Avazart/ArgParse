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
//----------------------------------------------------------------------------
// pre define for friend access

template<typename CharT=char>
class ArgumentParser;

template<typename T, TypeGroup group, typename CharT>
class Arg;
//----------------------------------------------------------------------------
//                      Impl
//----------------------------------------------------------------------------
template<typename CharT>
class ArgInfo
{
public:
  typedef std::basic_string<CharT> String;
  typedef std::vector<String> Strings;

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

  std::size_t maxCount()const{ return maxCount_; }
  std::size_t minCount()const{ return minCount_; }

  virtual String maxValueString()const=0;
  virtual String minValueString()const=0;

  const String& help()const { return help_;  }
  void setHelp(const String& help){ help_= help; }

  virtual bool exists()const { return exists_; } ;

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

  using StringsConstIterator = typename Strings::const_iterator;

  virtual void assingOrAppendFromString(const String& str)=0;
};
//---------------------------------------------------------------------------------------
template<typename T, TypeGroup group, typename CharT>
class ArgImpl
    : public ArgInfo<CharT>
{
  friend Arg<T,group,CharT>;

  static constexpr const bool isSequence=
      group==TypeGroup::numbers || group==TypeGroup::strings;

public:
  using String= std::basic_string<CharT>;
  using Strings= std::vector<String>;
  using StringsConstIter= typename Strings::const_iterator;

  using Base= ArgInfo<CharT>;


  using RangeValueType =
     std::conditional_t< TypeUtils::IsBasicStringV<T,CharT>,
                         TypeUtils::RangeTypeT<T,CharT>,
                         T>;

  using StorageType =
        std::conditional_t< isSequence,
                            std::vector<T>,
                            std::optional<T>>;

  using ValueType =
        std::conditional_t< group==TypeGroup::number,
                            T,
                            std::conditional_t< group==TypeGroup::string,
                                                const T&,
                                                const std::vector<T>&
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
//             Arg < >
//---------------------------------------------------------------------------------------
template< typename T, TypeGroup group,typename CharT>
class BaseArg
{
protected:
  using Impl = ArgImpl<T,group,CharT>;
  using ValueType= typename Impl::ValueType;

  std::shared_ptr<Impl> impl_;

public:
  explicit BaseArg(std::shared_ptr<Impl> impl):impl_(impl){}

  bool exists()const   { return impl_->exists();   }

  bool hasValue()const { return impl_->hasValue(); }
  operator bool()const { return hasValue(); }

  ValueType operator*()const    { return   impl_->storage(); }
  const auto* operator->()const { return &(impl_->storage());}

  static constexpr TypeGroup typeGroup(){ return group; }
};
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
  using RangeValueType= typename Base::Impl::RangeValueType;
  using ValueType= typename Base::Impl::ValueType;

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
  using RangeValueType= typename Base::Impl::RangeValueType;
  using ValueType= typename Base::Impl::ValueType;

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

  void setMinLength(RangeValueType minLength){ this->impl_->range_.first = minLength; }
  void setMaxLength(RangeValueType maxLength){ this->impl_->range_.second= maxLength; }
};
//---------------------------------------------------------------------------------------
template<typename T,typename CharT>
class Arg<T,TypeGroup::number,CharT>
    :public BaseArg<T,TypeGroup::number,CharT>
{
  using Base= BaseArg<T,TypeGroup::number,CharT>;
  using RangeValueType= typename Base::Impl::RangeValueType;
  using ValueType= typename Base::Impl::ValueType;

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
  using RangeValueType= typename Base::Impl::RangeValueType;
  using ValueType= typename Base::Impl::ValueType;

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
//----------------------------------------------------------------------------
template<typename CharT>
auto makeOptions(ArgInfo<CharT>* arg)
{
  using namespace StringUtils::literals;

  if(arg->shortOption().empty())
    return arg->longOption();

  if(arg->longOption().empty())
    return arg->shortOption();

  return arg->shortOption()+"/"_lv+arg->longOption();
}
//----------------------------------------------------------------
template<typename CharT>
auto makeUsage(ArgInfo<CharT>* arg,
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
//-----------------------------------------------------------------------
template<typename CharT>
auto makeHelpLine(ArgInfo<CharT>* arg,
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
   virtual String what()const=0;
};
//----------------------------------------------------------------------------------
template <typename CharT>
class CommonException: public Exception<CharT>
{
public:
   using String = std::basic_string<CharT>;
   using ArgInfoPtr = std::shared_ptr<ArgInfo<CharT>>;

   CommonException(const String& value,
                   ArgInfoPtr arg,
                    const String& prefixChars)
    :Exception<CharT>(),
     value_(value),
     arg_(arg),
     prefixChars_(prefixChars)
    {
    }

   virtual String what()const=0;

   const String& value()const{ return value_; };
   ArgInfoPtr arg()const{ return arg_; };
   const String& prefixChars()const{return prefixChars_;}

private:
   String value_;
   ArgInfoPtr arg_;
   String prefixChars_;
};
//----------------------------------------------------------------------------------
template <typename CharT>
class WrongCountException: public CommonException<CharT>
{
public:
   using String= typename CommonException<CharT>::String;
   using ArgInfoPtr= typename CommonException<CharT>::ArgInfoPtr;

  WrongCountException(const String& value,
                      ArgInfoPtr arg,
                      const String& prefixChars)
    :CommonException<CharT>(value,arg,prefixChars)
  {
  }

  virtual String what()const override
  {
    using namespace StringUtils::literals;
    String msg=
      "Error: argument '"_lv+
      detail::makeOptions(arg().get())+
      "': expected values count: "_lv+
      StringUtils::toString<CharT>(arg()->minCount());

    if(arg()->minCount() != arg()->maxCount())
      msg+= ".."_lv+ StringUtils::toString<CharT>(arg()->minCount());

    return msg;
  }
};
//----------------------------------------------------------------------------------
template <typename CharT>
class InvalidChoiseException: public Exception<CharT>
{
public:
   using String= typename CommonException<CharT>::String;
   using Strings= std::vector<String>;
   using ArgInfoPtr= typename CommonException<CharT>::ArgInfoPtr;

   InvalidChoiseException(const Strings& subParserNames)
    :Exception<CharT>(),
     subParserNames_(subParserNames)
   {
   }

   virtual String what()const override
   {
     using namespace StringUtils::literals;
     String msg =
        "Error: invalid choice: '3' (choose from '"_lv+
        subParserNames_[0]+"'"_lv;

     for(std::size_t i=1; i<subParserNames_.size(); ++i)
        msg +=  ", '"_lv+subParserNames_[i]+"'"_lv;
     msg += ")"_lv;

     return msg;
   }

   const Strings& subParserNames()const{ return subParserNames_; }

  private:
    Strings subParserNames_;
};
//----------------------------------------------------------------------------------
template <typename CharT>
class UnrecognizedArgumentsException: public Exception<CharT>
{
public:
  using String= typename CommonException<CharT>::String;

  explicit UnrecognizedArgumentsException(const String& str)
    :Exception<CharT>(),
     value_(str)
  {
  }

  virtual String what()const override
  {
    using namespace StringUtils::literals;
    return "Error: unrecognized arguments: '"_lv+value()+"'"_lv;
  }

  const String& value()const{ return value_; }

private:
  String value_;
};
//----------------------------------------------------------------------------------
template <typename CharT>
class ArgumentRequiredException: public CommonException<CharT>
{
public:
   using String = typename CommonException<CharT>::String;
   using ArgInfoPtr = typename CommonException<CharT>::ArgInfoPtr;

  ArgumentRequiredException(const String& value,
                      ArgInfoPtr arg,
                      const String& prefixChars)
    :CommonException<CharT>(value,arg,prefixChars)
  {
  }

  virtual String what()const override
  {
    using namespace StringUtils::literals;
    return "Error: the following argument are required:'"_lv+
            detail::makeUsage(arg().get(),prefixChars())+"'"_lv;
  }
};
//----------------------------------------------------------------------------------
template <typename CharT>
class OutOfRangeException: public CommonException<CharT>
{
public:
   using String = typename CommonException<CharT>::String;
   using ArgInfoPtr = typename CommonException<CharT>::ArgInfoPtr;

  OutOfRangeException(const String& value,
                      ArgInfoPtr arg,
                      const String& prefixChars)
    :CommonException<CharT>(value,arg,prefixChars)
  {
  }

  virtual String what()const override
  {
    using namespace StringUtils::literals;
    return
        "Error: argument '"_lv +
        detail::makeUsage(arg().get(),prefixChars())+
        "' value: '"_lv+value()+"' out of range ["_lv+
        arg()->minValueString()+".."_lv+arg()->maxValueString()+
        "]"_lv;
  }
};
//----------------------------------------------------------------------------------
template <typename CharT>
class InvalidArgumentException: public CommonException<CharT>
{
public:
  using String =     typename CommonException<CharT>::String;
  using ArgInfoPtr = typename CommonException<CharT>::ArgInfoPtr;

  InvalidArgumentException(const String& value,
                           ArgInfoPtr arg,
                           const String& prefixChars)
    :CommonException<CharT>(value,arg,prefixChars)
  {
  }

  virtual String what()const override
  {
     using namespace StringUtils::literals;
     return
          "Error: argument '"_lv+
          detail::makeOptions(arg().get())+
          "': invalid "_lv      +
          StringUtils::LatinView(arg()->typeName())+
          " value: '"_lv+value()+"'"_lv;
  }
};
//----------------------------------------------------------------------------------
template <typename CharT>
class LengthErrorException: public CommonException<CharT>
{
public:
  using String = typename CommonException<CharT>::String;
  using ArgInfoPtr = typename CommonException<CharT>::ArgInfoPtr;

  LengthErrorException(const String& value,
                       ArgInfoPtr arg,
                       const String& prefixChars)
    :CommonException<CharT>(value,arg,prefixChars)
  {
  }

  virtual String what()const override
  {
    using namespace StringUtils::literals;
    return
        "Error: argument '"_lv +
        detail::makeUsage(arg().get(),prefixChars())+
        "' value: '"_lv+value()+"' "_lv+
        "string length out of range ["_lv+
        arg()->minValueString()+".."_lv+arg()->maxValueString()+
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
  using Strings = std::vector<String>;
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

  void parseArgs(int argc, CharT *argv[]);
  void parseArgs(int argc, const CharT *argv[]);

  void parseArgs(const Strings& args);
  void parseCmdLine(const String& str);

  String help()const;
  String usage()const;
  void reset();

  void setSubParserHelp(const String& help){ help_= help; };
  bool exists() const { return exists_; }

private:
  template <typename Iter>
  void parse(Iter first,Iter last);

  template <typename Iter>
  Iter pasrePositional(Iter first,Iter last);

  template <typename Iter>
  Iter parseOptional(Iter first,Iter last);

  std::shared_ptr<ArgInfo<CharT>>
     getOptional(std::basic_string_view<CharT> argOption);

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
ArgumentParser<CharT>::getOptional(std::basic_string_view<CharT> argOption)
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
    throw WrongCountException<CharT>(*first,arg,prefixChars_);
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
    throw OutOfRangeException<CharT>(*first,arg,prefixChars_);
  }
  catch (const std::range_error&)
  {
    throw OutOfRangeException<CharT>(*first,arg,prefixChars_);
  }
  catch (const std::length_error&)
  {
    throw LengthErrorException<CharT>(*first,arg,prefixChars_);
  }
  catch (const std::invalid_argument&)
  {
   throw InvalidArgumentException<CharT>(*first,arg,prefixChars_);
  }
}
//------------------------------------------------------------------
template<typename CharT>
template<typename Iter>
Iter ArgumentParser<CharT>::pasrePositional(Iter first, Iter last)
{
  std::size_t totalCount= std::distance(first,last);

  for(auto argIt=std::begin(positional_);
      argIt!=std::end(positional_);
      ++argIt)
  {
    (*argIt)->exists_= true;

    const std::size_t shouldRemain=
        std::accumulate(std::next(argIt), std::end(positional_), 0u,
                        [](std::size_t sum,auto arg)
    {
      return sum + arg->minCount_;
    });

    const std::size_t available = (shouldRemain > totalCount)
        ? totalCount
        : totalCount-shouldRemain;

    const std::size_t count= std::min((*argIt)->maxCount(),available);
    auto lastValueOfArgIt= std::next(first,count);

    assignValues((*argIt),first,lastValueOfArgIt);

    first =lastValueOfArgIt;
    totalCount -= count;
  }
  return first;
}
//------------------------------------------------------------------
template<typename CharT>
template<typename Iter>
Iter ArgumentParser<CharT>::parseOptional(Iter first, Iter last)
{
  while(first!=last)
  {
    auto arg = getOptional(*first);
    if(!arg)
      return first;

    arg->exists_= true;
    first= std::next(first);

    auto nextOptionIt=
        std::find_if(first, last,
                     std::bind(StringUtils::isOption<String>,
                               std::placeholders::_1,
                               prefixChars_));

    const std::size_t count= std::distance(first,nextOptionIt);
    const std::size_t currentArgCount= std::min(count,arg->maxCount());
    auto lastValueOfArgIt= std::next(first,currentArgCount);

    assignValues(arg,first,lastValueOfArgIt);
    first= lastValueOfArgIt;
  }

  for(auto arg:optional_)
  {
    if(arg->required_ && !arg->exists_)
       throw ArgumentRequiredException<CharT>(*first,arg,prefixChars_);
  }
  return first;
}
//------------------------------------------------------------------
template<typename CharT>
template<typename Iter>
void ArgumentParser<CharT>::parse(Iter first, Iter last)
{
  auto endItOfPositional=
      std::find_if(first,last,
                   std::bind(StringUtils::isOption<String>,
                             std::placeholders::_1,
                             prefixChars_));

  pasrePositional(first,endItOfPositional);
  auto it= parseOptional(endItOfPositional,last);

  bool invalidChoice= false;
  if(it!=last)
  {
     if(subParsers_.empty())
        throw UnrecognizedArgumentsException<CharT>(*it);

     auto parserIt=
        std::find_if(std::begin(subParsers_),std::end(subParsers_),
                       [&it](auto parser)
        {
           return parser->name_== *it;
        });

     invalidChoice= (parserIt==std::end(subParsers_));
     if(!invalidChoice)
     {
       (*parserIt)->exists_= true;
       (*parserIt)->parse(std::next(it),last);
     }
  }
  else
  {
    invalidChoice= !subParsers_.empty();
  }

  if(invalidChoice)
  {
     Strings subParsersNames;
     subParsersNames.reserve(subParsers_.size());
     for(auto parser: subParsers_)
       subParsersNames.push_back(parser->name_);

     throw InvalidChoiseException<CharT>(subParsersNames);
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
  parseArgs(StringUtils::split(str));
}
//------------------------------------------------------------------
template <typename CharT>
typename ArgumentParser<CharT>::ArgumentParserPtr
ArgumentParser<CharT>::addSubParser(const ArgumentParser::String &name)
{
  ArgumentParserPtr parser= new ArgumentParser;
  parser->name_= name;
  subParsers_.push_back(parser);
  return parser;
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
  static_assert(TypeUtils::TypeInfo<T>::isRegistred,
      "Not allowed type for arg!");

  constexpr const TypeGroup group=
      TypeUtils::groupOfMaxCount<T,maxCount,CharT>();

  std::shared_ptr<ArgImpl<T,group,CharT>>
      argImplPtr(new ArgImpl<T,group,CharT>(shortOption,longOption,required));
  argImplPtr->minCount_= minCount;
  argImplPtr->maxCount_= maxCount;

  if(!StringUtils::isOption(shortOption, prefixChars_) &&
     !StringUtils::isOption(longOption,  prefixChars_))
    positional_.push_back(argImplPtr);
  else
    optional_.push_back(argImplPtr);

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
  constexpr const std::size_t maxCount=
      std::numeric_limits<std::size_t>::max();

  (void)maxCount;

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
  String params= "{"_lv+(subParsers_.empty()?String():subParsers_[0]->name_);
  for(std::size_t i=1; i<subParsers_.size(); ++i)
    params+= ","_lv+subParsers_[i]->name_;
  params+= "}"_lv;
  return params;
}
//---------------------------------------------------------------------------------------
template<typename CharT>
typename ArgumentParser<CharT>::String
ArgumentParser<CharT>::help() const
{
  using namespace StringUtils::literals;

  String helpStr = "\npositional arguments:\n"_lv;
  for(auto arg: positional_)
    helpStr+= "  "_lv+arg->helpLine()+"\n"_lv;

  if(!subParsers_.empty())
  {
    helpStr+= "  "_lv+subParsersUsage()+"\n"_lv;
    for(std::size_t i=0; i<subParsers_.size(); ++i)
      if(!subParsers_[i]->help_.empty())
        helpStr+= "  "_lv+ subParsers_[i]->name_ +" "_lv+
            "  "_lv+ subParsers_[i]->help_ +"\n"_lv;
  }

  helpStr += "\noptional arguments:\n"_lv;
  for(auto arg: optional_)
    helpStr+= "  "_lv+arg->helpLine()+"\n"_lv;

  return helpStr;
}
//----------------------------------------------------------------------------
template<typename CharT>
typename ArgumentParser<CharT>::String ArgumentParser<CharT>::usage() const
{
  using namespace StringUtils::literals;

  String usageStr;
  for(std::size_t i=0; i<optional_.size(); ++i)
  {
    if(i>0)
      usageStr+= " "_lv;
    usageStr+= "["_lv+ optional_[i]->shortOption_+" "_lv+
        optional_[i]->usage()+"]"_lv;
  }

  for(std::size_t i=0; i<positional_.size(); ++i)
  {
    if(i>0 || !usageStr.empty())
      usageStr+= " "_lv;
    usageStr+= positional_[i]->usage();
  }

  if(subParsers_.empty())
    return usageStr;

  if(!usageStr.empty())
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
//----------------------------------------------------------------------------
}
//----------------------------------------------------------------------------
#endif // ARGUMENTPARSER_H
