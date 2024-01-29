
#ifndef tp_value_node_h
#define tp_value_node_h

#include <vector>
#include <assert.h>
#include <memory>

#include "value_type.hpp"

namespace atf
{


class tp_value_node
{
  public:
    tp_value_node()
      : _value(), _parent( nullptr ), _childs(), _depth( 0 )
    {
      ++__number_tree_nodes;
    }
  

    template< typename T >
    tp_value_node( const T& value, void* tp_value_ptr, tp_value_node* parent )
      : _value( value ), _tp_value_ptr( tp_value_ptr ), _parent( parent ), _childs(), _depth( 0 )
    {
      ++__number_tree_nodes;
    }


    tp_value_node( const tp_value_node& other ) = delete;


    tp_value_node( const tp_value_node&& other ) = delete;
//    }

    auto value() const
    {
      return _value;
    }

    auto tp_value_ptr() const
    {
      return _tp_value_ptr;
    }

    auto num_childs() const
    {
      return _childs.size();
    }
  
  
    const auto& child( size_t i ) const
    {
      assert( i < _childs.size() );
      
      return *_childs[ i ];
    }
  
  
    const auto& child( const std::vector<size_t>& indices ) const
    {
      const tp_value_node* res = this;
      
      for( const auto& index : indices )
        res = &res->child( index );
      
      return *res;
    }
  
    const tp_value_node& parent() const
    {
      return *_parent;
    }
  
    size_t num_params() const
    {
      return _depth;
    }
  
  
    // return value is leaf node corresponding to the inserted path
    template< typename T, typename... T_rest >
    const tp_value_node& insert( T fst, T_rest... rest )
    {
      auto value        = std::get<0>( fst );
      auto tp_value_ptr = std::get<1>( fst );
      const size_t num_params = 1 + sizeof...( rest );
      if( _depth < num_params )
        _depth = num_params;
      
      if( _childs.empty() || value != static_cast< decltype(value) >( _childs.back()->value() ) )       {
        auto parent = this;
        _childs.emplace_back( std::make_unique<tp_value_node>( value, tp_value_ptr, parent ) );
        return _childs.back()->insert( rest... );
      }
      
      else
        return _childs.back()->insert( rest... );
    }
  
  

    // returns corresponding leaf node
    const tp_value_node& insert()
    {
      return *this;
    }
  
  
    void print() const
    {
    }
  
  
    // maximal number of childs for a node in the layer "layer". root has layer "0".
    size_t max_childs( size_t layer ) const
    {
      if( layer == 0 )
        return num_childs();
      
      size_t max_childs = 0;
      for( const auto& child : this->_childs )
        if( child->max_childs( layer - 1 ) > max_childs )
          max_childs = child->max_childs( layer - 1 );
      
      return max_childs;
    }
  
  
  protected:
    value_type                                     _value;
    void*                                          _tp_value_ptr;
    tp_value_node*                                 _parent;
    std::vector< std::unique_ptr<tp_value_node> >  _childs;
    size_t                                         _depth;

// static member
  public:
    static size_t number_of_nodes()
    {
      return __number_tree_nodes;
    }
  
  private:
    static size_t __number_tree_nodes;
  
};
size_t tp_value_node::__number_tree_nodes = 0;

} // namespace "atf"

#endif /* tp_value_node_h */
