
#ifndef search_space_h
#define search_space_h

#include <iostream>
#include <assert.h>

#include <fstream>

#include "tp_value_node.hpp"
#include "tp_value.hpp"
#include "big_int.hpp"

namespace atf
{


class search_space
{
  public:
    virtual big_int num_configs() const = 0;

    virtual void add_name( const std::string& name ) = 0;
  
    virtual configuration operator[]( const big_int& index ) const = 0;

    virtual configuration get_configuration( const big_int& index ) const = 0;
    virtual configuration get_configuration( const coordinates& indices ) const = 0;
    virtual configuration get_configuration( const std::vector<size_t>& indices ) const = 0;

    virtual size_t num_params() const = 0;

    virtual size_t max_childs( size_t layer ) const = 0;
  
    virtual size_t max_childs_of_node( std::vector<size_t>& indices ) const = 0;
    virtual const std::vector< std::string >& names() const = 0;

    virtual const std::string& name( size_t layer ) const = 0;
};


} // namespace "atf"

#endif /* search_space_h */
