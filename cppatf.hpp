#ifndef cppatf_hpp
#define cppatf_hpp

#include "include/abort_conditions.hpp"
#include "include/range.hpp"
#include "include/tp.hpp"

#include "include/operators.hpp"
#include "include/predicates.hpp"

#include "include/exhaustive.hpp"
#include "include/simulated_annealing.hpp"
#include "include/random_search.hpp"
#include "include/differential_evolution.hpp"
#include "include/particle_swarm.hpp"
#include "include/pattern_search.hpp"
#include "include/torczon.hpp"
#ifdef ENABLE_OPEN_TUNER_SEARCH_TECHNIQUE
#include "include/open_tuner.hpp"
#endif

#include "include/auc_bandit.hpp"
#include "include/round_robin.hpp"

#include "include/exploration_engine.hpp"

#ifdef ENABLE_OPENCL_COST_FUNCTION
#include "include/ocl_wrapper.hpp"
#endif
#ifdef ENABLE_CUDA_COST_FUNCTION
#include "include/cuda_wrapper.hpp"
#endif
#include "include/cpp_cf.hpp"
#include "include/bash_cf.hpp"

#include <cstdlib>

namespace atf {

// helper for converting interface classes to ATF's internal classes
template<typename T>
auto to_internal_type(T& type) {
    return type;
}

// abort conditions
using evaluations = cond::evaluations;
using valid_evaluations = cond::valid_evaluations;
using speedup = cond::speedup;
template<typename T>
using duration = cond::duration<T>;
using cost = cond::result;

// tuner
class tuner {
  public:
    tuner() = default;
    tuner(const tuner& other) : _engine(other._engine), _is_stepping(false), _stepping_config(), _stepping_start(), _stepping_log(), _log_file(other._log_file) {}
    ~tuner() {
      if (_is_stepping) {
        _engine.finalize();
        if (!_engine._silent) {
          auto end = std::chrono::steady_clock::now();
          auto runtime_in_sec = std::chrono::duration_cast<std::chrono::seconds>( end - _stepping_start ).count();
          std::cout << "\nnumber of evaluated configs: " << _engine._status._number_of_evaluated_configs << " , evaluations required to find best found result: " << _engine._status._evaluations_required_to_find_best_found_result << std::endl;
          std::cout << std::endl << "total runtime for tuning = " << runtime_in_sec << "sec" << std::endl;
          std::cout << "tuning finished" << std::endl;
        }
        _stepping_log.close();
      }
    }

    template< typename... Ts, typename... range_ts, typename... callables >
    tuner& tuning_parameters(tp_t<Ts,range_ts,callables>&... tps) {
      _engine(tps...);
      return *this;
    }

    template< typename... Ts, typename... G_CLASSES >
    tuner& tuning_parameters(G_class<Ts...> G_class, G_CLASSES... G_classes) {
      _engine(G_class, G_classes...);
      return *this;
    }

    template< typename search_technique_t, typename std::enable_if<std::is_base_of<search_technique, search_technique_t>::value, bool>::type = true >
    tuner& search_technique(const search_technique_t& search_technique) {
      _engine.set_search_technique(search_technique);
      return *this;
    }

    template< typename search_technique_t, typename std::enable_if<std::is_base_of<search_technique_1d, search_technique_t>::value, bool>::type = true >
    tuner& search_technique(const search_technique_t& search_technique) {
      _engine.set_search_technique(search_technique);
      return *this;
    }

    tuner& silent(bool silent) {
      _engine.set_silent(silent);
      return *this;
    }

    tuner& log_file(const std::string &log_file) {
      _engine.set_log_file(log_file);
      _log_file = log_file;
      return *this;
    }

    template<typename cf_t, typename abort_condition_t>
    tuning_status tune(cf_t& cf, const abort_condition_t& abort_condition) {
      if (_is_stepping)
        throw std::runtime_error("cannot start tuning while using online tuning");
      _engine.set_abort_condition(abort_condition);
      auto internal_cf = to_internal_type(cf);
      return _engine(internal_cf);
    }

    template<typename cf_t>
    tuning_status tune(cf_t& cf) {
      if (_is_stepping)
        throw std::runtime_error("cannot start tuning while using online tuning");
      _engine.set_abort_condition();
        auto internal_cf = to_internal_type(cf);
      return _engine(internal_cf);
    }

    configuration get_configuration() {
      bool write_header = false;
      if (!_is_stepping) {
        if (!_engine._silent)
          std::cout << "\nsearch space size: " << _engine._search_space.num_configs() << std::endl << std::endl;
        if (_log_file.empty())
          _log_file = "tuning_log_" + atf::timestamp_str() + ".csv";
        _stepping_log.open(_log_file, std::ofstream::out | std::ofstream::trunc);
        _stepping_log.precision(std::numeric_limits<cost_t>::max_digits10);
        _stepping_log << "timestamp;cost";
        write_header = true;
        _stepping_start = std::chrono::steady_clock::now();
        _is_stepping = true;
        _stepping_expects_report_cost = false;
        _engine.initialize();
      }
      if (_stepping_expects_report_cost) {
        throw std::runtime_error("call to report_cost() expected");
      }
      _stepping_config = _engine.get_next_config();
      _engine._status._number_of_evaluated_configs += 1;
      _stepping_expects_report_cost = true;
      // update tp values
      for(auto& tp : _stepping_config) {
        auto tp_value = tp.second;
        tp_value.update_tp();
      }
      if (write_header) {
        for (const auto &tp : _stepping_config) {
          _stepping_log << ";" << tp.first;
        }
      }
      return _stepping_config;
    }

    void report_cost(cost_t cost) {
      if (!_is_stepping) {
        throw std::runtime_error("no tuning in progress");
      }
      if (!_stepping_expects_report_cost) {
        throw std::runtime_error("call to get_configuration() expected");
      }
      _engine.report_result(cost);
      _stepping_expects_report_cost = false;
      _stepping_log << std::endl << atf::timestamp_str() << ";" << cost;
      for (const auto &tp : _stepping_config) {
        _stepping_log << ";" << tp.second.value();
      }
      auto current_best_result = std::get<2>( _engine._status._history.back() );
      if (cost < current_best_result) {
        _engine._status._evaluations_required_to_find_best_found_result = _engine._status._number_of_evaluated_configs;
        _engine._status._history.emplace_back( std::chrono::steady_clock::now(),
                                               _stepping_config,
                                               cost
        );
      }
      if (!_engine._silent)
        std::cout << std::endl << "evaluated configs: " << _engine._status._number_of_evaluated_configs << " , program cost: " << cost << " , current best result: " << _engine._status.min_cost() << std::endl << std::endl;
    }

    template<typename cf_t>
    cost_t make_step(cf_t& cf) {
        auto config = get_configuration();
        auto internal_cf = to_internal_type(cf);
        auto cost = internal_cf(config);
        report_cost(cost);
        return cost;
    }

    const tuning_status& get_tuning_status() {
      if (!_is_stepping) {
        throw std::runtime_error("no tuning in progress");
      }
      return _engine._status;
    }

  private:
    exploration_engine _engine;
    std::string _log_file;

    bool                                       _is_stepping = false;
    bool                                       _stepping_expects_report_cost;
    configuration                              _stepping_config;
    decltype(std::chrono::steady_clock::now()) _stepping_start;
    std::ofstream                              _stepping_log;
};

// helper
template<typename T>
class scalar : public data::scalar<T> {
  public:
    using internal_type = data::scalar<T>;
    using elem_type = T;
    using host_type = elem_type;

    scalar() : data::scalar<elem_type>(T{}) {}
    scalar(std::array<T, 2> interval) : data::scalar<elem_type>(interval[0], interval[1]) {}
    explicit scalar(elem_type value) : data::scalar<elem_type>(value) {}

    host_type get_host_data() const {
      return this->get();
    }

    internal_type& to_internal_type() {
      return *this;
    }
    const internal_type& to_internal_type() const {
      return *this;
    }
};
template<> scalar<bool>::scalar() : data::scalar<elem_type>(false, true) {}
template<> scalar<int>::scalar() : data::scalar<elem_type>(1, 10) {}
template<> scalar<long>::scalar() : data::scalar<elem_type>(1, 10) {}
template<> scalar<long long>::scalar() : data::scalar<elem_type>(1, 10) {}
template<> scalar<unsigned int>::scalar() : data::scalar<elem_type>(1, 10) {}
template<> scalar<unsigned long>::scalar() : data::scalar<elem_type>(1, 10) {}
template<> scalar<unsigned long long>::scalar() : data::scalar<elem_type>(1, 10) {}
template<> scalar<float>::scalar() : data::scalar<elem_type>(0.0f, 1.0f) {}
template<> scalar<double>::scalar() : data::scalar<elem_type>(0.0, 1.0) {}
template<> scalar<long double>::scalar() : data::scalar<elem_type>(0.0, 1.0) {}

template<typename T>
class buffer : public data::buffer_class<T> {
  public:
    using internal_type = data::buffer_class<T>;
    using elem_type = T;
    using host_type = std::vector<elem_type>;

    explicit buffer(bool copy_once = false) : data::buffer_class<elem_type>(0, T{}, T{}, copy_once) {}
    explicit buffer(int size, bool copy_once = false) : data::buffer_class<elem_type>(size, T{}, T{}, copy_once) {}
    explicit buffer(size_t size, bool copy_once = false) : data::buffer_class<elem_type>(size, T{}, T{}, copy_once) {}
    buffer(int size, elem_type elem, bool copy_once = false) : data::buffer_class<elem_type>(std::vector<elem_type>(size, elem), copy_once) {}
    buffer(size_t size, elem_type elem, bool copy_once = false) : data::buffer_class<elem_type>(std::vector<elem_type>(size, elem), copy_once) {}
    buffer(int size, std::function<elem_type(size_t)> generator, bool copy_once = false) : data::buffer_class<elem_type>(create_data_with_generator(size, generator), copy_once) {}
    buffer(size_t size, std::function<elem_type(size_t)> generator, bool copy_once = false) : data::buffer_class<elem_type>(create_data_with_generator(size, generator), copy_once) {}
    explicit buffer(const std::vector<elem_type> &data, bool copy_once = false) : data::buffer_class<elem_type>(data, copy_once) {}

    host_type get_host_data() const {
      return this->get_vector();
    }

    internal_type& to_internal_type() {
      return *this;
    }
    const internal_type& to_internal_type() const {
      return *this;
    }
  private:
    static std::vector<elem_type> create_data_with_generator(size_t size, std::function<elem_type(size_t)> generator) {
      std::vector<elem_type> data(size);
      for (size_t i = 0; i < size; ++i)
        data[i] = generator(i);
      return data;
    }
};
template<> buffer<bool>::buffer(int size, bool copy_once) : data::buffer_class<elem_type>(size, false, true, copy_once) {}
template<> buffer<bool>::buffer(size_t size, bool copy_once) : data::buffer_class<elem_type>(size, false, true, copy_once) {}
template<> buffer<int>::buffer(int size, bool copy_once) : data::buffer_class<elem_type>(size, 1, 10, copy_once) {}
template<> buffer<int>::buffer(size_t size, bool copy_once) : data::buffer_class<elem_type>(size, 1, 10, copy_once) {}
template<> buffer<long>::buffer(int size, bool copy_once) : data::buffer_class<elem_type>(size, 1, 10, copy_once) {}
template<> buffer<long>::buffer(size_t size, bool copy_once) : data::buffer_class<elem_type>(size, 1, 10, copy_once) {}
template<> buffer<long long>::buffer(int size, bool copy_once) : data::buffer_class<elem_type>(size, 1, 10, copy_once) {}
template<> buffer<long long>::buffer(size_t size, bool copy_once) : data::buffer_class<elem_type>(size, 1, 10, copy_once) {}
template<> buffer<unsigned int>::buffer(int size, bool copy_once) : data::buffer_class<elem_type>(size, 1, 10, copy_once) {}
template<> buffer<unsigned int>::buffer(size_t size, bool copy_once) : data::buffer_class<elem_type>(size, 1, 10, copy_once) {}
template<> buffer<unsigned long>::buffer(int size, bool copy_once) : data::buffer_class<elem_type>(size, 1, 10, copy_once) {}
template<> buffer<unsigned long>::buffer(size_t size, bool copy_once) : data::buffer_class<elem_type>(size, 1, 10, copy_once) {}
template<> buffer<unsigned long long>::buffer(int size, bool copy_once) : data::buffer_class<elem_type>(size, 1, 10, copy_once) {}
template<> buffer<unsigned long long>::buffer(size_t size, bool copy_once) : data::buffer_class<elem_type>(size, 1, 10, copy_once) {}
template<> buffer<float>::buffer(int size, bool copy_once) : data::buffer_class<elem_type>(size, 0.0f, 1.0f, copy_once) {}
template<> buffer<float>::buffer(size_t size, bool copy_once) : data::buffer_class<elem_type>(size, 0.0f, 1.0f, copy_once) {}
template<> buffer<double>::buffer(int size, bool copy_once) : data::buffer_class<elem_type>(size, 0.0, 1.0, copy_once) {}
template<> buffer<double>::buffer(size_t size, bool copy_once) : data::buffer_class<elem_type>(size, 0.0, 1.0, copy_once) {}
template<> buffer<long double>::buffer(int size, bool copy_once) : data::buffer_class<elem_type>(size, 0.0, 1.0, copy_once) {}
template<> buffer<long double>::buffer(size_t size, bool copy_once) : data::buffer_class<elem_type>(size, 0.0, 1.0, copy_once) {}

std::string source(const std::string& source) {
    return source;
}

std::string path(const std::string& path) {
    std::ifstream path_stream(path, std::ios::in);
    std::stringstream source;
    source << path_stream.rdbuf();
    path_stream.close();
    return source.str();
}

// cost functions
namespace generic {

class cost_function_class {
  public:
    cost_function_class(const std::string &run_command) : _run_command(run_command) {}

    cost_function_class& compile_command(const std::string &compile_command) {
      _compile_command = compile_command;
      return *this;
    }

    cost_function_class& costfile(const std::string &costfile) {
      _costfile = costfile;
      return *this;
    }

    inline auto to_internal_type() {
      return [=](configuration &configuration) {
        // concat configuration values
        std::stringstream ss;
        for (auto &tp : configuration) {
          ss << tp.first << "=" << tp.second << " ";
        }
        ss << "bash -c '";

        // execute compile script if defined
        if (!_compile_command.empty()) {
          auto ret = system((ss.str() + _compile_command + "'").c_str());
          if (ret != 0) {
            throw std::exception();
          }
        }

        // execute run script
        auto start = std::chrono::steady_clock::now();
        auto ret = system((ss.str() + _run_command + "'").c_str());
        auto end = std::chrono::steady_clock::now();
        if (ret != 0) {
          throw std::exception();
        }
        size_t cost = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        // read cost from file if defined
        if (!_costfile.empty()) {
          std::ifstream cost_in;
          cost_in.open(_costfile, std::ifstream::in);
          ss.clear();
          cost = std::numeric_limits<size_t>::max();
          if (!(cost_in >> cost)) {
            std::cerr << "could not read runtime from costfile: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
          }
          cost_in.close();
        }

        return cost;
      };
    }
  private:
    std::string _run_command;
    std::string _compile_command;
    std::string _costfile;
};

auto cost_function(const std::string &run_command) {
  return cost_function_class(run_command);
}

};

namespace cxx {

template<typename T, typename... Ts>
auto cost_function( Ts&&... args ) {
  return [&](atf::configuration &config) -> cost_t {
    auto tunable = T(config);

    auto start = std::chrono::steady_clock::now();

    tunable( args... );

    auto end = std::chrono::steady_clock::now();
    auto cost = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    return static_cast<cost_t>(cost);
  };
}

};

template<>
auto to_internal_type(generic::cost_function_class& type) {
    return type.to_internal_type();
}

#ifdef ENABLE_OPENCL_COST_FUNCTION
namespace opencl {

template<typename... Ts>
class kernel: public cf::kernel_info {
  public:
    using kernel_info::kernel_info;
};

template<typename... Ts>
class cost_function_class {
  public:
    using internal_type = cf::ocl_cf_class<typename Ts::internal_type...>;
    template<typename T>
    using single_gold_callable_t = std::function<typename T::host_type(const typename Ts::host_type&...)>;
    using all_gold_callable_t = std::function<void(typename Ts::host_type&...)>;

    explicit cost_function_class(const kernel<Ts...> &kernel) : _kernel(kernel) {
      _check_type.fill(0);
    }

    cost_function_class<Ts...>& platform_id(size_t platform_id) {
      _platform_id = platform_id;
      return *this;
    }
    cost_function_class<Ts...>& device_id(size_t device_id) {
      _device_id = device_id;
      return *this;
    }
    cost_function_class<Ts...>& inputs(Ts&&... args) {
      _inputs = std::make_tuple(args...);
      return *this;
    }
    cost_function_class<Ts...>& global_size(tp_int_expression&& gs_0, tp_int_expression&& gs_1 = 1, tp_int_expression&& gs_2 = 1) {
      _global_size = cf::GS(std::forward<tp_int_expression>(gs_0),
                            std::forward<tp_int_expression>(gs_1),
                            std::forward<tp_int_expression>(gs_2));
      return *this;
    }
    cost_function_class<Ts...>& local_size(tp_int_expression&& ls_0, tp_int_expression&& ls_1 = 1, tp_int_expression&& ls_2 = 1) {
      _local_size = cf::GS(std::forward<tp_int_expression>(ls_0),
                           std::forward<tp_int_expression>(ls_1),
                           std::forward<tp_int_expression>(ls_2));
      return *this;
    }
    template<size_t index>
    cost_function_class<Ts...>& check_result(const typename NthTypeOf<index, Ts...>::host_type& gold_data, const comparator<typename NthTypeOf<index, Ts...>::elem_type>& comparator = equality()) {
      std::get<index>(_gold_data) = gold_data;
      std::get<index>(_gold_comparator) = comparator;
      std::get<index>(_check_type) = 1;
      return *this;
    }
    template<size_t index>
    cost_function_class<Ts...>& check_result(const single_gold_callable_t<NthTypeOf<index, Ts...>>& gold_callable, const comparator<typename NthTypeOf<index, Ts...>::elem_type>& comparator = equality()) {
      std::get<index>(_single_gold_callables) = gold_callable;
      std::get<index>(_gold_comparator) = comparator;
      std::get<index>(_check_type) = 2;
      return *this;
    }
    cost_function_class<Ts...>& check_result(const all_gold_callable_t& gold_callable, const std::tuple<comparator<typename Ts::elem_type>...>& comparator = {}) {
      _all_gold_callable = gold_callable;
      _gold_comparator = comparator;
      _check_type.fill(3);
      return *this;
    }

    cost_function_class<Ts...>& warmups(size_t warmups) {
      _warmups = warmups;
      return *this;
    }

    cost_function_class<Ts...>& evaluations(size_t evaluations) {
      _evaluations = evaluations;
      return *this;
    }

    inline auto to_internal_type() {
      return to_internal_type_impl(std::make_index_sequence<sizeof...(Ts)>{});
    }
  private:
    const kernel<Ts...> &_kernel;
    size_t _platform_id = 0;
    size_t _device_id = 0;
    std::tuple<Ts...> _inputs;
    std::tuple<typename Ts::host_type...> _host_inputs;
    std::array<int, sizeof...(Ts)> _check_type;
    std::tuple<comparator<typename Ts::elem_type>...> _gold_comparator;
    std::tuple<typename Ts::host_type...> _gold_data;
    std::tuple<single_gold_callable_t<Ts>...> _single_gold_callables;
    all_gold_callable_t _all_gold_callable;
    std::tuple<typename Ts::host_type...> _all_gold_callable_results;
    std::tuple<tp_int_expression, tp_int_expression, tp_int_expression> _global_size{1, 1, 1};
    std::tuple<tp_int_expression, tp_int_expression, tp_int_expression> _local_size{1, 1, 1};
    size_t _warmups = 0;
    size_t _evaluations = 1;

    template<size_t... Is>
    inline auto to_internal_type_impl(std::index_sequence<Is...>) {
      internal_type internal_type_object{
          {_platform_id, _device_id},
          {_kernel.source(), _kernel.name(), _kernel.flags()},
          data::inputs(
              std::get<Is>(_inputs).to_internal_type()...
          ),
          _global_size,
          _local_size
      };
      internal_type_object.warm_ups(_warmups);
      internal_type_object.evaluations(_evaluations);
      bool calls_all_gold_callable = std::any_of(_check_type.begin(), _check_type.end(), [](auto t) {return t == 3;});
      if (calls_all_gold_callable) {
        _all_gold_callable_results = std::make_tuple(
            std::get<Is>(_inputs).get_host_data()...
        );
        _all_gold_callable(std::get<Is>(_all_gold_callable_results)...);
      }
      call_check_result_impl<true, Is...>( internal_type_object ); // bool template parameter necessary for call to call_check_result_impl with no indices
      return internal_type_object;
    }

    template<bool, size_t index, size_t... indices>
    inline void call_check_result_impl( internal_type& internal_type_object ) {
      auto check_type = std::get<index>(_check_type);
      switch(check_type) {
        case 1:
          internal_type_object.template check_result<index>(std::get<index>(_gold_data), std::get<index>(_gold_comparator));
          break;
        case 2:
          call_single_gold_callable_impl<index>( internal_type_object, std::make_index_sequence<sizeof...(Ts)>{} );
          break;
        case 3:
          internal_type_object.template check_result<index>(std::get<index>(_all_gold_callable_results), std::get<index>(_gold_comparator));
          break;
      }
      call_check_result_impl<true, indices...>( internal_type_object );
    }
    template<bool>
    inline void call_check_result_impl( internal_type& internal_type_object ) {
    }

    template<size_t index, size_t... all_indices>
    void call_single_gold_callable_impl( internal_type& internal_type_object, std::index_sequence<all_indices...> ) {
      auto callable = std::get<index>(_single_gold_callables);
      internal_type_object.template check_result<index>(callable(std::get<all_indices>(_inputs).get_host_data()...), std::get<index>(_gold_comparator));
    }
};

template<typename... Ts>
auto cost_function(const kernel<Ts...> &kernel) {
  return cost_function_class<Ts...>(kernel);
}

cl::Device get_device(size_t platform_id = 0, size_t device_id = 0) {
    // get platform
    std::vector<cl::Platform> platforms;
    auto error = cl::Platform::get( &platforms ); atf::cf::check_error( error );
    if (platform_id >= platforms.size()) {
        throw std::runtime_error("No OpenCL platform with id " + std::to_string(platform_id));
    }

    // get device
    std::vector<cl::Device> devices;
    error = platforms[ platform_id ].getDevices( CL_DEVICE_TYPE_ALL, &devices ); atf::cf::check_error( error );

    if (device_id >= devices.size()) {
        throw std::runtime_error("No OpenCL platform with id " + std::to_string(platform_id));
    }
    return devices[device_id];
}

auto local_mem_size(size_t platform_id = 0, size_t device_id = 0) {
    auto device = get_device(platform_id, device_id);
    cl_ulong local_mem_size = 0;
    device.getInfo(CL_DEVICE_LOCAL_MEM_SIZE, &local_mem_size);
    return local_mem_size;
}

auto max_work_item_sizes(size_t platform_id = 0, size_t device_id = 0) {
    auto device = get_device(platform_id, device_id);
    cl_uint max_work_item_dimensions = 0;
    device.getInfo(CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, &max_work_item_dimensions);
    std::vector<size_t> max_work_item_sizes(max_work_item_dimensions);
    device.getInfo(CL_DEVICE_MAX_WORK_ITEM_SIZES, &max_work_item_sizes);
    return max_work_item_sizes;
}

auto max_work_group_size(size_t platform_id = 0, size_t device_id = 0) {
    auto device = get_device(platform_id, device_id);
    size_t max_work_group_size = 0;
    device.getInfo(CL_DEVICE_MAX_WORK_GROUP_SIZE, &max_work_group_size);
    return max_work_group_size;
}

};

template<typename... Ts>
auto to_internal_type(opencl::cost_function_class<Ts...>& type) {
    return type.to_internal_type();
}
#endif

#ifdef ENABLE_CUDA_COST_FUNCTION
namespace cuda {

template<typename... Ts>
class kernel: public cf::kernel_info {
  public:
    using kernel_info::kernel_info;
};

template<typename... Ts>
class cost_function_class {
  public:
    using internal_type = cf::detail::cuda_wrapper<typename Ts::internal_type...>;
    template<typename T>
    using single_gold_callable_t = std::function<typename T::host_type(const typename Ts::host_type&...)>;
    using all_gold_callable_t = std::function<void(typename Ts::host_type&...)>;

    explicit cost_function_class(const kernel<Ts...> &kernel) : _kernel(kernel) {
      _check_type.fill(0);
    }

    cost_function_class<Ts...>& device_id(int device_id) {
      _device_id = device_id;
      return *this;
    }
    cost_function_class<Ts...>& inputs(Ts&&... args) {
      _inputs = std::make_tuple(args...);
      return *this;
    }
    cost_function_class<Ts...>& grid_dim(tp_int_expression&& gd_0, tp_int_expression&& gd_1 = 1, tp_int_expression&& gd_2 = 1) {
      _grid_dim = cf::grid_dim(std::forward<tp_int_expression>(gd_0),
                               std::forward<tp_int_expression>(gd_1),
                               std::forward<tp_int_expression>(gd_2));
      return *this;
    }
    cost_function_class<Ts...>& block_dim(tp_int_expression&& bd_0, tp_int_expression&& bd_1 = 1, tp_int_expression&& bd_2 = 1) {
      _block_dim = cf::block_dim(std::forward<tp_int_expression>(bd_0),
                                 std::forward<tp_int_expression>(bd_1),
                                 std::forward<tp_int_expression>(bd_2));
      return *this;
    }
    template<size_t index>
    cost_function_class<Ts...>& check_result(const typename NthTypeOf<index, Ts...>::host_type& gold_data, const comparator<typename NthTypeOf<index, Ts...>::elem_type>& comparator = equality()) {
      std::get<index>(_gold_data) = gold_data;
      std::get<index>(_gold_comparator) = comparator;
      std::get<index>(_check_type) = 1;
      return *this;
    }
    template<size_t index>
    cost_function_class<Ts...>& check_result(const single_gold_callable_t<NthTypeOf<index, Ts...>>& gold_callable, const comparator<typename NthTypeOf<index, Ts...>::elem_type>& comparator = equality()) {
      std::get<index>(_single_gold_callables) = gold_callable;
      std::get<index>(_gold_comparator) = comparator;
      std::get<index>(_check_type) = 2;
      return *this;
    }
    cost_function_class<Ts...>& check_result(const all_gold_callable_t& gold_callable, const std::tuple<comparator<typename Ts::elem_type>...>& comparator = {}) {
      _all_gold_callable = gold_callable;
      _gold_comparator = comparator;
      _check_type.fill(3);
      return *this;
    }

    cost_function_class<Ts...>& warmups(size_t warmups) {
      _warmups = warmups;
      return *this;
    }

    cost_function_class<Ts...>& evaluations(size_t evaluations) {
      _evaluations = evaluations;
      return *this;
    }

    inline auto to_internal_type() {
      return to_internal_type_impl(std::make_index_sequence<sizeof...(Ts)>{});
    }
  private:
    const kernel<Ts...> &_kernel;
    int _device_id = 0;
    std::tuple<Ts...> _inputs;
    std::tuple<typename Ts::host_type...> _host_inputs;
    std::array<int, sizeof...(Ts)> _check_type;
    std::tuple<comparator<typename Ts::elem_type>...> _gold_comparator;
    std::tuple<typename Ts::host_type...> _gold_data;
    std::tuple<single_gold_callable_t<Ts>...> _single_gold_callables;
    all_gold_callable_t _all_gold_callable;
    std::tuple<typename Ts::host_type...> _all_gold_callable_results;
    std::tuple<tp_int_expression, tp_int_expression, tp_int_expression> _grid_dim{1, 1, 1};
    std::tuple<tp_int_expression, tp_int_expression, tp_int_expression> _block_dim{1, 1, 1};
    size_t _warmups = 0;
    size_t _evaluations = 1;

    template<size_t... Is>
    inline auto to_internal_type_impl(std::index_sequence<Is...>) {
      internal_type internal_type_object{
          {_device_id},
          {_kernel.source(), _kernel.name(), _kernel.flags()},
          data::inputs(
              std::get<Is>(_inputs).to_internal_type()...
          ),
          _grid_dim,
          _block_dim
      };
      internal_type_object.warm_ups(_warmups);
      internal_type_object.evaluations(_evaluations);
      bool calls_all_gold_callable = std::any_of(_check_type.begin(), _check_type.end(), [](auto t) {return t == 3;});
      if (calls_all_gold_callable) {
        _all_gold_callable_results = std::make_tuple(
            std::get<Is>(_inputs).get_host_data()...
        );
        _all_gold_callable(std::get<Is>(_all_gold_callable_results)...);
      }
      call_check_result_impl<true, Is...>( internal_type_object ); // bool template parameter necessary for call to call_check_result_impl with no indices
      return internal_type_object;
    }

    template<bool, size_t index, size_t... indices>
    inline void call_check_result_impl( internal_type& internal_type_object ) {
      auto check_type = std::get<index>(_check_type);
      switch(check_type) {
        case 1:
          internal_type_object.template check_result<index>(std::get<index>(_gold_data), std::get<index>(_gold_comparator));
          break;
        case 2:
          call_single_gold_callable_impl<index>( internal_type_object, std::make_index_sequence<sizeof...(Ts)>{} );
          break;
        case 3:
          internal_type_object.template check_result<index>(std::get<index>(_all_gold_callable_results), std::get<index>(_gold_comparator));
          break;
      }
      call_check_result_impl<true, indices...>( internal_type_object );
    }
    template<bool>
    inline void call_check_result_impl( internal_type& internal_type_object ) {
    }

    template<size_t index, size_t... all_indices>
    void call_single_gold_callable_impl( internal_type& internal_type_object, std::index_sequence<all_indices...> ) {
      auto callable = std::get<index>(_single_gold_callables);
      internal_type_object.template check_result<index>(callable(std::get<all_indices>(_inputs).get_host_data()...), std::get<index>(_gold_comparator));
    }
};

template<typename... Ts>
auto cost_function(const kernel<Ts...> &kernel) {
  return cost_function_class<Ts...>(kernel);
}

};

template<typename... Ts>
auto to_internal_type(cuda::cost_function_class<Ts...>& type) {
    return type.to_internal_type();
}
#endif

};


#endif /* cppatf_hpp */
