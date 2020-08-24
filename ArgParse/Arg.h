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
#include <cassert>
//----------------------------------------------------------------------------
#include "StringUtils.h"
#include "TypeUtils.h"
//----------------------------------------------------------------------------
namespace ArgParse
{
//----------------------------------------------------------------------------
template<typename CharT=char> class ArgumentParser;

using TypeGroup = TypeUtils::Group;

template <typename T>
using TypeInfo = TypeUtils::TypeInfo<T>;
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
template<typename CharT>
auto makeOptions(BaseArgImpl<CharT>* arg);

template<typename CharT>
auto makeOptions(BaseArgImpl<CharT>* arg);

template<typename CharT>
auto makeHelpLine(BaseArgImpl<CharT>* arg,
                  const std::basic_string<CharT>& prefixChars);
//---------------------------------------------------------------------------------------
template<typename T, TypeGroup group, typename CharT>
class Arg;
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
           makeUsage(this,prefixChars)+ // !!!
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
            makeOptions(this)+
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
            makeOptions(this)+
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
// Arg
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
template<typename T, char nargs='?',typename CharT= char>
using ArgT = Arg<T,
                 TypeUtils::groupOfNArgs<T,nargs,CharT>(),
                 CharT>;

template<char nargs='?',typename CharT= char>
using IntArg = ArgT<int,nargs,CharT>;

template<char nargs='?',typename CharT= char>
using StringArg = ArgT<std::basic_string<CharT>,nargs,CharT>;
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
//----------------------------------------------------------------------------
//
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
//---------------------------------------------------------------------------------------
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
//---------------------------------------------------------------------------------------
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
//-------------------------------------------------------------------
}
//-------------------------------------------------------------------
#endif // ARG_H
