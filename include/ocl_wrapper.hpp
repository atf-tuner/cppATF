
#ifndef ocl_wrapper_h
#define ocl_wrapper_h


#include <stdio.h>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <type_traits>
#include <utility>
#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#undef  __CL_ENABLE_EXCEPTIONS

#include "tp_value.hpp"
#include "helper.hpp"


namespace atf
{

namespace cf
{
using nd_range_t              = std::array<size_t,3>;
using thread_configurations_t = std::map< ::atf::configuration, std::array<nd_range_t,2> >;


void check_error(cl_int err)
{
  if (err != CL_SUCCESS)
  {
    printf("Error with errorcode: %d\n", err);
  }
}

auto GS( tp_int_expression&& gs_0, tp_int_expression&& gs_1 = 1, tp_int_expression&& gs_2 = 1 )
{
  return std::make_tuple( std::forward<tp_int_expression>(gs_0), std::forward<tp_int_expression>(gs_1), std::forward<tp_int_expression>(gs_2) );
}


auto LS( tp_int_expression&& ls_0, tp_int_expression&& ls_1 = 1, tp_int_expression&& ls_2 = 1 )
{
  return std::make_tuple( std::forward<tp_int_expression>(ls_0), std::forward<tp_int_expression>(ls_1), std::forward<tp_int_expression>(ls_2) );
}


class device_info
{
  public:
    enum device_t { CPU, GPU, ACC };

    device_info( cl_device_id device_id )
        : _platform(), _device( cl::Device( device_id ) )
    {
      _platform = _device.getInfo<CL_DEVICE_PLATFORM>();
    }

    device_info( size_t platform_id, size_t device_id)
        : _platform(), _device()
    {
      // get platform
      std::vector<cl::Platform> platforms;
      auto error = cl::Platform::get( &platforms ); check_error( error );

      if( platform_id >= platforms.size() )
      {
        std::cout << "No OpenCL platform with id " << platform_id << std::endl;
        exit( 1 );
      }

      _platform = platforms[ platform_id ];
      std::string platform_name;
      _platform.getInfo( CL_PLATFORM_VENDOR, &platform_name );
      std::cout << "OpenCL platform with name " << platform_name << " found." << std::endl;

      // get device
      std::vector<cl::Device> devices;
      error = _platform.getDevices( CL_DEVICE_TYPE_ALL, &devices ); check_error( error );

      if( device_id >= devices.size() )
      {
        std::cout << "No OpenCL device with id " << device_id << " for OpenCL platform with id " << platform_id << std::endl;
        exit( 1 );
      }

      _device = devices[ device_id ];
      std::string device_name;
      _device.getInfo( CL_DEVICE_NAME, &device_name );
      std::cout << "OpenCL device with name " << device_name << " found." << std::endl;
    }

    device_info( const std::string& vendor_name,
                 const device_t&    device_type,
                 const int&         device_number
    )
        : _platform(), _device()
    {
      cl_device_type ocl_device_type;
      switch( device_type )
      {
        case CPU:
          ocl_device_type = CL_DEVICE_TYPE_CPU;
          break;

        case GPU:
          ocl_device_type = CL_DEVICE_TYPE_GPU;
          break;

        case ACC:
          ocl_device_type = CL_DEVICE_TYPE_ACCELERATOR;
          break;

        default:
          assert( false && "unknown OpenCL device type" );
          break;
      }

      // get platform
      std::vector<cl::Platform> platforms;

      bool found = false;
      auto error = cl::Platform::get( &platforms ); check_error( error );
      for( const auto& platform : platforms )
      {
        std::string platform_name;
        platform.getInfo( CL_PLATFORM_VENDOR, &platform_name );
        if( platform_name.find( vendor_name ) != std::string::npos )
        {
          _platform = platform;
          std::cout << "OpenCL platform with name " << platform_name << " found." << std::endl;
          found = true;
          break;
        }
      }

      if( !found )
      {
        std::cout << "OpenCL platform not found." << std::endl;
        exit( 1 );
      }

      // get device
      std::vector<cl::Device> devices;
      error = _platform.getDevices( ocl_device_type, &devices ); check_error( error );

      if( device_number < devices.size() )
      {
        _device = devices[ device_number ];
        std::string device_name;
        _device.getInfo( CL_DEVICE_NAME, &device_name );
        std::cout << "OpenCL device with name " << device_name << " found." << std::endl;
      }
      else
      {
        std::cout << "OpenCL device not found." << std::endl;
        exit( 1 );
      }
    }

    cl::Platform platform() const
    {
      return _platform;
    }


    cl::Device device() const
    {
      return _device;
    }


  private:
    cl::Platform _platform;
    cl::Device   _device;
};

template< typename... Ts >
class ocl_cf_class
{
  private:
    // helper
    cl_int  error;
    cl_uint arg_index  = 0;
    size_t  buffer_pos = 0;

  public:
    ocl_cf_class( const device_info&             device,
                  const kernel_info&             kernel,
                  const std::tuple<Ts...>&       kernel_inputs,
                  std::tuple< tp_int_expression, tp_int_expression, tp_int_expression > global_size,
                  std::tuple< tp_int_expression, tp_int_expression, tp_int_expression > local_size
    )
        : _platform( device.platform() ), _device( device.device() ), _context(), _command_queue(), _program(), _kernel_source( kernel.source() ), _kernel_name( kernel.name() ), _kernel_flags( kernel.flags() ), _kernel_inputs( kernel_inputs ), _kernel_buffers(), _kernel_input_sizes(), _global_size_pattern( global_size ), _local_size_pattern( local_size ), _thread_configuration( nullptr ), _check_result( ), _num_wrong_results( 0 )
    {
      _check_result.fill(false);
      cl_context_properties props[] = { CL_CONTEXT_PLATFORM,
                                        reinterpret_cast<cl_context_properties>( _platform() ),
                                        0
      };

      _context       = cl::Context( VECTOR_CLASS<cl::Device>( 1, _device ), props );
      _command_queue = cl::CommandQueue( _context, _device, CL_QUEUE_PROFILING_ENABLE );

      // create program
      _program = cl::Program( _context,
                              cl::Program::Sources( 1, std::make_pair( _kernel_source.c_str(), _kernel_source.length() ) )
      );

      // create kernel input buffers
      this->create_buffers( std::make_index_sequence<sizeof...(Ts)>() );
      this->fill_buffers( true, std::make_index_sequence<sizeof...(Ts)>() );
    }

    ~ocl_cf_class()
    {
      if( std::any_of(_check_result.begin(), _check_result.end(), [](auto b) { return b; }) && _num_wrong_results > 0 )
        std::cout << "\nnumber of wrong results: " << _num_wrong_results << std::endl;
    }


    auto& save_thread_configuration( thread_configurations_t& thread_configuration )
    {
      _thread_configuration = &thread_configuration;

      return *this;
    }

    void warm_ups(size_t warm_ups) {
      _warm_ups = warm_ups;
    }

    void evaluations(size_t evaluations) {
      _evaluations = evaluations;
    }

    template<size_t index>
    void check_result(const typename NthTypeOf<index, Ts...>::host_type& gold_data, const comparator<typename NthTypeOf<index, Ts...>::elem_type>& comparator = atf::equality()) {
      std::get<index>(_gold_data) = gold_data;
      std::get<index>(_gold_comparator) = comparator;
      std::get<index>(_check_result) = true;
    }

    size_t operator()( configuration& configuration )
    {
      // update tp values
      for( auto& tp : configuration )
      {
        auto tp_value = tp.second;
        tp_value.update_tp();
      }

      size_t gs_0 = std::get<0>( _global_size_pattern ).evaluate();
      size_t gs_1 = std::get<1>( _global_size_pattern ).evaluate();
      size_t gs_2 = std::get<2>( _global_size_pattern ).evaluate();

      size_t ls_0 = std::get<0>( _local_size_pattern ).evaluate();
      size_t ls_1 = std::get<1>( _local_size_pattern ).evaluate();
      size_t ls_2 = std::get<2>( _local_size_pattern ).evaluate();

      // create flags
      std::stringstream flags;

      for( const auto& tp : configuration )
        flags << " -D " << tp.first << "=" << tp.second.value();

      // set additional kernel flags
      flags << " " << _kernel_flags;


      // compile kernel
      try
      {
        auto start = std::chrono::steady_clock::now();

        _program.build( std::vector<cl::Device>( 1, _device ), flags.str().c_str() );

        auto end = std::chrono::steady_clock::now();
        auto runtime_in_sec = std::chrono::duration_cast<std::chrono::milliseconds>( end - start ).count();
      }
      catch( cl::Error& err )
      {
        if( err.err() == CL_BUILD_PROGRAM_FAILURE )
        {
          auto buildLog = _program.getBuildInfo<CL_PROGRAM_BUILD_LOG>( _device );
          std::cout << std::endl << "Build failed! Log:" << std::endl << buildLog << std::endl;
        }

        throw std::exception();
      }

      auto kernel = cl::Kernel( _program, _kernel_name.c_str(), &error ); check_error( error );

      // set kernel arguments
      this->set_kernel_args( kernel, std::make_index_sequence<sizeof...(Ts)>() );

      // start kernel
      cl::Event event;
      cl::NDRange global_size( gs_0, gs_1, gs_2 );
      cl::NDRange local_size( ls_0, ls_1, ls_2 );

      // warm ups
      for( size_t i = 0 ; i < _warm_ups ; ++i )
      {
        this->fill_buffers( false, std::make_index_sequence<sizeof...(Ts)>() );
        error = _command_queue.enqueueNDRangeKernel( kernel, cl::NullRange, global_size, local_size, NULL, &event ); if( error != CL_SUCCESS ) throw std::exception();
      }

      // kernel launch with profiling
      cl_ulong kernel_runtime_in_ns = 0;
      cl_ulong start_time;
      cl_ulong end_time;

      for( size_t i = 0 ; i < _evaluations ; ++i )
      {
        this->fill_buffers( false, std::make_index_sequence<sizeof...(Ts)>() );
        error = _command_queue.enqueueNDRangeKernel( kernel, cl::NullRange, global_size, local_size, NULL, &event ); if( error != CL_SUCCESS ) throw std::exception();
        error = event.wait(); check_error( error );

        event.getProfilingInfo( CL_PROFILING_COMMAND_START, &start_time );
        event.getProfilingInfo( CL_PROFILING_COMMAND_END,   &end_time   );

        kernel_runtime_in_ns += end_time - start_time;
      }

      // check result
      if( std::any_of(_check_result.begin(), _check_result.end(), [](auto b) { return b; }) )
        check_result_helper( std::make_index_sequence<sizeof...(Ts)>() );

      // save thread configuration
      if( _thread_configuration != nullptr )
      {
        nd_range_t gs = { gs_0, gs_1, gs_2 };
        nd_range_t ls = { ls_0, ls_1, ls_2 };
        (*_thread_configuration)[ configuration ] = { gs, ls };
      }

      return kernel_runtime_in_ns / _evaluations;
    }

  private:
    cl::Platform                   _platform;
    cl::Device                     _device;
    cl::Context                    _context;
    cl::CommandQueue               _command_queue;

    cl::Program                    _program;
    std::string                    _kernel_source;
    std::string                    _kernel_name;
    std::string                    _kernel_flags;
    size_t                         _warm_ups = 0;
    size_t                         _evaluations = 1;

    std::tuple<Ts...>              _kernel_inputs;
    std::vector<cl::Buffer>        _kernel_buffers;
    std::vector<size_t>            _kernel_input_sizes;

    std::tuple< tp_int_expression, tp_int_expression, tp_int_expression > _global_size_pattern;
    std::tuple< tp_int_expression, tp_int_expression, tp_int_expression > _local_size_pattern;

    thread_configurations_t*       _thread_configuration;

    std::array<bool, sizeof...(Ts)>                   _check_result;
    size_t                                            _num_wrong_results;
    std::tuple<typename Ts::host_type...>             _gold_data;
    std::tuple<comparator<typename Ts::elem_type>...> _gold_comparator;


    // helper for creating buffers
    template< size_t... Is >
    void create_buffers( std::index_sequence<Is...> )
    {
      create_buffers_impl( std::get<Is>( _kernel_inputs )... );
    }

    template< typename T, typename... ARGs >
    void create_buffers_impl( const data::scalar<T>& scalar, ARGs&... args )
    {
      _kernel_input_sizes.emplace_back( 0 );

      create_buffers_impl( args... );
    }


    template< typename T, typename... ARGs >
    void create_buffers_impl( const data::buffer_class<T>& buffer, ARGs&... args )
    {
      auto start_time = std::chrono::steady_clock::now();

      // add buffer size to _kernel_input_sizes
      _kernel_input_sizes.emplace_back( buffer.size() );

      // create buffer
      _kernel_buffers.emplace_back( _context, CL_MEM_READ_WRITE, buffer.size() * sizeof( T ) );

      auto end_time = std::chrono::steady_clock::now();
      auto runtime  = std::chrono::duration_cast<std::chrono::milliseconds>( end_time - start_time ).count();
      std::cout << "Time to create OpenCL device buffer: " << runtime << "ms" << std::endl;

      create_buffers_impl( args... );
    }


    void create_buffers_impl()
    {}


    // helper for filling buffers
    template< size_t... Is >
    void fill_buffers( bool init, std::index_sequence<Is...> )
    {
      buffer_pos = 0;
      fill_buffers_impl( init, std::get<Is>( _kernel_inputs )... );
    }

    template< typename T, typename... ARGs >
    void fill_buffers_impl( bool init, const data::scalar<T>& scalar, ARGs&... args )
    {
      fill_buffers_impl( init, args... );
    }


    template< typename T, typename... ARGs >
    void fill_buffers_impl( bool init, const data::buffer_class<T>& buffer, ARGs&... args )
    {
      if (buffer.copy_once() == init) {

        auto start_time = std::chrono::steady_clock::now();

        try
        {
          error = _command_queue.enqueueWriteBuffer( _kernel_buffers[buffer_pos], CL_TRUE, 0, buffer.size() * sizeof( T ), buffer.get() ); check_error(error);
        }
        catch(cl::Error& err)
        {
          std::cerr << "ERROR: " << err.what() << "(" << err.err() << ")" << std::endl;
          abort();
        }

        auto end_time = std::chrono::steady_clock::now();
        auto runtime  = std::chrono::duration_cast<std::chrono::milliseconds>( end_time - start_time ).count();
        if (init)
          std::cout << "Time to fill OpenCL device buffer: " << runtime << "ms" << std::endl;

      }

      // increment position of kernel buffer to fill next
      ++buffer_pos;

      fill_buffers_impl( init, args... );
    }


    void fill_buffers_impl(bool init)
    {}

    // helper for set kernel arguments
    template< size_t... Is >
    void set_kernel_args( cl::Kernel& kernel, std::index_sequence<Is...> )
    {
      arg_index  = 0;
      buffer_pos = 0;
      set_kernel_args_impl( kernel, std::get<Is>( _kernel_inputs )... );
    }


    template< typename T, typename... ARGs >
    void set_kernel_args_impl( cl::Kernel& kernel, data::scalar<T> scalar, ARGs... args )
    {
      kernel.setArg( arg_index++, scalar.get() );

      set_kernel_args_impl( kernel, args... );
    }


    template< typename T, typename... ARGs >
    void set_kernel_args_impl( cl::Kernel& kernel, data::buffer_class<T> buffer, ARGs... args )
    {
      kernel.setArg( arg_index++, _kernel_buffers[ buffer_pos++ ] );

      set_kernel_args_impl( kernel, args... );
    }


    template< typename... ARGs >
    void set_kernel_args_impl( cl::Kernel& kernel, cl_mem buffer, ARGs... args )
    {
      kernel.setArg( arg_index++, _kernel_buffers[ buffer_pos++ ] );

      set_kernel_args_impl( kernel, args... );
    }


    void set_kernel_args_impl( cl::Kernel& kernel )
    {}

    // helper for checking results
    template< size_t... Is >
    void check_result_helper( std::index_sequence<Is...> )
    {
      buffer_pos = 0;
      check_result_helper_impl( std::make_tuple( std::get<Is>( _check_result ) ,
                                                 std::cref(std::get<Is>( _kernel_inputs )) ,
                                                 std::cref(std::get<Is>( _gold_data )) ,
                                                 std::ref(std::get<Is>( _gold_comparator )) )... );
    }

    template< typename T, typename... ARGs >
    void check_result_helper_impl( const std::tuple<bool, const data::scalar<T>&, const T&, comparator<T>&>& scalar_and_gold, const ARGs&... args )
    {
      const auto& check_result = std::get<0>(scalar_and_gold);
      const auto& scalar = std::get<1>(scalar_and_gold);
      const auto& gold_value = std::get<2>(scalar_and_gold);
      auto& gold_comparator = std::get<3>(scalar_and_gold);
      if (!gold_comparator)
        gold_comparator = equality();

      // check scalar value
      if( check_result && !gold_comparator(scalar.get(), gold_value) ) {
        std::cout << "computation finished: RESULT NOT CORRECT ! ! !\n";
        ++_num_wrong_results;
        throw std::exception();
      }
      check_result_helper_impl( args... );
    }


    template< typename T, typename... ARGs >
    void check_result_helper_impl( const std::tuple<bool, const data::buffer_class<T>&, const std::vector<T>&, comparator<T>&>& buffer_and_gold, const ARGs&... args )
    {
      const auto& check_result = std::get<0>(buffer_and_gold);
      const auto& buffer = std::get<1>(buffer_and_gold);
      const auto& gold_vector = std::get<2>(buffer_and_gold);
      auto& gold_comparator = std::get<3>(buffer_and_gold);
      if (!gold_comparator)
        gold_comparator = equality();

      if (check_result) {
        // copy data from device to host
        std::vector<T> device_data(buffer.size());
        _command_queue.enqueueReadBuffer( _kernel_buffers[ buffer_pos ], CL_TRUE, 0, buffer.size() * sizeof(T), device_data.data() );

        // check device data
        if (device_data.size() != gold_vector.size()) {
          std::cout << "computation finished: RESULT NOT CORRECT ! ! !\n";
          ++_num_wrong_results;
          throw std::exception();
        }
        for (size_t i = 0; i < device_data.size(); ++i) {
          if( !gold_comparator( device_data[i], gold_vector[i] ) ) {
            std::cout << "computation finished: RESULT NOT CORRECT ! ! !\n";
            ++_num_wrong_results;
            throw std::exception();
          }
        }
      }

      // increment position of kernel buffer to check next
      ++buffer_pos;

      check_result_helper_impl( args... );
    }

    void check_result_helper_impl() {
      std::cout << "computation finished: result correct\n";
    }
};



template< typename... Ts >
auto ocl( const device_info&       device,
          const kernel_info&       kernel,
          const std::tuple<Ts...>& kernel_inputs,

          std::tuple< tp_int_expression, tp_int_expression, tp_int_expression > global_size,
          std::tuple< tp_int_expression, tp_int_expression, tp_int_expression > local_size
)
{
  return ocl_cf_class< Ts... >( device, kernel, kernel_inputs, global_size, local_size );
}


} // namespace cf

} // namespace atf

#endif /* defined( ocl_wrapper_h ) */
