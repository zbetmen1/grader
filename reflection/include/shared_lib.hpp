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

#ifndef SHARED_LIB_H
#define SHARED_LIB_H

// STL headers
#include <string>
#include <dlfcn.h>
#include <stdexcept>

// Project headers
#include "reflection_types.hpp"

namespace reflection
{
  
  /**
   * @brief Simple class that represents exception fired when loading of shared library fails.
   * NOTE: As shared library load failure is undetectable at compile time this class extends std::runtime_error,
   * not std::logic_error.
   * 
   */
  class shared_lib_load_failed: public std::runtime_error
  {
    std::string m_msg;
  public:
    explicit shared_lib_load_failed(const std::string& arg)
    : std::runtime_error{arg}, m_msg{arg}
    {}
    
    virtual const char* what() const noexcept { return m_msg.c_str(); }
  };
  
  // Forward declaration of object to minimize compilation dependencies
  class object;
  
  /**
   * @brief This is simple strongly typed enum used to specify in which way shared library should be loaded.
   * 
   */
  enum class shared_lib_mode: int
  {
    LAZY = RTLD_LAZY,
    NOW = RTLD_NOW
  };
  
  class shared_lib
  {
    shared_lib_impl m_impl;
    
    /**
     * @brief Converts enum to int. One way to avoid this function is to use union of enum and int (that is not portable).
     * 
     * @param flag Enum to convert.
     * @return int
     */
    static int convert(shared_lib_mode flag);
  public:
    
    /**
     * @brief Constructs shared_lib object from shared library on given path.
     * 
     * @param p Path to shared library.
     * @param flag Mode in which to open shared library.
     */
    shared_lib(const path& p, shared_lib_mode flag);
    
    /**
     * @brief Unloads loaded shared library. 
     * 
     */
    ~shared_lib();
    
    /**
     * @brief Returns object dynamically created from this shared library.
     * 
     * @param className Name of class to be created.
     * @return reflection::object*
     */
    object* make_object(const std::string& className) const;

    /**
     * @brief Gets pointer to function, with C linkage, stored in this shared library by name.
     * 
     * @param functionName Name of function for which you need to fetch pointer.
     * @return void*
     */
    void* get_c_function(const std::string& functionName) const;
    
    // Shared library shouldn't be copied
    shared_lib(const shared_lib&) = delete;
    shared_lib& operator=(const shared_lib&) = delete;
    
    // Shared library should be movable
    shared_lib(shared_lib&& moved);
    shared_lib& operator=(shared_lib&& moved);
  };
}

#endif // SHARED_LIB_H
