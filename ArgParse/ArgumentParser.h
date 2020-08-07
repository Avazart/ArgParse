#ifndef ARGUMENTPARSER_H
#define ARGUMENTPARSER_H
//----------------------------------------------------------------------------
#include <algorithm>
#include <type_traits>
#include <assert.h>
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
       template <typename T,
                 std::size_t minCount,
                 std::size_t maxCount>
       Arg<T,CharT,detail::CaseOfV<T,CharT,maxCount>>*
                 addArgument(const String& shortKey,
                                 const String& longKey,
                                 bool required = false);

       template <typename T,
                 std::size_t minCount,
                 std::size_t maxCount>
       Arg<T,CharT,detail::CaseOfV<T,CharT,maxCount>>*
          addArgument(const String& longKey,bool required=false)
       {
         return addArgument<T,minCount,maxCount>(String(),longKey,required);
       }

       // nargs

       template <typename T,NArgs nargs= NArgs::optional>
       Arg<T,CharT,detail::CaseOfNArgV<T,CharT,nargs>>*
                         addArgument(const String& shortKey,
                                     const String& longKey,
                                     bool required = false);

       template <typename T,NArgs nargs= NArgs::optional>
       Arg<T,CharT,detail::CaseOfNArgV<T,CharT,nargs>>*
                         addArgument(const String& longKey,
                                     bool required = false)
       {
          return addArgument<T,nargs>(String(),longKey,required);
       }

       // char nargs

       template <typename T,char nargs>
       Arg<T,CharT,detail::CaseOfCharNArgV<T,CharT,nargs>>*
                         addArgument(const String& shortKey,
                                     const String& longKey,
                                     bool required = false);

       template <typename T,char nargs>
       Arg<T,CharT,detail::CaseOfCharNArgV<T,CharT,nargs>>*
                         addArgument(const String& longKey,
                                     bool required = false)
       {
         return addArgument<T,nargs>(String(),longKey,required);
       }

       bool parseArgs(int argc, CharT *argv[]);
       bool parseCmdLine(const String& str);
       bool parseCmdLine(const Strings& strs);

       String help()const;
       const String& errorString()const{ return errorString_; }

       bool hasError()const { return !errorString_.empty(); }

   private:
       template <typename Iter>
       bool parse(Iter first,Iter last);

       template <typename Iter>
       Iter pasrePositional(Iter first,Iter last);

       template <typename Iter>
       Iter parseOptional(Iter first,Iter last);

       BaseArg<CharT>* getOptional(std::basic_string_view<CharT> argKey);

       bool checkCount(BaseArg<CharT>* arg,std::size_t count);
   private:
       std::vector<BaseArg<CharT>*> positional_;
       std::vector<BaseArg<CharT>*> optional_;
       std::vector<ArgumentParser*> subParsers_;
       String errorString_;
       String name_;
};
//------------------------------------------------------------------
template<typename CharT>
BaseArg<CharT> *ArgumentParser<CharT>::getOptional(std::basic_string_view<CharT> argKey)
{
  auto it = std::find_if(std::begin(optional_), std::end(optional_),
  [&](auto arg)
  {
    return (!arg->longKey_.empty()  && arg->longKey_ == argKey) ||
           (!arg->shortKey_.empty() && arg->shortKey_== argKey);
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
    errorString_= "Error: argument '"_lv+ arg->name()+
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
Iter ArgumentParser<CharT>::pasrePositional(Iter first, Iter last)
{
  using namespace detail::literals;

  std::size_t count= std::distance(first,last);
  for(auto arg:positional_)
  {
    arg->exists_= true;

    if(!checkCount(arg,count))
    {
      return last;
    }

    const std::size_t currentArgCount= std::min(count,arg->maxCount_);
    last = std::next(first,currentArgCount);
    for( ; first!=last; ++first)
    {
       if(!arg->tryAssignOrAppend(*first, errorString_))
       {
         return last;
       }
    }
    count -= currentArgCount;
  }

  if(count)
  {
    errorString_ = "Error: unrecognized arguments: '"_lv+ (*first)+"'"_lv;
    return last;
  }

  return last;
}
//------------------------------------------------------------------
template<typename CharT>
template<typename Iter>
Iter ArgumentParser<CharT>::parseOptional(Iter first, Iter last)
{  
  using namespace detail::literals;

  while(first!=last)
  {        
     BaseArg<CharT>* arg = getOptional(*first);
     if(!arg)
     {
       errorString_= "Error: unrecognized arguments: '"_lv+ (*first)+"'"_lv;
       return last;
     }

     arg->exists_= true;
     first= std::next(first);

     auto nextOptionIt =
        std::find_if(first,last,detail::isOption<String>);

     const std::size_t count= std::distance(first,nextOptionIt);
     if(!checkCount(arg,count))
     {
       return nextOptionIt;
     }

     const std::size_t currentArgCount= std::min(count,arg->maxCount_);
     auto lastValueOfArgIt = std::next(first,currentArgCount);
     for( ;first!=lastValueOfArgIt; ++first)
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
      errorString_= "Error: the following argument are required:'"_lv+
                     arg->name()+"'"_lv;
      return last;
    }
  }
  return first;
}
//------------------------------------------------------------------
template<typename CharT>
template<typename Iter>
bool ArgumentParser<CharT>::parse(Iter first, Iter last)
{
  auto firstOptionIt =
      std::find_if(first,last, detail::isOption<String>);

  auto lastPositionalIt = pasrePositional(first,firstOptionIt);
  if(hasError())
    return false;

  auto lastOptionalIt = parseOptional(firstOptionIt,last);
  if(hasError())
    return false;

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
typename ArgumentParser<CharT>::String
   ArgumentParser<CharT>::help() const
{
   String s;
//   for(auto arg: positional_)
//   {
//     if(!arg->description_.empty())
//       s+= arg->shortKey_ + " " +
//           arg->longKey_  + " " +
//           arg->description_+"\n";
//   }
   return s;
}
//----------------------------------------------------------------------------
template<typename CharT,typename T,std::size_t maxCount>
auto createArg(const std::basic_string<CharT>& shortKey,
             const std::basic_string<CharT>& longKey,
             bool required)
{
  return new Arg<T,CharT,detail::CaseOfV<T,CharT,maxCount>>(shortKey,longKey,required);
}
//----------------------------------------------------------------------------
template<typename CharT>
template<typename T,  std::size_t minCount, std::size_t maxCount>
Arg<T, CharT,detail::CaseOfV<T,CharT,maxCount>>*
  ArgumentParser<CharT>::addArgument(
      const ArgumentParser<CharT>::String & shortKey,
      const ArgumentParser<CharT>::String & longKey,
      bool required)
{ 
   auto argPrt = createArg<CharT,T,maxCount>(shortKey,longKey,required);
   argPrt->minCount_= minCount;
   argPrt->maxCount_= maxCount;

   if(!detail::isOption(shortKey) && !detail::isOption(longKey))
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
                              const ArgumentParser<CharT>::String& shortKey,
                              const ArgumentParser<CharT>::String& longKey,
                              bool required)
{
   if constexpr(nargs==NArgs::optional)
     return addArgument<T,0,1>(shortKey,longKey,required);
   else if constexpr(nargs==NArgs::zeroOrMore)
     return addArgument<T,0,std::numeric_limits<std::size_t>::max()>(shortKey,longKey,required);
   else if constexpr(nargs==NArgs::oneOrMore)
      return addArgument<T,1,std::numeric_limits<std::size_t>::max()>(shortKey,longKey,required);
}
//----------------------------------------------------------------------------
template<typename CharT>
template <typename T,char nargs>
Arg<T, CharT, detail::CaseOfCharNArgV<T, CharT, nargs>> *
    ArgumentParser<CharT>::addArgument(
                              const ArgumentParser<CharT>::String& shortKey,
                              const ArgumentParser<CharT>::String& longKey,
                              bool required)
{
   if constexpr(nargs==(char)NArgs::optional)
     return addArgument<T,0,1>(shortKey,longKey,required);
   else if constexpr(nargs==(char)NArgs::zeroOrMore)
     return addArgument<T,0,std::numeric_limits<std::size_t>::max()>(shortKey,longKey,required);
   else if constexpr(nargs==(char)NArgs::oneOrMore)
     return addArgument<T,1,std::numeric_limits<std::size_t>::max()>(shortKey,longKey,required);

   static_assert( nargs=='?' || nargs=='*' || nargs=='+',
       "Error wrong nargs option!");
}
//----------------------------------------------------------------------------
}
//----------------------------------------------------------------------------
#endif // ARGUMENTPARSER_H
