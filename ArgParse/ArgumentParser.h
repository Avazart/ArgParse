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
class BaseArgImpl
{
  public:
      typedef std::basic_string<CharT> String;
      typedef std::vector<String> Strings;

      BaseArgImpl(const String& shortOption,
              const String& longOption,
              bool required)
        :shortOption_(shortOption),
         longOption_(longOption),
         required_(required)
      {}

      virtual ~BaseArgImpl()=default;

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

      virtual bool tryAssingFromStrings(StringsConstIterator first,
                                        StringsConstIterator last,
                                        const String& prefixChars,
                                        String& error)=0;

      virtual bool tryAssingFromStrings(const CharT** first,
                                        const CharT** last,
                                        const String& prefixChars,
                                        String& error)=0;

};
//---------------------------------------------------------------------------------------
template<typename T, TypeGroup group, typename CharT>
class ArgImpl
    : public BaseArgImpl<CharT>
{
     friend Arg<T,group,CharT>;

     static constexpr const bool isSequence=
         group==TypeGroup::numbers || group==TypeGroup::strings;

  public:
     using String = std::basic_string<CharT>;
     using Strings = std::vector<String>;
     using StringsConstIter = typename Strings::const_iterator;

     using Base =   BaseArgImpl<CharT>;


     using RangeValueType = std::conditional_t<
                            TypeUtils::IsBasicStringV<T,CharT>,
                            TypeUtils::RangeTypeT<T,CharT>,
                            T>;

     using StorageType = std::conditional_t<isSequence,
                                            std::vector<T>,
                                            std::optional<T>>;

     using ValueType =
       std::conditional_t<
            group==TypeGroup::number,
            T,
            std::conditional_t<
                group==TypeGroup::string,
                const T&,
                const std::vector<T>&
           >
       >;

     ArgImpl(const String& shortKey,
             const String& longKey,
             bool required)
            :Base(shortKey,longKey,required)
            {}

     void assign(ValueType value)
     {
        storage_= value;
     }

     virtual void reset() override
     {
       if constexpr(isSequence)
          storage_.clear();
       else
          storage_.reset();
     }


     virtual bool tryAssingFromStrings(StringsConstIter first,
                                       StringsConstIter last,
                                       const String& prefixChars,
                                       String& error)override
     {
       return tryAssingFromStringsImpl(first,last,prefixChars,error);
     };

     virtual bool tryAssingFromStrings(const CharT** first,
                                       const CharT** last,
                                       const String& prefixChars,
                                       String& error) override
     {
       return tryAssingFromStringsImpl(first,last,prefixChars,error);
     };

     template<typename Iter>
     bool tryAssingFromStringsImpl(Iter first,
                                   Iter last,
                                   const String& prefixChars,
                                   String& error);


     T assingFromString(const String& str,
                        const String& prefixChars,
                        String& error);

     virtual std::size_t typeId() const  override{ return TypeInfo<T>::id; }
     virtual const char* typeName()const override{ return TypeInfo<T>::name; }
     virtual TypeGroup typeGroup()const  override{ return group; }

     virtual bool hasValue()const override
     {
       if constexpr(isSequence)
          return !storage_.empty();
       else
          return storage_.has_value();
     }

     ValueType storage()const
     {
       if constexpr(isSequence)
          return storage_;
       else
          return *storage_;
     };

   private:
       std::pair<RangeValueType,RangeValueType> range_ =
             std::make_pair(std::numeric_limits<RangeValueType>::lowest(),
                            std::numeric_limits<RangeValueType>::max());

       StorageType storage_;
};
//---------------------------------------------------------------------------------------
//   Detail
//---------------------------------------------------------------------------------------
namespace detail
{
    //----------------------------------------------------------------------------
    template<typename CharT>
    auto makeOptions(BaseArgImpl<CharT>* arg)
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
    auto makeUsage(BaseArgImpl<CharT>* arg,
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
    auto makeHelpLine(BaseArgImpl<CharT>* arg,
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
}
//---------------------------------------------------------------------------------------
template< typename T, TypeGroup group, typename CharT>
T ArgImpl<T, group, CharT>::
     assingFromString(const String& str,
                      const String& prefixChars,
                      String& error)
{
  using namespace StringUtils::literals;
  try
  {
    const T value = TypeInfo<T>::assignFromString(str);
    if constexpr(group==TypeGroup::string ||
                 group==TypeGroup::strings)
    {
      if(value.length() < range_.first ||
         value.length() > range_.second)
        throw std::out_of_range("out of range");
    }
    else
    {
      if(value< range_.first || value> range_.second)
        throw std::out_of_range("out of range");
    }
    return value;
  }
  catch (const std::out_of_range&)
  {
    error= "Error: argument '"_lv +
           detail::makeUsage(this,prefixChars)+ // !!!
           "' value: '"_lv+str+"' "_lv;

    if constexpr(group==TypeGroup::string ||
                 group==TypeGroup::strings)
       error += "string length "_lv;

    error+= "out of range ["_lv+
            StringUtils::toString<CharT>(this->range_.first)+
            ".."_lv+
            StringUtils::toString<CharT>(this->range_.second)+
            "]"_lv;

    return T();
  }
  catch (const std::invalid_argument& )
  {
    error=  "Error: argument '"_lv+
            detail::makeOptions(this)+
            "': invalid "_lv      +
            StringUtils::LatinView(typeName())+
            " value: '"_lv+str+"'"_lv;
    return T();
  }
  return T();
}
//---------------------------------------------------------------------------------------
template< typename T, TypeGroup group,typename CharT>
template<typename Iter>
bool ArgImpl<T, group, CharT>::
     tryAssingFromStringsImpl(Iter first,
                              Iter last,
                              const String& prefixChars,
                              String& error)
{
   const std::size_t count = std::distance(first,last);
   if(count < this->minCount_ || count > this->maxCount_)
   {
     using namespace StringUtils::literals;
     error= "Error: argument '"_lv+
            detail::makeOptions(this)+
            "': expected values count: "_lv+
            StringUtils::toString<CharT>(this->minCount_);

     if(this->minCount_ != this->maxCount_)
       error+= ".."_lv+ StringUtils::toString<CharT>(this->minCount_);

     return false;
   }

   if(count==0)
     return true;

   if constexpr(group==TypeGroup::number ||
                group==TypeGroup::string)
   {
     assert(first!=last);
     (void)last; // unused

     const T value = assingFromString(*first,prefixChars,error);
     if(!error.empty())
       return false;
     assign(value);
   }
   else
   {
     std::vector<T> values;
     for(; first!=last; ++first)
     {
       values.push_back(assingFromString(*first,prefixChars,error));
       if(!error.empty())
         return false;
     }
     assign(values);
   }
   return true;
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
     using Base = BaseArg<T,TypeGroup::string,CharT>;
     using RangeValueType = typename Base::Impl::RangeValueType;
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
template<typename T, char nargs='?',typename CharT= char>
using ArgT = Arg<T,
                 TypeUtils::groupOfNArgs<T,nargs,CharT>(),
                 CharT>;

template<char nargs='?',typename CharT= char>
using IntArg = ArgT<int,nargs,CharT>;

template<char nargs='?',typename CharT= char>
using StringArg = ArgT<std::basic_string<CharT>,nargs,CharT>;
//----------------------------------------------------------------------------
//                        ArgumentParser
//----------------------------------------------------------------------------
template <typename CharT>
class ArgumentParser
{
public:
  using String  = std::basic_string<CharT>;
  using Strings = std::vector<String>;
  using BaseArgPtr= std::shared_ptr<BaseArgImpl<CharT>>;
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

  bool parseArgs(int argc, CharT *argv[]);
  bool parseArgs(int argc, const CharT *argv[]);

  bool parseArgs(const Strings& args);
  bool parseCmdLine(const String& str);

  String help()const;
  String usage()const;
  void reset();

  const String& errorString()const{ return errorString_; }
  bool hasError()const { return !errorString_.empty(); }
  void setSubParserHelp(const String& help){ help_= help; };
  bool exists() const { return exists_; }

private:
  template <typename Iter>
  bool parse(Iter first,Iter last);

  template <typename Iter>
  bool pasrePositional(Iter first,Iter last);

  template <typename Iter>
  Iter parseOptional(Iter first,Iter last);

  std::shared_ptr<BaseArgImpl<CharT>>
  getOptional(std::basic_string_view<CharT> argOption);

  String subParsersUsage()const;

private:
  std::vector<BaseArgPtr> positional_;
  std::vector<BaseArgPtr> optional_;
  std::vector<ArgumentParserPtr> subParsers_;

  String errorString_;
  String name_;
  String help_;
  bool exists_ = false;
  String prefixChars_;
};
//------------------------------------------------------------------
template<typename CharT>
std::shared_ptr<BaseArgImpl<CharT>>
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
template<typename Iter>
bool ArgumentParser<CharT>::pasrePositional(Iter first, Iter last)
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

    if(!(*argIt)->tryAssingFromStrings(first,lastValueOfArgIt,
                                       prefixChars_,errorString_))
      return false;

    first =lastValueOfArgIt;
    totalCount -= count;
  }
  return true;
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
     {
       // ??
       return first;
     }

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

     if(!arg->tryAssingFromStrings(first,lastValueOfArgIt,
                                   prefixChars_,errorString_))
       return first;

     first = lastValueOfArgIt;
  }

  for(auto arg:optional_)
  {
    if(arg->required_ && !arg->exists_)
    {
      using namespace StringUtils::literals;
      errorString_=  "Error: the following argument are required:'"_lv+
                     detail::makeUsage(arg.get(),prefixChars_)+"'"_lv;
      return first;
    }
  }
  return first;
}
//------------------------------------------------------------------
template<typename CharT>
template<typename Iter>
bool ArgumentParser<CharT>::parse(Iter first, Iter last)
{
  auto endItOfPositional=
      std::find_if(first,last,
                   std::bind(StringUtils::isOption<String>,
                             std::placeholders::_1,
                             prefixChars_));

  if(!pasrePositional(first,endItOfPositional))
    return false;

  auto it = parseOptional(endItOfPositional,last);
  if(hasError())
    return false;

  bool invalidChoice= false;
  if(it!=last)
  {
    if(subParsers_.empty())
    {
       using namespace StringUtils::literals;
       errorString_= "Error: unrecognized arguments: '"_lv+
                     String(*it)+"'"_lv;
       return false;
    }
    else
    {
       auto parserIt=
           std::find_if(std::begin(subParsers_),std::end(subParsers_),
                        [&it](auto parser)
                        {
                          return parser->name_== *it;
                        });

       invalidChoice = (parserIt==std::end(subParsers_));             
       if(!invalidChoice)
       {
         (*parserIt)->exists_= true;
         if(!(*parserIt)->parse(std::next(it),last))
         {
           errorString_= (*parserIt)->errorString_;
           return false;
         }
       }
    }
  }
  else
    invalidChoice = !subParsers_.empty();

  if(invalidChoice)
  {
    using namespace StringUtils::literals;
    errorString_ = "Error: invalid choice: '3' (choose from '"_lv
                    +subParsers_.at(0)->name_+"'"_lv;
    for(std::size_t i=1; i<subParsers_.size(); ++i)
       errorString_ +=  ", '"_lv+subParsers_[i]->name_+"'"_lv;
    errorString_ += ")"_lv;
    return false;
  }

  return true;
}
//------------------------------------------------------------------
template <typename CharT>
bool ArgumentParser<CharT>::parseArgs(int argc, CharT *argv[])
{
  return parse(argv,argv+argc);
}
//------------------------------------------------------------------
template<typename CharT>
bool ArgumentParser<CharT>::parseArgs(int argc, const CharT *argv[])
{
  return parse(argv,argv+argc);
}
//------------------------------------------------------------------
template<typename CharT>
bool ArgumentParser<CharT>::parseArgs(const ArgumentParser::Strings &args)
{
   return parse(std::begin(args),std::end(args));
}
//------------------------------------------------------------------
template<typename CharT>
bool ArgumentParser<CharT>::parseCmdLine(const ArgumentParser::String &str)
{
   return parseArgs(StringUtils::split(str));
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
  errorString_.clear();

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
