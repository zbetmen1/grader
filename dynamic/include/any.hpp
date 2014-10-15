/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2014  Kocic Ognjen <email>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef ANY_H
#define ANY_H

#include <typeinfo>
#include <type_traits>

namespace dynamic
{ 
  class any
  {
    class placeholder
    {
    public:
      virtual ~placeholder() {}
      virtual const std::type_info& type() const = 0;
      virtual placeholder* clone() const = 0;
    };
    
    template <typename T>
    class holder: public placeholder
    {
      holder & operator=(const holder &);
    public:
      T m_val;
      
      holder(const T& val)
      : m_val{val}
      {}
      
      holder(T&& val)
      : m_val{static_cast<T&&>(val)}
      {}
      
      const std::type_info& type() const { return typeid(T); }
      placeholder* clone() const { return new holder<T>{m_val}; }
    };
  
    placeholder* m_any;
  public:
    any();
    any(const any& oth);
    any(any&& moved);
    any& swap(any& oth) noexcept;
    any& operator=(const any& oth);
    any& operator=(any&& moved);
    ~any();
    
    bool empty() const;
    void clear();
    const std::type_info& type() const;
    
    template<typename T>
    any(const T& value)
    : m_any{new holder<typename std::decay<T>::type>{value}}
    {}
    
    template<typename T>
    any(T&& valMoved, 
        typename std::enable_if<! std::is_same<any&, T>::value>::type* = nullptr,
        typename std::enable_if<! std::is_const<T>::value>::type* = nullptr
       )
    : m_any(new holder<typename std::decay<T>::type>(static_cast<T&&>(valMoved)) )
    {}
    
    template <typename T>
    any& operator=(const T& val)
    {
      any{val}.swap(*this);
      return *this;
    }
    
    template <typename T>
    any& operator=(T&& valMoved)
    {
      any{valMoved}.swap(*this);
      return *this;
    }
  
    template <typename T> friend T* any_cast(any* x);
    template <typename T> friend const T* any_cast(const any* x);
    template <typename T> friend T any_cast(any& x);
    template <typename T> friend const T any_cast(const any& x);
    template <typename T> friend T any_cast(any&& x);
  };
  
  void swap(any& lhs, any& rhs) noexcept;
  
  template <typename T>
  T* any_cast(any* x) 
  {
    return x && x->m_any->type() == typeid(T) ? &static_cast< any::holder<T>* >(x->m_any)->m_val : nullptr;
  }
  
  template <typename T>
  const T* any_cast(const any* x)
  {
    return any_cast<T>(const_cast<any*>(x));
  }
  
  template <typename T>
  T any_cast(any& x)
  {
    // Remove reference and use previous version of any_cast
    using T_NotRefType = typename std::remove_reference<T>::type;
    T_NotRefType* result = any_cast<T_NotRefType>(&x);
    if (!result)
      throw std::bad_cast{};
    
    // Add reference to type T in case it isn't reference type
    using T_IsRefType = typename std::conditional<
                                              std::is_lvalue_reference<T>::value, 
                                              T, 
                                              typename std::add_lvalue_reference<T>::type 
                                              >::type;
    return static_cast<T_IsRefType>(*result);
  }
  
  template <typename T> 
  const T any_cast(const any& x)
  {
    using T_NotRefType = typename std::remove_reference<T>::type;
    return any_cast<const T_NotRefType&>(const_cast<any&>(x));
  }
  
  template <typename T> 
  T any_cast(any&& x)
  {
    static_assert(false, "Applying any_cast to temporary objects is forbidden for now.");
    return T{};
  }
}

#endif // ANY_H
