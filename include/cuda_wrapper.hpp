#pragma once

#ifndef LIBATF_DISABLE_CUDA


#include <stdio.h>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <iostream>
#include <ctime>
#include <chrono>
#include <tuple>
#include <functional>

#include <cuda.h>
#include <cuda_runtime.h>
#include <nvrtc.h>

#include "tp_value.hpp"
#include "helper.hpp"

using namespace std::literals::string_literals;

namespace atf
{
namespace cf
{
using cuda_device_id = int;
namespace detail
{
template< typename T = ::std::runtime_error >
auto nvrtc_safe_call(nvrtcResult p_result, const ::std::string& p_message)
-> void
{
  if(p_result != NVRTC_SUCCESS)
  {
    std::cout << nvrtcGetErrorString(p_result) << ": " << p_message << std::endl;
    throw T{p_message};
    ::std::exit(EXIT_FAILURE);
  }
}


template< typename T = ::std::runtime_error >
auto cuda_safe_call(cudaError_t p_result, const ::std::string& p_message)
-> void
{
  if(p_result != cudaSuccess)
  {
    std::cout << cudaGetErrorString(p_result) << ": " << p_message << std::endl;
    throw T{p_message};
    ::std::exit(EXIT_FAILURE);
  }
}

template< typename T = ::std::runtime_error >
auto cuda_safe_call(CUresult p_result, const ::std::string& p_message)
-> void
{
  if(p_result != CUDA_SUCCESS)
  {
    const char* error;
    cuGetErrorName(p_result, &error);
    std::cout << error << ": " << p_message << std::endl;
    throw T{p_message};
    ::std::exit(EXIT_FAILURE);
  }
}


template<typename... Ts>
class cuda_wrapper
{
    using this_type = cuda_wrapper< Ts... >;
    using input_type = ::std::tuple<Ts...>;
    using block_dim_type = ::std::tuple<tp_int_expression, tp_int_expression, tp_int_expression>;
    using grid_dim_type = ::std::tuple<tp_int_expression, tp_int_expression, tp_int_expression>;
    using return_type = size_t;
    template<typename T>
    using single_gold_callable_t = std::function<typename T::host_type(const typename Ts::host_type&...)>;
    using all_gold_callable_t = std::function<void(const typename Ts::host_type&...)>;

  public:
    cuda_wrapper() = delete;
    cuda_wrapper(const this_type&) = delete;
    cuda_wrapper(this_type&&) = default;

    const this_type& operator=(const this_type&) = delete;
    this_type& operator=(this_type&&) = default;

  public:
    cuda_wrapper(  cuda_device_id p_devId,
                   const kernel_info& p_krnlInfo,
                   const input_type& p_krnlInputs,
                   const grid_dim_type& p_gridDim,
                   const block_dim_type& p_blockDim
    )
        :  m_DeviceID{ p_devId },
           m_KernelSource{ p_krnlInfo.source() },
           m_KernelName{ p_krnlInfo.name() },
           m_KernelFlags{ p_krnlInfo.flags() },
           m_KernelInputs{ p_krnlInputs },
           m_GridDimensions{ p_gridDim },
           m_BlockDimensions{ p_blockDim }
    {
      m_KernelBuffers.reserve( sizeof...(Ts) );
      m_CheckResult.fill(false);
      m_NumWrongResults = 0;
      this->create_program();
      this->init_cuda();
      this->create_buffers();
      this->fill_buffers( true );
    }

    ~cuda_wrapper()
    {
      if( std::any_of(m_CheckResult.begin(), m_CheckResult.end(), [](auto b) { return b; }) && m_NumWrongResults > 0 )
        std::cout << "\nnumber of wrong results: " << m_NumWrongResults << std::endl;
    }

  public:

    void warm_ups(size_t warm_ups) {
      m_Warmups = warm_ups;
    }

    void evaluations(size_t evaluations) {
      m_Evaluations = evaluations;
    }


    template<size_t index>
    void check_result(const typename NthTypeOf<index, Ts...>::host_type& gold_data, const atf::comparator<typename NthTypeOf<index, Ts...>::elem_type>& comparator = atf::equality()) {
      std::get<index>(m_GoldData) = gold_data;
      std::get<index>(m_GoldComparator) = comparator;
      std::get<index>(m_CheckResult) = true;
    }

    auto operator()(configuration& p_cfg)
    -> return_type
    {
      // Update tuning parameters
      this->update_tps(p_cfg);

      // Compile kernel
      this->compile_kernel(p_cfg);

      // Run kernel
      return this->run_kernel();
    }

  private:
    size_t buffer_pos = 0;

    auto run_kernel()
    -> return_type
    {
      // Retrieve PTX
      size_t t_ptxSize;
      nvrtc_safe_call<>(nvrtcGetPTXSize(m_Program, &t_ptxSize), "Failed to retrieve PTX size");

      ::std::vector<char> t_ptxCode(t_ptxSize);
      nvrtc_safe_call<>(nvrtcGetPTX(m_Program, t_ptxCode.data()), "Failed to retrieve PTX code");

      // Load PTX
      CUmodule t_module;
      CUfunction t_kernel;

      cuda_safe_call<>(cuModuleLoadDataEx(&t_module, t_ptxCode.data(), 0, 0, 0), "Failed to load module data");
      cuda_safe_call<>(cuModuleGetFunction(&t_kernel, t_module, m_KernelName.c_str()), "Failed to retrieve kernel handle");

      // Retrieve grid and block dimensions
      const int gd0 = (int) std::get<0>(m_GridDimensions).evaluate();
      const int gd1 = (int) std::get<1>(m_GridDimensions).evaluate();
      const int gd2 = (int) std::get<2>(m_GridDimensions).evaluate();

      const int bd0 = (int) std::get<0>(m_BlockDimensions).evaluate();
      const int bd1 = (int) std::get<1>(m_BlockDimensions).evaluate();
      const int bd2 = (int) std::get<2>(m_BlockDimensions).evaluate();

      // warm ups
      for( size_t i = 0 ; i < m_Warmups ; ++i )
      {
        // Reset device data
        this->fill_buffers(false);

        // Launch kernel
        const auto t_kernelResult = cuLaunchKernel(
            t_kernel,
            gd0, gd1, gd2,
            bd0, bd1, bd2,
            0,
            nullptr,
            m_KernelInputPtrs.data(),
            nullptr
        );

        // Check for success
        if(t_kernelResult != CUDA_SUCCESS) {
          const char* error;
          cuda_safe_call<>(cuGetErrorName(t_kernelResult, &error), "failed to retrieve kernel error");
          throw ::std::runtime_error(error);
        }
      }

      // evaluations
      float t_runtimeInMs = 0;
      for( size_t i = 0 ; i < m_Evaluations ; ++i )
      {
        // Create benchmark events
        cudaEvent_t t_start, t_stop;
        cuda_safe_call<>(cudaEventCreate(&t_start), "Failed to create events");
        cuda_safe_call<>(cudaEventCreate(&t_stop), "Failed to create events");

        // Reset device data
        this->fill_buffers(false);

        // Launch kernel
        cuda_safe_call<>(cudaEventRecord(t_start, 0), "Failed to record start event");
        const auto t_kernelResult = cuLaunchKernel(
            t_kernel,
            gd0, gd1, gd2,
            bd0, bd1, bd2,
            0,
            nullptr,
            m_KernelInputPtrs.data(),
            nullptr
        );
        cuda_safe_call<>(cudaEventRecord(t_stop, 0), "Failed to record stop event");

        // Check for success
        if(t_kernelResult != CUDA_SUCCESS) {
          const char* error;
          cuda_safe_call<>(cuGetErrorName(t_kernelResult, &error), "failed to retrieve kernel error");
          throw ::std::runtime_error(error);
        }

        // Profiling
        float tmp = 0.f;

        cuda_safe_call<>(cudaEventSynchronize(t_stop), "Failed to synchronize events");
        cuda_safe_call<>(cudaEventElapsedTime(&tmp, t_start, t_stop), "Failed to retrieve elapsed time");
        t_runtimeInMs += tmp;

        cuda_safe_call<>(cudaEventDestroy(t_start), "Failed to destroy events");
        cuda_safe_call<>(cudaEventDestroy(t_stop), "Failed to destroy events");
      }

      // check result
      if( std::any_of(m_CheckResult.begin(), m_CheckResult.end(), [](auto b) { return b; }) )
        check_result_helper( std::make_index_sequence<sizeof...(Ts)>() );

      return static_cast<return_type>(t_runtimeInMs * 1000000 / m_Evaluations);
    }

    auto compile_kernel(configuration& p_cfg)
    -> void
    {
      // Retrieve flags
      ::std::vector<::std::string> t_flags;
      ::std::vector<const char*> t_views;
      this->create_flags(p_cfg, t_flags, t_views);

      // Compile kernel
      const auto t_result = nvrtcCompileProgram(
          m_Program,
          static_cast<int>(t_flags.size()),
          t_views.data()
      );

      // If compilation failed, retrieve log
      if(t_result != NVRTC_SUCCESS)
      {
        // Query log size
        size_t t_logSize;
        nvrtc_safe_call<>(nvrtcGetProgramLogSize(m_Program, &t_logSize), "Failed to retrieve log size");

        // Retrieve log
        ::std::vector<char> t_log(t_logSize);
        nvrtc_safe_call<>(nvrtcGetProgramLog(m_Program, t_log.data()), "Failed to retrieve log");

        ::std::cout << t_log.data() << ::std::endl;

        throw ::std::exception();
      }
    }

    auto create_flags(configuration& p_cfg, ::std::vector<::std::string>& p_dest, ::std::vector<const char*>& p_views)
    -> void
    {

      for(const auto& t_tp: p_cfg)
        p_dest.emplace_back(" -D "s + t_tp.first + "=" + static_cast<::std::string>(t_tp.second.value()));

      for(int i = 0; i < m_KernelBufferSizes.size(); ++i)
        p_dest.emplace_back(" -D N_"s + ::std::to_string(i) + "=" + ::std::to_string(m_KernelBufferSizes[i]));

      // Append additional kernel flags
      if (!m_KernelFlags.empty())
        p_dest.push_back(m_KernelFlags);

      // Create views
      ::std::transform(
          p_dest.begin(), p_dest.end(),
          ::std::back_inserter(p_views),
          [](auto& t_str) -> const char*
          {
            return t_str.c_str();
          }
      );
    }


    auto create_buffers()
    -> void
    {
      this->create_buffers_and_set_args(::std::make_index_sequence<sizeof...(Ts)>());
    }


    auto fill_buffers(bool init)
    -> void
    {
      this->fill_buffers(init, ::std::make_index_sequence<sizeof...(Ts)>());
    }

    auto create_program()
    -> void
    {
      nvrtc_safe_call<>(
          nvrtcCreateProgram(  &m_Program,
                               m_KernelSource.c_str(),
                               m_KernelName.c_str(),
                               0,
                               nullptr,
                               nullptr
          ),
          "Failed to create NVRTC program"
      );
    }

    auto init_cuda()
    -> void
    {
      cuda_safe_call<>(cuInit(0), "Failed to initialize CUDA");
      cuda_safe_call<>(cuDeviceGet(&m_Device, m_DeviceID), "Failed to retrieve specified device");
      cudaDeviceProp prop{};
      cudaGetDeviceProperties(&prop, m_DeviceID);
      std::cout << "CUDA device with name " << prop.name << " found." << std::endl;
      cuda_safe_call<>(cuCtxCreate(&m_Context, 0, m_Device), "Failed to create context");
    }

    auto update_tps(configuration& p_cfg)
    -> void
    {
      // Update all tuning parameters
      for(auto& t_tp: p_cfg)
      {
        auto tp_value = t_tp.second;
        tp_value.update_tp();
      }
    }

  private:
    template< size_t... Is >
    void create_buffers_and_set_args(std::index_sequence<Is...>)
    {
      create_buffers_and_set_args_impl(std::get<Is>(m_KernelInputs)...);
    }

    template< typename T, typename... ARGs >
    void create_buffers_and_set_args_impl( data::scalar<T>& scalar, ARGs&... args )
    {
      //set kernel arg
      m_KernelInputPtrs.emplace_back(scalar.get_ptr());

      create_buffers_and_set_args_impl(args...);
    }

    template< typename T, typename... ARGs >
    void create_buffers_and_set_args_impl( data::buffer_class<T>& buffer, ARGs&... args )
    {
      auto start_time = std::chrono::steady_clock::now();

      m_KernelBufferSizes.emplace_back(buffer.size());

      m_KernelBuffers.emplace_back();
      auto& ptr = m_KernelBuffers.back();

      cuda_safe_call<>(cuMemAlloc(&ptr, buffer.size() * sizeof(T)), "Failed to allocate buffer");

      auto end_time = std::chrono::steady_clock::now();
      auto runtime  = std::chrono::duration_cast<std::chrono::milliseconds>( end_time - start_time ).count();
      std::cout << "Time to create CUDA device buffer: " << runtime << "ms" << std::endl;

      //set kernel arg
      m_KernelInputPtrs.emplace_back(&ptr);

      create_buffers_and_set_args_impl(args...);
    }

    void create_buffers_and_set_args_impl()
    {
    }

    template< size_t... Is >
    void fill_buffers(bool init, std::index_sequence<Is...>)
    {
      buffer_pos = 0;
      fill_buffers_impl(init, std::get<Is>(m_KernelInputs)...);
    }

    template< typename T, typename... ARGs >
    void fill_buffers_impl( bool init, data::scalar<T>& scalar, ARGs&... args )
    {
      fill_buffers_impl(init, args...);
    }

    template< typename T, typename... ARGs >
    void fill_buffers_impl( bool init, data::buffer_class<T>& buffer, ARGs&... args )
    {
      if (buffer.copy_once() == init) {
        auto start_time = std::chrono::steady_clock::now();

        auto& ptr = m_KernelBuffers[buffer_pos];

        cuda_safe_call<>(cuMemcpyHtoD(ptr, buffer.get(), buffer.size() * sizeof(T)), "Failed to copy buffer data to device");

        auto end_time = std::chrono::steady_clock::now();
        auto runtime  = std::chrono::duration_cast<std::chrono::milliseconds>( end_time - start_time ).count();
        if (init)
          std::cout << "Time to fill CUDA device buffer: " << runtime << "ms" << std::endl;
      }

      // increment position of kernel buffer to fill next
      ++buffer_pos;

      fill_buffers_impl(init, args...);
    }

    void fill_buffers_impl(bool init)
    {
    }

    // helper for checking results
    template< size_t... Is >
    void check_result_helper( std::index_sequence<Is...> )
    {
      buffer_pos = 0;
      check_result_helper_impl( std::make_tuple( std::get<Is>( m_CheckResult ) ,
                                                 std::cref(std::get<Is>( m_KernelInputs )) ,
                                                 std::cref(std::get<Is>( m_GoldData )) ,
                                                 std::ref(std::get<Is>( m_GoldComparator )) )... );
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
        ++m_NumWrongResults;
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
        cuda_safe_call<>(cuMemcpyDtoH(device_data.data(), m_KernelBuffers[buffer_pos], buffer.size() * sizeof(T)), "Failed to copy buffer data from device");

        // check device data
        if (device_data.size() != gold_vector.size()) {
          std::cout << "computation finished: RESULT NOT CORRECT ! ! !\n";
          ++m_NumWrongResults;
          throw std::exception();
        }
        for (size_t i = 0; i < device_data.size(); ++i) {
          if( !gold_comparator( device_data[i], gold_vector[i] ) ) {
            std::cout << "computation finished: RESULT NOT CORRECT ! ! !\n";
            ++m_NumWrongResults;
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

  private:
    // --- CUDA Device Info
    cuda_device_id   m_DeviceID;

    // --- Kernel data
    size_t          m_Warmups = 0;
    size_t          m_Evaluations = 1;
    ::std::string   m_KernelSource;
    ::std::string   m_KernelName;
    ::std::string   m_KernelFlags;
    grid_dim_type   m_GridDimensions;
    block_dim_type  m_BlockDimensions;

    // --- Kernel buffers
    ::std::vector<CUdeviceptr>     m_KernelBuffers;
    ::std::vector<int>  m_KernelBufferSizes;

    // --- Kernel inputs
    input_type         m_KernelInputs;
    ::std::vector<void*>   m_KernelInputPtrs;

    // --- NVRCT values
    CUdevice    m_Device;
    CUcontext    m_Context;
    nvrtcProgram  m_Program;

    // --- gold data ---
    std::array<bool, sizeof...(Ts)>                   m_CheckResult;
    size_t                                            m_NumWrongResults;
    std::tuple<typename Ts::host_type...>             m_GoldData;
    std::tuple<comparator<typename Ts::elem_type>...> m_GoldComparator;
};
}




template<typename... Ts>
auto cuda(  cuda_device_id         device,
            const kernel_info&       kernel,
            const ::std::tuple<Ts...>&   kernel_inputs,

            ::std::tuple<tp_int_expression, tp_int_expression, tp_int_expression> grid_dim,
            ::std::tuple<tp_int_expression, tp_int_expression, tp_int_expression> block_dim
)
{
  return detail::cuda_wrapper< Ts... >{
      device,
      kernel,
      kernel_inputs,
      grid_dim,
      block_dim
  };
}



auto grid_dim( tp_int_expression&& grid_dim_0, tp_int_expression&& grid_dim_1 = 1, tp_int_expression&& grid_dim_2 = 1 )
{
  return std::make_tuple( std::forward<tp_int_expression>(grid_dim_0), std::forward<tp_int_expression>(grid_dim_1), std::forward<tp_int_expression>(grid_dim_2) );
}


auto block_dim( tp_int_expression&& block_dim_0, tp_int_expression&& block_dim_1 = 1, tp_int_expression&& block_dim_2 = 1 )
{
  return std::make_tuple( std::forward<tp_int_expression>(block_dim_0), std::forward<tp_int_expression>(block_dim_1), std::forward<tp_int_expression>(block_dim_2) );
}

}
}


#endif

