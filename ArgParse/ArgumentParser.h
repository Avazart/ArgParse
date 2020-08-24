#ifndef ARGUMENTPARSER_H
#define ARGUMENTPARSER_H
//----------------------------------------------------------------------------
#include <algorithm>
#include <numeric>
#include <type_traits>
#include <memory>
#include <functional>
//----------------------------------------------------------------------------
#include "StringUtils.h"
#include "Arg.h"
//----------------------------------------------------------------------------
namespace ArgParse
{
//----------------------------------------------------------------------------
using NArgs = TypeUtils::NArgs;
using StringUtils::split;
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
      errorString_= "Error: the following argument are required:'"_lv+
                     makeUsage(arg.get(),prefixChars_)+"'"_lv;
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
