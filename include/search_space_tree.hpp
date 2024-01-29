
#ifndef search_space_tree_h
#define search_space_tree_h

#include <iostream>
#include <assert.h>

#include <fstream>
#include <regex>

#include "tp_value_node.hpp"
#include "tp_value.hpp"
#include "search_space.hpp"

namespace atf
{

class Tree
{
  public:
    // ctors
    Tree()
      : _root( std::make_unique<tp_value_node>() ), _leafs()
    {}
    
    Tree(       Tree&& other ) = default;
    Tree( const Tree&  other ) = default;


    big_int num_configs() const
    {
      return _leafs.size();
    }
  
  
    size_t depth() const
    {
      return _root->num_params();
    }
  
    template< typename... Ts >
    void insert( Ts... params )
    {
      const tp_value_node& leaf = _root->insert( params... );
      _leafs.emplace_back( &leaf );
    }


    const tp_value_node& root() const
    {
      return *_root;
    }

  
    const tp_value_node& leaf( size_t i ) const
    {
      return *_leafs[ i ];
    }
  
  
    // forwards to tp_value node
  
    template< typename... Ts >
    const auto& child( Ts... params ) const
    {
      return _root->child( params... );
    }


    template< typename... Ts >
    size_t max_childs( Ts... params ) const
    {
      return _root->max_childs( params... );
    }


    template< typename... Ts >
    size_t num_params( Ts... params ) const
    {
      return _root->num_params( params... );
    }

  
    // cast
    operator tp_value_node&()
    {
      return *_root;
    }
    

  private:
    std::shared_ptr< tp_value_node >    _root;
    std::vector< tp_value_node const* > _leafs;
};



class search_space_tree : public search_space
{
  public:
    search_space_tree()
      : _trees(), _tp_names()
    {}
  
    search_space_tree( const search_space_tree&  other ) = default;
    search_space_tree(       search_space_tree&& other ) = default;

    big_int num_configs() const
    {
      big_int num_configs = 1;
      
      for( const auto& tree : _trees )
        num_configs = num_configs * tree.num_configs();
      
      return num_configs;
    }
  
  
    void append_new_trees( size_t num ) // as "friend"
    {
      auto old_size = _trees.size();
      _trees.resize( old_size + num );
    }
  
    template< typename... Ts >
    void insert_in_last_tree( Ts... values )  // as "friend"
    {
      _trees.back().insert( values... );
    }
  
    Tree& tree( size_t tree_id )  // as "friend"
    {
      return _trees[ tree_id ];
    }
  
    void add_name( const std::string& name )
    {
      _tp_names.emplace_back( name );
    }
  


  
    configuration operator[]( const big_int& index ) const {
        return get_configuration( index );
    }


    configuration get_configuration( const big_int& index ) const
    {
      if (index > this->num_configs()) {
        throw std::runtime_error("search space index is out of bounds");
      }

      auto config = configuration{};
      size_t pos = this->num_params() - 1;
      
      // iterate over trees (bottom-up)
      for( int tree_id = static_cast<int>( _trees.size() ) - 1 ; tree_id >= 0 ; --tree_id )
      {
        const auto& tree = _trees[ tree_id ];

        // select leaf
        big_int num_configs_of_lower_trees = 1;
        for( int lower_tree = tree_id + 1; lower_tree < _trees.size() ; ++lower_tree )
          num_configs_of_lower_trees = num_configs_of_lower_trees * _trees[ lower_tree ].num_configs();

        auto leaf_id = ( index / num_configs_of_lower_trees ) % tree.num_configs();
        // go leaf up and insert TP values in config
        tp_value_node const* tree_node = &tree.leaf( static_cast<size_t>(leaf_id) );
        for( size_t i = 0 ; i < tree.num_params() ; ++i )
        {
            config.emplace( std::piecewise_construct,
                            std::forward_as_tuple( this->name(pos)                               ),
                            std::forward_as_tuple( tree_node->value(), tree_node->tp_value_ptr() )
            );
            --pos;
            tree_node = &( tree_node->parent() );
        }
      }
      
      return config;
    }
  
  
    configuration get_configuration( const coordinates& indices ) const {
      assert( indices.size()   == this->num_params() );
      assert( _tp_names.size() == this->num_params() );
      if (!atf::valid_coordinates(indices)) {
        throw std::runtime_error("search space coordinate is out of bounds (0.0,1.0]");
      }

      configuration config;
      size_t i_global = 0;
      for( const auto& tree : _trees )
      {
        const tp_value_node* tree_node = &tree.root();
        for( size_t i = 0 ; i < tree.num_params() ; ++i, ++i_global )
        {
          tree_node = &( tree_node->child( std::ceil(indices[ i_global ] * tree_node->num_childs()) - 1 ) );
          config.emplace( std::piecewise_construct,
                          std::forward_as_tuple( this->name(i_global)                          ),
                          std::forward_as_tuple( tree_node->value(), tree_node->tp_value_ptr() )
                        );
        }
      }

      assert( i_global == config.size() );
      return config;
    }


    configuration get_configuration( const std::vector<size_t>& indices ) const
    {
      assert( indices.size()   == this->num_params() );
      assert( _tp_names.size() == this->num_params() );


      configuration config;
      size_t i_global = 0;
      for( const auto& tree : _trees )
      {
        const tp_value_node* tree_node = &tree.root();
        for( size_t i = 0 ; i < tree.num_params() ; ++i, ++i_global )
        {
          tree_node = &( tree_node->child( indices[ i_global ] ) );
          config.emplace( std::piecewise_construct,
                          std::forward_as_tuple( this->name(i_global)                          ),
                          std::forward_as_tuple( tree_node->value(), tree_node->tp_value_ptr() )
                        );
        }
      }

      assert( i_global == config.size() );       return config;
    }
  
  

  


    // the number of TPs, i.e. the tree depth
    size_t num_params() const
    {
      size_t num_params_of_all_trees = 0;
      
      for( const auto& tree : _trees )
        num_params_of_all_trees += tree.num_params();
      
      return num_params_of_all_trees;
    }


    size_t max_childs( size_t layer ) const
    {
      assert( layer < this->num_params() );
      
      for( const auto& tree : _trees )
      {
        if( layer < tree.num_params() )
          return tree.max_childs( layer );
        else
          layer -= tree.num_params(); // go to the next config_tree generated by "G(...)"
      }
  
      assert( false ); // should never be reached
      
      return 0;
    }


    size_t max_childs_of_node( std::vector<size_t>& indices ) const {
      assert( indices.size() < this->num_params() );
      size_t tree_index = 0;
      
      while( indices.size() >= _trees[ tree_index ].num_params() )
        indices.erase( indices.begin(), indices.begin() + _trees[ tree_index++ ].num_params() );

      return _trees[ tree_index ].child( indices ).num_childs();
    }
  
  
    const std::vector< std::string >& names() const
    {
      return _tp_names;
    }


    const std::string& name( size_t i ) const
    {
      return _tp_names[ i ];
    }

  
    size_t num_trees() const // as "friend"
    {
      return _trees.size();
    }

    const std::vector<Tree>& trees() const      {
      return _trees;
    }
  
  
        ~search_space_tree()
    {

//      std::cout << "search space size: " << this->num_configs() << std::endl;

      
      
      

      
      
    }
  
  private:
    std::vector< Tree >        _trees;
    std::vector< std::string > _tp_names;
};


} // namespace "atf"


#endif /* search_space_tree_h */
