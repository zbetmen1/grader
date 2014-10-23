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

#ifndef EXPECTED_H
#define EXPECTED_H

#include <stdexcept>
#include <exception>
#include <type_traits>

namespace dynamic
{ 
  template <typename T>
  class expected
  {
    bool m_valid;
    union 
    {
      T m_val;
      std::exception_ptr m_error;
    };
    
  public:
    template <typename U = T>
    expected(const U& val, typename std::enable_if<std::is_base_of<std::exception, U>::value, U>::type* = 0);
    
    template <typename U = T>
    expected(U&& val, typename std::enable_if<std::is_base_of<std::exception, U>::value, U>::type* = 0);
    
    template <typename U = T>
    expected(const U& val, 
             typename std::enable_if<!std::is_base_of<std::exception, U>::value, U>::type* = 0);
    
    template <typename U = T>
    expected(U&& val, 
             typename std::enable_if<!std::is_base_of<std::exception, U>::value, U>::type* = 0);
    
    template <typename U = T>
    expected(U excPtr, typename std::enable_if<std::is_same<std::exception_ptr, U>::value, U>::type* = 0);
    
    expected(const expected& oth);
    
    expected(expected&& moved);
    
    expected(const expected<void>& oth);
    
    expected(expected<void>&& moved);
    
    ~expected();
    
    expected& swap(expected& oth) noexcept;
    
    bool is_valid() const;
    T& get();
    const T& get() const;
  };
  
  template <>
  class expected<void>
  {
    std::exception_ptr m_error;
  public:
    expected()
    : m_error{nullptr}
    {}
    
    expected(const std::exception& e)
    : m_error{std::make_exception_ptr(e)}
    {}
    
    expected& swap(expected& oth) noexcept
    {
      m_error.swap(oth.m_error);
      return *this;
    }
    
    bool is_valid() const { return nullptr == m_error; }
    const std::exception_ptr get() const { return m_error; }
    
    template<typename T> friend class expected;
  };
  
  template <typename T>
  void swap(expected<T>& lhs, expected<T>& rhs) noexcept
  {
    lhs.swap(rhs);
  }
  
  template <typename T>
  template <typename U>
  expected<T>::expected(const U& val, typename std::enable_if<std::is_base_of<std::exception, U>::value, U>::type*)
   : m_valid{false}, m_error{std::make_exception_ptr(val)}
   {}
   
  template <typename T>
  template <typename U>
  expected<T>::expected(U&& val, typename std::enable_if<std::is_base_of<std::exception, U>::value, U>::type*)
   : m_valid{false}, m_error{std::make_exception_ptr(val)}
   {}
   
  template <typename T>
  template <typename U>
  expected<T>::expected(const U& val, 
            typename std::enable_if<!std::is_base_of<std::exception, U>::value, U>::type*)
   : m_valid{true}, m_val{val}
   {}
   
  template <typename T>
  template <typename U>
  expected<T>::expected(U&& val, 
            typename std::enable_if<!std::is_base_of<std::exception, U>::value, U>::type*)
   : m_valid{true}, m_val{std::move(val)}
   {}

   template <typename T>
   expected<T>::expected(const expected<T>& oth)
   : m_valid{oth.m_valid}
   {
     if (m_valid) new (&m_val) T{oth.m_val};
     else new (&m_error) std::exception_ptr{oth.m_error};
   }
   
   template <typename T>
   expected<T>::expected(expected<T>&& moved)
   : m_valid{moved.m_valid}
   {
     if (m_valid) new (&m_val) T{std::move(moved.m_val)};
     else { new (&m_error) std::exception_ptr{std::move(moved.m_error)}; moved.m_error = nullptr; }
   }
   
   template <typename T>
   expected<T>::expected(const expected<void>& oth)
   : m_valid{oth.is_valid()}
   {
     if (m_valid) new(&m_val) T{};
     else new (&m_error) std::exception_ptr{oth.get()};
   }
   
   template <typename T>
   expected<T>::expected(expected<void>&& moved)
   : m_valid{moved.is_valid()}
   {
     if (m_valid) new(&m_val) T{};
     else { new (&m_error) std::exception_ptr{moved.get()}; moved.m_error = nullptr; }
   }
   
   template <typename T>
   expected<T>::~expected()
   {
     using namespace std;
     if (m_valid) m_val.~T();
     else m_error.~exception_ptr();
   }
   
   template <typename T>
   expected<T>& expected<T>::swap(expected<T>& oth) noexcept
   {
     if (m_valid)
     {
       if (oth.m_valid)
       {
         using namespace std; 
         swap(m_val, oth.m_val);
       }
       else 
       {
         auto val = std::move(m_val);
         new (&m_error) std::exception_ptr{std::move(oth.m_error)};
         new (&(oth.m_val)) T{val};
         std::swap(m_valid, oth.m_valid);
       }
     }
     else 
     {
       if (oth.m_valid)
       {
         return oth.swap(*this);
       }
       else 
       {
         m_error.swap(oth.m_error);
       }
     }
     
     return *this;
   }
   
   template <typename T>
   bool expected<T>::is_valid() const { return m_valid; }
   
   template <typename T>
   T& expected<T>::get() { if (!m_valid) std::rethrow_exception(m_error); return m_val; }
   
   template <typename T>
   const T& expected<T>::get() const { if (!m_valid) std::rethrow_exception(m_error); return m_val; }
}

#endif // EXPECTED_H
