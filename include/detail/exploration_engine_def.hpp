#ifndef exploration_engine_def_h
#define exploration_engine_def_h

#define ATF_EXTENDED_LOG

namespace atf
{


template< typename... TPs >
G_class<TPs...>::G_class( TPs&... tps )
  : _tps( tps... )
{}
  
template< typename... TPs >
auto G_class<TPs...>::tps() const
{
  return _tps;
}

// first application of operator(): IS
template< typename... Ts, typename... range_ts, typename... callables >
exploration_engine& exploration_engine::operator()(tp_t<Ts,range_ts,callables>&... tps )
{
  return this->operator()( G(tps...) );
}

// first application of operator(): IS (required due to ambiguity)
template< typename T, typename range_t, typename callable >
exploration_engine& exploration_engine::operator()(tp_t<T,range_t,callable>& tp )
{
  return this->operator()( G(tp) );
}


// second case
template< typename... Ts, typename... G_CLASSES >
exploration_engine& exploration_engine::operator()(G_class<Ts...> G_class, G_CLASSES... G_classes )
{
  const size_t num_trees = sizeof...(G_CLASSES) + 1;
  _search_space.append_new_trees( num_trees );
  
  insert_tp_names_in_search_space( G_class, G_classes... );
  
  return generate_config_trees<num_trees>( G_class, G_classes... );
}


template< typename callable >
tuning_status exploration_engine::operator()(callable& program ) // func must take config_t and return a value for which "<" is defined.
{
  if (!_silent)
    std::cout << "\nsearch space size: " << _search_space.num_configs() << std::endl << std::endl;
  
  // if no abort condition is specified then iterate over the whole search space.
  if( _abort_condition == NULL )
    _abort_condition = std::unique_ptr<abort_condition>(new cond::evaluations(static_cast<size_t>( _search_space.num_configs() ) ) );

  // if no search technique is specified then use exhaustive search
  if( _search_technique == NULL && _search_technique_1d == NULL )
    set_search_technique( exhaustive() );
 
  // open file for verbose logging
  if (_log_file.empty())
    _log_file = "tuning_log_" + atf::timestamp_str() + ".csv";
  std::ofstream csv_file;
  bool write_header = true;
  csv_file.open(_log_file, std::ofstream::out | std::ofstream::trunc);
  csv_file.precision(std::numeric_limits<cost_t>::max_digits10);
 
  auto start = std::chrono::steady_clock::now();

  initialize();
  
  cost_t program_runtime = std::numeric_limits<cost_t>::max();
  size_t get_next_config_ms, cost_function_ms, report_cost_ms;
  while( !_abort_condition->stop( _status ) )
  {
    auto get_next_config_start = std::chrono::steady_clock::now();
    auto config = get_next_config();
    auto get_next_config_end = std::chrono::steady_clock::now();
    get_next_config_ms = std::chrono::duration_cast<std::chrono::milliseconds>(get_next_config_end - get_next_config_start).count();

    ++_status._number_of_evaluated_configs;
    auto cost_function_start = std::chrono::steady_clock::now();
    try
    {
      program_runtime = program( config );
    }
    catch( ... )
    {
      ++_status._number_of_invalid_configs;
      
      if( _abort_on_error )
        abort();
      else
        program_runtime = std::numeric_limits<cost_t>::max();
    }
    auto cost_function_end = std::chrono::steady_clock::now();
    cost_function_ms = std::chrono::duration_cast<std::chrono::milliseconds>(cost_function_end - cost_function_start).count();
    
    auto current_best_result = std::get<2>( _status._history.back() );
    if( program_runtime < current_best_result  )
    {
      _status._evaluations_required_to_find_best_found_result = _status._number_of_evaluated_configs;
      _status._valid_evaluations_required_to_find_best_found_result = _status.number_of_valid_configs();
      _status._history.emplace_back( std::chrono::steady_clock::now(),
                             config,
                             program_runtime
                           );
    }

    auto report_cost_start = std::chrono::steady_clock::now();
    report_result( program_runtime );
    auto report_cost_end = std::chrono::steady_clock::now();
    report_cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(report_cost_end - report_cost_start).count();

    if (write_header) {
      csv_file << "timestamp;cost";
      for (const auto &tp : config) {
        csv_file << ";" << tp.first;
      }
#ifdef ATF_EXTENDED_LOG
      csv_file << ";get_next_config_ms;cost_function_ms;report_cost_ms";
#endif
      write_header = false;
    }
    csv_file << std::endl << atf::timestamp_str() << ";" << program_runtime;
    for (const auto &tp : config) {
      csv_file << ";" << tp.second.value();
    }
#ifdef ATF_EXTENDED_LOG
    csv_file << ";" << get_next_config_ms << ";" << cost_function_ms << ";" << report_cost_ms;
#endif

    if (!_silent)
      std::cout << std::endl << "evaluated configs: " << _status._number_of_evaluated_configs << " , valid configs: " << _status.number_of_valid_configs() << " , program cost: " << program_runtime << " , current best result: " << _status.min_cost() << std::endl << std::endl;

  }
  
  finalize();
  
  csv_file.close();

  if (!_silent)
    std::cout << "\nnumber of evaluated configs: " << _status._number_of_evaluated_configs << " , number of valid configs: " << _status.number_of_valid_configs() << " , number of invalid configs: " << _status._number_of_invalid_configs << " , evaluations required to find best found result: " << _status._evaluations_required_to_find_best_found_result << " , valid evaluations required to find best found result: " << _status._valid_evaluations_required_to_find_best_found_result << std::endl;

  auto end = std::chrono::steady_clock::now();
  auto runtime_in_sec = std::chrono::duration_cast<std::chrono::seconds>( end - start ).count();
  if (!_silent)
    std::cout << std::endl << "total runtime for tuning = " << runtime_in_sec << "sec" << std::endl;
  
  
  // output
  if (!_silent)
    std::cout << "tuning finished" << std::endl;
  return _status;
}


template< typename... Ts, typename... rest_tp_tuples >
void exploration_engine::insert_tp_names_in_search_space(G_class<Ts...> tp_tuple, rest_tp_tuples... tuples )
{
  insert_tp_names_of_one_tree_in_search_space ( tp_tuple, std::make_index_sequence<sizeof...(Ts)>{} );
  
  insert_tp_names_in_search_space( tuples... );
}


template< typename... Ts, size_t... Is>
void exploration_engine::insert_tp_names_of_one_tree_in_search_space (G_class<Ts...> tp_tuple, std::index_sequence<Is...> ) {
  insert_tp_names_of_one_tree_in_search_space ( std::get<Is>( tp_tuple.tps() )... );
}

template< typename T, typename range_t, typename callable, typename... Ts >
void exploration_engine::insert_tp_names_of_one_tree_in_search_space (tp_t<T,range_t,callable>& tp, Ts&... tps )
{
  _search_space.add_name( tp.name() );
  
  insert_tp_names_of_one_tree_in_search_space ( tps... );
}


template< size_t TREE_ID, typename... Ts, typename... rest_tp_tuples>
exploration_engine& exploration_engine::generate_config_trees(G_class<Ts...> tp_tuple, rest_tp_tuples... tuples )
{
  return generate_config_trees<TREE_ID>( tp_tuple, std::make_index_sequence<sizeof...(Ts)>{}, tuples... );
}


template< size_t TREE_ID, typename... Ts, size_t... Is, typename... rest_tp_tuples >
exploration_engine& exploration_engine::generate_config_trees(G_class<Ts...> tp_tuple, std::index_sequence<Is...>, rest_tp_tuples... tuples )
{
  // fill generated config tree
  const size_t TREE_DEPTH = sizeof...(Is);

#ifdef PARALLEL_SEARCH_SPACE_GENERATION
  _threads.emplace_back( [=](){
#endif
    generate_single_config_tree< TREE_ID, TREE_DEPTH >( std::get<Is>( tp_tuple.tps() )... );
#ifdef PARALLEL_SEARCH_SPACE_GENERATION
  } );
#endif

  return generate_config_trees< TREE_ID - 1 >( tuples... );
}


template< size_t TREE_ID >
exploration_engine& exploration_engine::generate_config_trees()
{
#ifdef PARALLEL_SEARCH_SPACE_GENERATION
  for( auto& thread : _threads )
    thread.join();
  
  _threads.clear();
#endif

  return *this;
}


template< size_t TREE_ID, size_t TREE_DEPTH, typename T, typename range_t, typename callable, typename... Ts, std::enable_if_t<( TREE_DEPTH > 0 )>* >
void exploration_engine::generate_single_config_tree(tp_t<T,range_t,callable>& tp, Ts&... tps )
{
  T value;
  while( tp.get_next_value(value) )
  {
    auto value_tp_pair = std::make_pair( value, static_cast<void*>( tp._act_elem.get() /*&(tp._act_elem)*/ ) );
    generate_single_config_tree< TREE_ID, TREE_DEPTH-1 >( tps..., value_tp_pair );
  }
}


template< size_t TREE_ID, size_t TREE_DEPTH, typename... Ts, std::enable_if_t<( TREE_DEPTH == 0 )>*  >
void exploration_engine::generate_single_config_tree(const Ts&... values )
{
  _search_space.tree( _search_space.num_trees() -  TREE_ID ).insert( values... );   }


template< typename T, typename... Ts >
void exploration_engine::print_path(T val, Ts... tps)
{
  std::cout << val << " ";
  print_path(tps...);
}


} // namespace "atf"

#endif /* exploration_engine_def_h */
