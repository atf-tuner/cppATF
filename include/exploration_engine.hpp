
#ifndef exploration_engine_h
#define exploration_engine_h

#include <tuple>
#ifdef PARALLEL_SEARCH_SPACE_GENERATION
#include <thread>
#endif
#include <utility>
#include <chrono>

#include "search_space_tree.hpp"
#include "abort_condition.hpp"
#include "search_technique.hpp"
#include "search_technique_1d.hpp"
#include "tuning_status.hpp"

#include "helper.hpp"
#include "tp.hpp"


namespace atf
{

// helper
template< typename... TPs >
class G_class
{
  public:
    G_class( TPs&... tps );
  
    G_class()                 = delete;
    G_class( const G_class& ) = default;
    G_class(       G_class& ) = default;
  
    ~G_class() = default;
  
    auto tps() const;

  private:
    std::tuple<TPs&...> _tps;
  
};

template< typename... TPs >
auto G( TPs&... tps )
{
  return G_class<TPs...>( tps... );
}

class tuner;

class exploration_engine
{
public:
    friend atf::tuner;

    exploration_engine()
      : _search_space(), _abort_on_error( false ), _status()
    {
      _abort_condition = NULL;
      _copy_abort_condition = []() -> abort_condition* { return NULL; };
      _search_technique = NULL;
      _copy_search_technique = []() -> search_technique* { return NULL; };
      _search_technique_1d = NULL;
      _copy_search_technique_1d = []() -> search_technique_1d* { return NULL; };
      _status._history.emplace_back( std::chrono::steady_clock::now(),
                                     configuration{},
                                     std::numeric_limits<size_t>::max()
                                   );
    }


    exploration_engine(const exploration_engine& other )       :
      _search_space(other._search_space),
      _abort_condition( other._copy_abort_condition() ),
      _copy_abort_condition( other._copy_abort_condition ),
      _search_technique( other._copy_search_technique() ),
      _copy_search_technique( other._copy_search_technique ),
      _search_technique_1d( other._copy_search_technique_1d() ),
      _copy_search_technique_1d( other._copy_search_technique_1d ),
      _abort_on_error( other._abort_on_error ),
      _silent( other._silent ),
#ifdef PARALLEL_SEARCH_SPACE_GENERATION
      _threads(),
#endif
      _status( other._status ),
      _log_file( other._log_file )
    {}

    exploration_engine(exploration_engine&& other )       :
      _search_space( other._search_space ),
      _abort_condition( other._copy_abort_condition() ),
      _copy_abort_condition( other._copy_abort_condition ),
      _search_technique( other._copy_search_technique() ),
      _copy_search_technique( other._copy_search_technique ),
      _search_technique_1d( other._copy_search_technique_1d() ),
      _copy_search_technique_1d( other._copy_search_technique_1d ),
      _abort_on_error( other._abort_on_error ),
      _silent( other._silent ),
#ifdef PARALLEL_SEARCH_SPACE_GENERATION
      _threads(),
#endif
      _status( other._status ),
      _log_file( other._log_file )
    {}

  
    virtual ~exploration_engine() = default;

    void set_abort_condition() {
      _abort_condition = NULL;
      _copy_abort_condition = []() -> abort_condition* { return NULL; };
    }

    template< typename abort_condition_t >
    void set_abort_condition( const abort_condition_t& abort_condition ) {
      _abort_condition = std::unique_ptr<atf::abort_condition>(new abort_condition_t(abort_condition ) );
      _copy_abort_condition = [this]() -> atf::abort_condition* { return new abort_condition_t(*(dynamic_cast<abort_condition_t*>(this->_abort_condition.get()))); };
    }

    template< typename search_technique_t, typename std::enable_if<std::is_base_of<search_technique, search_technique_t>::value, bool>::type = true >
    void set_search_technique( const search_technique_t& search_technique_val ) {
      _search_technique = std::unique_ptr<search_technique>( new search_technique_t( search_technique_val ) );
      _copy_search_technique = [this]() -> search_technique* { return new search_technique_t(*(dynamic_cast<search_technique_t*>(this->_search_technique.get()))); };
    }

    template< typename search_technique_t, typename std::enable_if<std::is_base_of<search_technique_1d, search_technique_t>::value, bool>::type = true >
    void set_search_technique( const search_technique_t& search_technique ) {
      _search_technique_1d = std::unique_ptr<search_technique_1d>( new search_technique_t( search_technique ) );
      _copy_search_technique_1d = [this]() -> search_technique_1d* { return new search_technique_t(*(dynamic_cast<search_technique_t*>(this->_search_technique_1d.get()))); };
    }

    void set_silent(bool silent) {
      _silent = silent;
    }

    void set_log_file(const std::string &log_file) {
      _log_file = log_file;
    }

    // set tuning parameters
    template< typename... Ts, typename... range_ts, typename... callables >
    exploration_engine& operator()(tp_t<Ts,range_ts,callables>&... tps );
  
    // first application of operator(): IS (required due to ambiguity)
    template< typename T, typename range_t, typename callable >
    exploration_engine& operator()(tp_t<T,range_t,callable>& tp );
  

    // set tuning parameters
    template< typename... Ts, typename... G_CLASSES >
    exploration_engine& operator()(G_class<Ts...> G_class, G_CLASSES... G_classes );

    template< typename callable >
    tuning_status operator()( callable& program ); // program must take config_t and return a size_t

  protected:
    search_space_tree _search_space;
    tuning_status     _status;

  private:
    template< typename... Ts, typename... rest_tp_tuples >
    void insert_tp_names_in_search_space( G_class<Ts...> tp_tuple, rest_tp_tuples... tuples );

    void insert_tp_names_in_search_space()
    {}

    template< typename... Ts, size_t... Is >
    void insert_tp_names_of_one_tree_in_search_space( G_class<Ts...> tp_tuple, std::index_sequence<Is...> );

    template< typename T, typename range_t, typename callable, typename... Ts >
    void insert_tp_names_of_one_tree_in_search_space( tp_t<T,range_t,callable>& tp, Ts&... tps );
  
    void insert_tp_names_of_one_tree_in_search_space()
    {}
  
    template< size_t TREE_ID, typename... Ts, typename... rest_tp_tuples>
    exploration_engine& generate_config_trees(G_class<Ts...> tp_tuple, rest_tp_tuples... tuples );

    template< size_t TREE_ID, typename... Ts, size_t... Is, typename... rest_tp_tuples >
    exploration_engine& generate_config_trees(G_class<Ts...> tp_tuple, std::index_sequence<Is...>, rest_tp_tuples... tuples );

    template< size_t TREE_ID >
    exploration_engine& generate_config_trees();

    template< size_t TREE_ID, size_t TREE_DEPTH, typename T, typename range_t, typename callable, typename... Ts, std::enable_if_t<( TREE_DEPTH>0 )>* = nullptr >
    void generate_single_config_tree( tp_t<T,range_t,callable>& tp, Ts&... tps );
  
    template< size_t TREE_ID, size_t TREE_DEPTH, typename... Ts, std::enable_if_t<( TREE_DEPTH==0 )>* = nullptr >
    void generate_single_config_tree( const Ts&... values );

    template< typename T, typename... Ts >
    void print_path(T val, Ts... tps);

    void print_path()
    {
        std::cout << std::endl;
    }

    std::unique_ptr<abort_condition>  _abort_condition;
    std::function<abort_condition*()> _copy_abort_condition;
    const bool                                  _abort_on_error;
    bool                                        _silent = false;
    std::string                                 _log_file;
#ifdef PARALLEL_SEARCH_SPACE_GENERATION
    std::vector<std::thread>                     _threads;
#endif
    std::unique_ptr<search_technique>     _search_technique;
    std::function<search_technique*()>    _copy_search_technique;
    std::set<coordinates>                 _next_coordinates{};
    std::map<coordinates, cost_t>         _next_costs{};
    std::unique_ptr<search_technique_1d>  _search_technique_1d;
    std::function<search_technique_1d*()> _copy_search_technique_1d;
    std::set<index>                       _next_indices_1d{};
    std::map<index, cost_t>               _next_costs_1d{};

    void initialize() {
      if (_search_technique) {
        _search_technique->initialize( _search_space.num_params() );
      } else if (_search_technique_1d) {
        _search_technique_1d->initialize( _search_space.num_configs() );
      }
    }

    void finalize() {
      if (_search_technique) {
        _search_technique->finalize();
      } else if (_search_technique_1d) {
        _search_technique_1d->finalize();
      }
    }

    configuration get_next_config() {
      if (_search_technique) {
        if (_next_coordinates.empty())
          _next_coordinates = _search_technique->get_next_coordinates();
        return _search_space.get_configuration( *_next_coordinates.begin() );
      } else if (_search_technique_1d) {
        if (_next_indices_1d.empty())
          _next_indices_1d = _search_technique_1d->get_next_indices();
        return _search_space.get_configuration( *_next_indices_1d.begin() );
      }
      throw std::runtime_error("no search technique selected");
    }

    void report_result(cost_t cost) {
      if (_search_technique) {
        _next_costs[ *_next_coordinates.begin() ] = cost;
        _next_coordinates.erase( _next_coordinates.begin() );
        if (_next_coordinates.empty()) {
          _search_technique->report_costs( _next_costs );
          _next_costs.clear();
        }
      } else if (_search_technique_1d) {
        _next_costs_1d[ *_next_indices_1d.begin() ] = cost;
        _next_indices_1d.erase( _next_indices_1d.begin() );
        if (_next_indices_1d.empty()) {
          _search_technique_1d->report_costs( _next_costs_1d );
          _next_costs_1d.clear();
        }
      }
    }

};


} // namespace "atf"

#include "detail/exploration_engine_def.hpp"

#endif /* exploration_engine_h */
