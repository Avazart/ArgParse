#ifndef ARGUMENTPARSER_H
#define ARGUMENTPARSER_H
//----------------------------------------------------------------------------
#include <algorithm>
#include <type_traits>
#include <assert.h>

#include <iostream>
//----------------------------------------------------------------------------
#include "ArgumentParserDetail.h"
//----------------------------------------------------------------------------
#include "Arg.h"
//----------------------------------------------------------------------------
namespace argparse
{
//----------------------------------------------------------------------------
using NArgs = detail::NArgs;
using detail::split;
//----------------------------------------------------------------------------
template <typename CharT>
class ArgumentParser
{
   public:
       using String  = std::basic_string<CharT>;
       using Strings = std::vector<String>;

       virtual ~ArgumentParser();

       ArgumentParser* addSubParser(const String& name);

       // minCount maxCount
       template <typename T, std::size_t minCount, std::size_t maxCount>
       Arg<T,CharT,detail::CaseOfV<T,CharT,maxCount>>*
          addArgument(const String& shortOption,
                      const String& longOption=String(),
                      bool required= false);

       // nargs
       template <typename T,NArgs nargs= NArgs::optional>
       Arg<T,CharT,detail::CaseOfNArgV<T,CharT,nargs>>*
                         addArgument(const String& shortKey,
                                     const String& longKey=String(),
                                     bool required = false);

       // char nargs
       template <typename T,char nargs>
       Arg<T,CharT,detail::CaseOfCharNArgV<T,CharT,nargs>>*
                         addArgument(const String& shortOption,
                                     const String& longOption=String(),
                                     bool required = false);

       bool parseArgs(int argc, CharT *argv[]);
       bool parseCmdLine(const String& str);
       bool parseCmdLine(const Strings& strs);

       String help()const;
       const String& errorString()const{ return errorString_; }

       bool hasError()const { return !errorString_.empty(); }

       String usage()const;

       void setSubParserHelp(const String& help){ help_= help; };

       void reset();

   private:
       template <typename Iter>
       bool parse(Iter first,Iter last);

       template <typename Iter>
       bool pasrePositional(Iter first,Iter last);

       template <typename Iter>
       Iter parseOptional(Iter first,Iter last);

       BaseArg<CharT>* getOptional(std::basic_string_view<CharT> argOption);

       bool checkCount(BaseArg<CharT>* arg,std::size_t count);

       String makeSubParsersParam()const;
   private:
       std::vector<BaseArg<CharT>*> positional_;
       std::vector<BaseArg<CharT>*> optional_;
       std::vector<ArgumentParser*> subParsers_;
       String errorString_;
       String name_;
       String help_;
       String prefixChars_;
};
//------------------------------------------------------------------
template<typename CharT>
BaseArg<CharT> *ArgumentParser<CharT>::getOptional(std::basic_string_view<CharT> argOption)
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
bool ArgumentParser<CharT>::checkCount(BaseArg<CharT> *arg, std::size_t count)
{
  using namespace detail::literals;
  if(count < arg->minCount_) // || count > arg->maxCount_)
  {
    errorString_= "Error: argument '"_lv+ arg->makeOptions()+
                   "': expected values count: "_lv+
                   detail::toStringT<CharT>(std::to_string(arg->minCount_));

    if(arg->minCount_ != arg->maxCount_)
      errorString_+= ".."_lv+detail::toStringT<CharT>(std::to_string(arg->maxCount_));

    return false;
  }
  return true;
}
//------------------------------------------------------------------
template<typename CharT>
template<typename Iter>
bool ArgumentParser<CharT>::pasrePositional(Iter first, Iter last)
{
  bool greedy = true;
  std::size_t totalCount= std::distance(first,last);
  for(auto pa=std::begin(positional_);  pa!=std::end(positional_); ++pa)
  {
     (*pa)->exists_= true; // always exists for positional args

     std::size_t remainderCount= 0;
     std::for_each(std::next(pa),std::end(positional_),
                   [&remainderCount](auto a){ remainderCount += a->minCount_;});

     // +  if((*pa)->minCount_=1 && (*pa)->minCount_> (*pa)->maxCount_)
     // ?
     // *

     // the first posible arg must be greedy
     std::size_t optimalCount = std::max((*pa)->minCount_,1u);
     if(greedy && (*pa)->maxCount_>(*pa)->minCount_ && (*pa)->minCount_!=0)
     {
       optimalCount = (*pa)->maxCount_;
       greedy= false;
     }

     const std::size_t count= totalCount>remainderCount
                              ? std::min( optimalCount, totalCount-remainderCount)
                              : std::min( optimalCount, totalCount);

     auto lastOfArg = std::next(first,count);
     for(auto it=first; it!=lastOfArg; ++it)
     {
       if(!(*pa)->tryAssignOrAppend(*it, errorString_))
         return false;
     }
     first= lastOfArg;
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
     BaseArg<CharT>* arg = getOptional(*first);
     if(!arg)
     {
       // ??
       return first;
     }

     arg->exists_= true;
     first= std::next(first);

     auto nextOptionIt =
        std::find_if(first,last,detail::isOption<String>);

     const std::size_t count= std::distance(first,nextOptionIt);
     if(!checkCount(arg,count))
     {
       return first;
     }

     const std::size_t currentArgCount= std::min(count,arg->maxCount_);
     auto lastValueOfArgIt = std::next(first,currentArgCount);
     for( ; first!=lastValueOfArgIt; ++first)
     {
       if(!arg->tryAssignOrAppend(*first, errorString_))
         return first;
     }
     first = lastValueOfArgIt;
  }

  for(auto arg:optional_)
  {
    if(arg->required_ && !arg->exists_)
    {
      using namespace detail::literals;
      errorString_= "Error: the following argument are required:'"_lv+arg->makeParam()+"'"_lv;
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
  auto endItOfPositional = std::find_if(first,last,detail::isOption<String>);
  if(!pasrePositional(first,endItOfPositional))
    return false;

  auto it = parseOptional(endItOfPositional,last);
  if(hasError())
    return false;

  using namespace detail::literals;

  bool invalidChoice= false;
  if(it!=last)
  {
    if(subParsers_.empty())
    {
       errorString_= "Error: unrecognized arguments: '"_lv+(*it)+"'"_lv;
       return false;
    }
    else
    {
       auto parserIt= std::find_if(std::begin(subParsers_),std::end(subParsers_),
                                [&it](auto parser)
                                {
                                  return parser->name_== *it;
                                });
       invalidChoice = (parserIt==std::end(subParsers_));
       if(!invalidChoice && !(*parserIt)->parse(std::next(it),last))
       {
          errorString_= (*parserIt)->errorString_;
          return false;
       }
    }
  }
  else
    invalidChoice = !subParsers_.empty();

  if(invalidChoice)
  {
    errorString_ =  "Error: invalid choice: '3' (choose from "_lv;
    for(std::size_t i=0; i<subParsers_.size(); ++i)
    {
       if(i!=0) errorString_ += ", "_lv;
       errorString_ +=  "'"_lv+subParsers_[i]->name_+"'"_lv;
    }
    errorString_ += ")"_lv;
    return false;
  }

  return true;
}
//------------------------------------------------------------------
template <typename CharT>
bool ArgumentParser<CharT>::parseArgs(int argc, CharT *argv[])
{
   return parse(argv, argv+argc);
}
//------------------------------------------------------------------
template<typename CharT>
bool ArgumentParser<CharT>::parseCmdLine(const ArgumentParser::Strings &strs)
{
   return parse(std::begin(strs),std::end(strs));
}
//------------------------------------------------------------------
template<typename CharT>
bool ArgumentParser<CharT>::parseCmdLine(const ArgumentParser::String &str)
{
   return parseCmdLine(detail::split(str));
}
//------------------------------------------------------------------
template <typename CharT>
ArgumentParser<CharT>*
   ArgumentParser<CharT>::addSubParser(const ArgumentParser::String &name)
{
   ArgumentParser* parser= new ArgumentParser;
   parser->name_= name;
   subParsers_.push_back(parser);
   return parser;
}
//------------------------------------------------------------------
template<typename CharT>
ArgumentParser<CharT>::~ArgumentParser()
{
  for(auto optPtr:optional_)
     delete optPtr;
  for(auto posPtr:positional_)
     delete posPtr;
  for(auto parserPtr:subParsers_)
     delete parserPtr;
}
//----------------------------------------------------------------------------
template<typename CharT>
template<typename T,  std::size_t minCount, std::size_t maxCount>
Arg<T, CharT,detail::CaseOfV<T,CharT,maxCount>>*
  ArgumentParser<CharT>::addArgument(
      const ArgumentParser<CharT>::String & shortOption,
      const ArgumentParser<CharT>::String & longOption,
      bool required)
{ 
   auto argPrt =
       new Arg<T,CharT,detail::CaseOfV<T,CharT,maxCount>>(shortOption,longOption,required);
   argPrt->minCount_= minCount;
   argPrt->maxCount_= maxCount;

   if(!detail::isOption(shortOption) && !detail::isOption(longOption))
      positional_.push_back(argPrt);
   else
      optional_.push_back(argPrt);

   return argPrt;
}
//----------------------------------------------------------------------------
template<typename CharT>
template <typename T,detail::NArgs nargs>
Arg<T, CharT, detail::CaseOfNArgV<T, CharT, nargs> >*
     ArgumentParser<CharT>::addArgument(
                              const ArgumentParser<CharT>::String& shortOption,
                              const ArgumentParser<CharT>::String& longOption,
                              bool required)
{
   if constexpr(nargs==NArgs::optional)
     return addArgument<T,0,1>(shortOption,longOption,required);
   else if constexpr(nargs==NArgs::zeroOrMore)
     return addArgument<T,0,std::numeric_limits<std::size_t>::max()>(shortOption,longOption,required);
   else if constexpr(nargs==NArgs::oneOrMore)
     return addArgument<T,1,std::numeric_limits<std::size_t>::max()>(shortOption,longOption,required);
}
//----------------------------------------------------------------------------
template<typename CharT>
template <typename T,char nargs>
Arg<T, CharT, detail::CaseOfCharNArgV<T, CharT, nargs>> *
    ArgumentParser<CharT>::addArgument(
                              const ArgumentParser<CharT>::String& shortOption,
                              const ArgumentParser<CharT>::String& longOption,
                              bool required)
{
   if constexpr(nargs==(char)NArgs::optional)
     return addArgument<T,0,1>(shortOption,longOption,required);
   else if constexpr(nargs==(char)NArgs::zeroOrMore)
     return addArgument<T,0,std::numeric_limits<std::size_t>::max()>(shortOption,longOption,required);
   else if constexpr(nargs==(char)NArgs::oneOrMore)
     return addArgument<T,1,std::numeric_limits<std::size_t>::max()>(shortOption,longOption,required);

   static_assert( nargs=='?' || nargs=='*' || nargs=='+',
       "Error wrong nargs option!");
}
//------------------------------------------------------------------
template<typename CharT>
typename ArgumentParser<CharT>::String
  ArgumentParser<CharT>::makeSubParsersParam() const
{
  using namespace detail::literals;
  String params = "{"_lv;
  for(std::size_t i=0; i<subParsers_.size(); ++i)
  {
    if(i>0)
      params+= ","_lv;
    params+= subParsers_[i]->name_;
  }
  params+= "}"_lv;
  return params;
}
//----------------------------------------------------------------------------
template<typename CharT>
typename ArgumentParser<CharT>::String
   ArgumentParser<CharT>::help() const
{
   using namespace detail::literals;

   String helpStr;

   helpStr += "\npositional arguments:\n"_lv;
   for(auto arg: positional_)
   //  if(!arg->help_.empty())
       helpStr+= "  "_lv+arg->makeHelpLine()+"\n"_lv;

   if(!subParsers_.empty())
   {
     helpStr+= "  "_lv+makeSubParsersParam()+"\n"_lv;
     for(std::size_t i=0; i<subParsers_.size(); ++i)
       if(!subParsers_[i]->help_.empty())
          helpStr+= "  "_lv+ subParsers_[i]->name_ +" "_lv+
                    "  "_lv+ subParsers_[i]->help_ +"\n"_lv;
   }

   helpStr += "\noptional arguments:\n"_lv;
   for(auto arg: optional_)
   //  if(!arg->help_.empty())
       helpStr+= "  "_lv+arg->makeHelpLine()+"\n"_lv;

   return helpStr;
}
//----------------------------------------------------------------------------
template<typename CharT>
typename ArgumentParser<CharT>::String ArgumentParser<CharT>::usage() const
{
  using namespace detail::literals;

  String usageStr;
  for(std::size_t i=0; i<optional_.size(); ++i)
  {
    if(i>0)
      usageStr+= " "_lv;
    usageStr+= "["_lv+ optional_[i]->shortOption_+" "_lv+
                       optional_[i]->makeParam()+"]"_lv;
  }

  for(std::size_t i=0; i<positional_.size(); ++i)
  {
    if(i>0 || !usageStr.empty())
      usageStr+= " "_lv;
    usageStr+= positional_[i]->makeParam();
  }

  if(subParsers_.empty())
    return usageStr;

  if(!usageStr.empty())
    usageStr+= " "_lv;

   usageStr+= this->makeSubParsersParam();

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
