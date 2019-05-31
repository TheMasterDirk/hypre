/*BHEADER**********************************************************************
 * Copyright (c) 2008,  Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * This file is part of HYPRE.  See file COPYRIGHT for details.
 *
 * HYPRE is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (as published by the Free
 * Software Foundation) version 2.1 dated February 1999.
 *
 * $Revision$
 ***********************************************************************EHEADER*/

#ifndef HYPRE_CUDA_UTILS_H
#define HYPRE_CUDA_UTILS_H

#if defined(HYPRE_USING_CUDA)

#ifdef __cplusplus
extern "C++" {
#endif

#include <cuda.h>
#include <cuda_runtime.h>
#include <assert.h>

#include <thrust/execution_policy.h>
#include <thrust/count.h>
#include <thrust/device_ptr.h>
#include <thrust/unique.h>
#include <thrust/sort.h>
#include <thrust/binary_search.h>
#include <thrust/iterator/constant_iterator.h>
#include <thrust/iterator/counting_iterator.h>
#include <thrust/transform.h>
#include <thrust/functional.h>
#include <thrust/gather.h>
#include <thrust/scan.h>
#include <thrust/fill.h>
#include <thrust/adjacent_difference.h>

#define HYPRE_WARP_SIZE      32
#define HYPRE_WARP_FULL_MASK 0xFFFFFFFF
#define HYPRE_MAX_NUM_WARPS  (64 * 64 * 32)
#define HYPRE_FLT_LARGE      1e30
#define HYPRE_1D_BLOCK_SIZE  512

/* macro for launching CUDA kernels */
#define HYPRE_CUDA_LAUNCH HYPRE_CUDA_LAUNCH_ASYNC

#define HYPRE_CUDA_LAUNCH_ASYNC(kernel_name, gridsize, blocksize, ...) \
{ \
   if ( gridsize.x  == 0 || gridsize.y  == 0 || gridsize.z  == 0 || \
        blocksize.x == 0 || blocksize.y == 0 || blocksize.z == 0 ) \
   { \
      hypre_printf("Warning %s %d: Zero CUDA grid/block (%d %d %d) (%d %d %d)\n", \
                   __FILE__, __LINE__,\
                   gridsize.x, gridsize.y, gridsize.z, blocksize.x, blocksize.y, blocksize.z); \
   } \
   else \
   { \
      (kernel_name) <<< (gridsize), (blocksize) >>> (__VA_ARGS__); \
   } \
}

#define HYPRE_CUDA_LAUNCH_SYNC(kernel_name, gridsize, blocksize, ...) \
    HYPRE_CUDA_LAUNCH_ASYNC(kernel_name, gridsize, blocksize, __VA_ARGS__) \
    cudaDeviceSynchronize();

/* return the number of threads in block */
template <hypre_int dim>
static __device__ __forceinline__
hypre_int hypre_cuda_get_num_threads()
{
   switch (dim)
   {
      case 1:
         return (blockDim.x);
      case 2:
         return (blockDim.x * blockDim.y);
      case 3:
         return (blockDim.x * blockDim.y * blockDim.z);
   }

   return -1;
}

/* return the flattened thread id in block */
template <hypre_int dim>
static __device__ __forceinline__
hypre_int hypre_cuda_get_thread_id()
{
   switch (dim)
   {
      case 1:
         return (threadIdx.x);
      case 2:
         return (threadIdx.y * blockDim.x + threadIdx.x);
      case 3:
         return (threadIdx.z * blockDim.x * blockDim.y + threadIdx.y * blockDim.x +
                 threadIdx.x);
   }

   return -1;
}

/* return the number of warps in block  */
template <hypre_int dim>
static __device__ __forceinline__
hypre_int hypre_cuda_get_num_warps()
{
   return hypre_cuda_get_num_threads<dim>() >> 5;
}

/* return the warp id in block */
template <hypre_int dim>
static __device__ __forceinline__
hypre_int hypre_cuda_get_warp_id()
{
   return hypre_cuda_get_thread_id<dim>() >> 5;
}

/* return the thread lane id in warp */
template <hypre_int dim>
static __device__ __forceinline__
hypre_int hypre_cuda_get_lane_id()
{
   return hypre_cuda_get_thread_id<dim>() & (HYPRE_WARP_SIZE-1);
}

/* return the num of blocks in grid */
template <hypre_int dim>
static __device__ __forceinline__
hypre_int hypre_cuda_get_num_blocks()
{
   switch (dim)
   {
      case 1:
         return (gridDim.x);
      case 2:
         return (gridDim.x * gridDim.y);
      case 3:
         return (gridDim.x * gridDim.y * gridDim.z);
   }

   return -1;
}

/* return the flattened block id in grid */
template <hypre_int dim>
static __device__ __forceinline__
hypre_int hypre_cuda_get_block_id()
{
   switch (dim)
   {
      case 1:
         return (blockIdx.x);
      case 2:
         return (blockIdx.y * gridDim.x + blockIdx.x);
      case 3:
         return (blockIdx.z * gridDim.x * gridDim.y + blockIdx.y * gridDim.x +
                 blockIdx.x);
   }

   return -1;
}

/* return the number of threads in grid */
template <hypre_int bdim, hypre_int gdim>
static __device__ __forceinline__
hypre_int hypre_cuda_get_grid_num_threads()
{
   return hypre_cuda_get_num_blocks<gdim>() * hypre_cuda_get_num_threads<bdim>();
}

/* return the flattened thread id in grid */
template <hypre_int bdim, hypre_int gdim>
static __device__ __forceinline__
hypre_int hypre_cuda_get_grid_thread_id()
{
   return hypre_cuda_get_block_id<gdim>() * hypre_cuda_get_num_threads<bdim>() +
          hypre_cuda_get_thread_id<bdim>();
}

/* return the number of warps in grid */
template <hypre_int bdim, hypre_int gdim>
static __device__ __forceinline__
hypre_int hypre_cuda_get_grid_num_warps()
{
   return hypre_cuda_get_num_blocks<gdim>() * hypre_cuda_get_num_warps<bdim>();
}

/* return the flattened warp id in grid */
template <hypre_int bdim, hypre_int gdim>
static __device__ __forceinline__
hypre_int hypre_cuda_get_grid_warp_id()
{
   return hypre_cuda_get_block_id<gdim>() * hypre_cuda_get_num_warps<bdim>() +
          hypre_cuda_get_warp_id<bdim>();
}

#if CUDART_VERSION < 9000

template <typename T>
static __device__ __forceinline__
T __shfl_sync(unsigned mask, T val, hypre_int src_line, hypre_int width=32)
{
   return __shfl(val, src_line, width);
}

template <typename T>
static __device__ __forceinline__
T __shfl_down_sync(unsigned mask, T val, unsigned delta, hypre_int width=32)
{
   return __shfl_down(val, delta, width);
}

template <typename T>
static __device__ __forceinline__
T __shfl_xor_sync(unsigned mask, T val, unsigned lanemask, hypre_int width=32)
{
   return __shfl_xor(val, lanemask, width);
}

template <typename T>
static __device__ __forceinline__
T __shfl_up_sync(unsigned mask, T val, unsigned delta, hypre_int width=32)
{
   return __shfl_up(val, delta, width);
}

static __device__ __forceinline__
void __syncwarp()
{
}

#endif

template <typename T>
static __device__ __forceinline__
T read_only_load( const T *ptr )
{
   return __ldg( ptr );
}

template <typename T>
static __device__ __forceinline__
T warp_prefix_sum(hypre_int lane_id, T in, T &all_sum)
{
#pragma unroll
   for (hypre_int d = 2; d <= 32; d <<= 1)
   {
      T t = __shfl_up_sync(HYPRE_WARP_FULL_MASK, in, d >> 1);
      if ( (lane_id & (d - 1)) == d - 1 )
      {
         in += t;
      }
   }

   all_sum = __shfl_sync(HYPRE_WARP_FULL_MASK, in, 31);

   if (lane_id == 31)
   {
      in = 0;
   }

#pragma unroll
   for (hypre_int d = 16; d > 0; d >>= 1)
   {
      T t = __shfl_xor_sync(HYPRE_WARP_FULL_MASK, in, d);

      if ( (lane_id & (d - 1)) == d - 1)
      {
         if ( (lane_id & (d << 1 - 1)) == (d << 1 - 1) )
         {
            in += t;
         }
         else
         {
            in = t;
         }
      }
   }
   return in;
}

template <typename T>
static __device__ __forceinline__
T warp_reduce_sum(T in)
{
#pragma unroll
  for (hypre_int d = 16; d > 0; d >>= 1)
  {
    in += __shfl_down_sync(HYPRE_WARP_FULL_MASK, in, d);
  }
  return in;
}

template <typename T>
static __device__ __forceinline__
T warp_allreduce_sum(T in)
{
#pragma unroll
  for (hypre_int d = 16; d > 0; d >>= 1)
  {
    in += __shfl_xor_sync(HYPRE_WARP_FULL_MASK, in, d);
  }
  return in;
}

template <typename T>
static __device__ __forceinline__
T warp_reduce_max(T in)
{
#pragma unroll
  for (hypre_int d = 16; d > 0; d >>= 1)
  {
    in = max(in, __shfl_down_sync(HYPRE_WARP_FULL_MASK, in, d));
  }
  return in;
}

template <typename T>
static __device__ __forceinline__
T warp_allreduce_max(T in)
{
#pragma unroll
  for (hypre_int d = 16; d > 0; d >>= 1)
  {
    in = max(in, __shfl_xor_sync(HYPRE_WARP_FULL_MASK, in, d));
  }
  return in;
}

template <typename T>
static __device__ __forceinline__
T warp_reduce_min(T in)
{
#pragma unroll
  for (hypre_int d = 16; d > 0; d >>= 1)
  {
    in = min(in, __shfl_down_sync(HYPRE_WARP_FULL_MASK, in, d));
  }
  return in;
}

template <typename T>
static __device__ __forceinline__
T warp_allreduce_min(T in)
{
#pragma unroll
  for (hypre_int d = 16; d > 0; d >>= 1)
  {
    in = min(in, __shfl_xor_sync(HYPRE_WARP_FULL_MASK, in, d));
  }
  return in;
}

static __device__ __forceinline__
hypre_int next_power_of_2(hypre_int n)
{
   if (n <= 0)
   {
      return 0;
   }

   /* if n is power of 2, return itself */
   if ( (n & (n - 1)) == 0 )
   {
      return n;
   }

   n |= (n >>  1);
   n |= (n >>  2);
   n |= (n >>  4);
   n |= (n >>  8);
   n |= (n >> 16);
   n ^= (n >>  1);
   n  = (n <<  1);

   return n;
}

#ifdef __cplusplus
}
#endif

#endif /* HYPRE_USING_CUDA */
#endif /* #ifndef HYPRE_CUDA_UTILS_H */

