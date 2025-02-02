// @copyright 2022-2024, Antoine Basset (CNES)
// This file is part of Linx <github.com/kabasset/Linx>
// SPDX-License-Identifier: Apache-2.0

#ifndef _LINXTRANSFORMS_FILTERS_H
#define _LINXTRANSFORMS_FILTERS_H

#include "Linx/Base/TypeUtils.h"
#include "Linx/Data/Mask.h" // for sparse_*
#include "Linx/Transforms/FilterAgg.h"
#include "Linx/Transforms/FilterSeq.h"
#include "Linx/Transforms/SimpleFilter.h"
#include "Linx/Transforms/mixins/Kernel.h"

namespace Linx {

/**
 * @ingroup filtering
 * @brief Correlation kernel.
 */
template <typename T, typename TWindow>
class Correlation : public KernelMixin<T, TWindow, Correlation<T, TWindow>> {
public:

  using KernelMixin<T, TWindow, Correlation>::KernelMixin;

  template <typename TIn>
  inline T operator()(const TIn& neighbors) const
  {
    return std::inner_product(this->m_values.begin(), this->m_values.end(), neighbors.begin(), T {});
  }

  void init_impl() // FIXME private
  {
    if constexpr (is_complex<T>()) {
      for (auto& e : this->m_values) {
        e = std::conj(e);
      }
    }
  }
};

/**
 * @ingroup filtering
 * @brief Convolution kernel.
 */
template <typename T, typename TWindow>
class Convolution : public KernelMixin<T, TWindow, Convolution<T, TWindow>> {
public:

  using KernelMixin<T, TWindow, Convolution>::KernelMixin;

  template <typename TIn>
  inline T operator()(const TIn& neighbors) const
  {
    return std::inner_product(this->m_values.rbegin(), this->m_values.rend(), neighbors.begin(), T {});
  }

  void init_impl() {} // FIXME private
};

/**
 * @ingroup filtering
 * @brief Mean filtering kernel.
 */
template <typename T, typename TWindow>
struct MeanFilter : public StructuringElementMixin<T, TWindow, MeanFilter<T, TWindow>> {
  using StructuringElementMixin<T, TWindow, MeanFilter>::StructuringElementMixin;
  template <typename TIn>
  T operator()(const TIn& neighbors) const
  {
    return std::accumulate(neighbors.begin(), neighbors.end(), T()) / neighbors.size();
  }
};

/**
 * @ingroup filtering
 * @brief Median filtering kernel.
 */
template <typename T, typename TWindow>
struct MedianFilter :
    public StructuringElementMixin<T, TWindow, MedianFilter<T, TWindow>> { // FIXME even and odd specializations
  using StructuringElementMixin<T, TWindow, MedianFilter>::StructuringElementMixin;
  template <typename TIn>
  T operator()(const TIn& neighbors) const
  {
    std::vector<T> v(neighbors.begin(), neighbors.end());
    const auto size = v.size();
    auto b = v.data();
    auto e = b + size;
    auto n = b + size / 2;
    std::nth_element(b, n, e);
    if (size % 2 == 1) {
      return *n;
    }
    std::nth_element(b, n + 1, e);
    return (*n + *(n + 1)) * .5;
  }
};

/**
 * @ingroup filtering
 * @brief Minimum filtering kernel.
 */
template <typename T, typename TWindow>
struct MinimumFilter : public StructuringElementMixin<T, TWindow, MinimumFilter<T, TWindow>> {
  using StructuringElementMixin<T, TWindow, MinimumFilter>::StructuringElementMixin;
  template <typename TIn>
  inline T operator()(const TIn& neighbors) const
  {
    return *std::min_element(neighbors.begin(), neighbors.end());
  }
};

/**
 * @ingroup filtering
 * @brief Maximum filtering kernel.
 */
template <typename T, typename TWindow>
struct MaximumFilter : public StructuringElementMixin<T, TWindow, MaximumFilter<T, TWindow>> {
  using StructuringElementMixin<T, TWindow, MaximumFilter>::StructuringElementMixin;
  template <typename TIn>
  inline T operator()(const TIn& neighbors) const
  {
    return *std::max_element(neighbors.begin(), neighbors.end());
  }
};

/**
 * @ingroup filtering
 * @brief Binary erosion kernel.
 * 
 * This is an optimization of the minimum filter for Booleans.
 */
template <typename T, typename TWindow>
struct BinaryErosion : public StructuringElementMixin<T, TWindow, BinaryErosion<T, TWindow>> {
  /**
   * @brief Optimization tag: erosion requires no neighborhood around false pixels.
   */
  struct ShiftsWindow;

  using StructuringElementMixin<T, TWindow, BinaryErosion>::StructuringElementMixin;

  template <typename TIn>
  inline T operator()(const TIn& neighbors) const
  {
    return std::all_of(neighbors.begin(), neighbors.end(), [](auto e) {
      return bool(e);
    });
  }

  template <typename TIn, typename TPatch, typename TPos>
  inline T operator()(const TIn& in, TPatch& patch, const TPos& pos) const
  {
    if (not in[pos]) {
      return false;
    }
    patch >>= pos;
    auto out = operator()(patch);
    patch <<= pos;
    return out;
  }
};

/**
 * @ingroup filtering
 * @brief Binary dilation kernel.
 * 
 * This is an optimization of the maximum filter for Booleans.
 */
template <typename T, typename TWindow>
struct BinaryDilation : public StructuringElementMixin<T, TWindow, BinaryDilation<T, TWindow>> {
  /**
   * @brief Optimization tag: dilation requires no neighborhood around true pixels.
   */
  struct ShiftsWindow;

  using StructuringElementMixin<T, TWindow, BinaryDilation>::StructuringElementMixin;

  template <typename TIn>
  inline T operator()(const TIn& neighbors) const
  {
    return std::any_of(neighbors.begin(), neighbors.end(), [](auto e) {
      return bool(e);
    });
  }

  template <typename TIn, typename TPatch, typename TPos>
  inline T operator()(const TIn& in, TPatch& patch, const TPos& pos) const
  {
    if (in[pos]) {
      return true;
    }
    patch >>= pos;
    auto out = operator()(patch);
    patch <<= pos;
    return out;
  }
};

/**
 * @ingroup filtering
 * @brief Make a convolution kernel from values and a window.
 */
template <typename T, Index N = 2>
auto convolution(const T* values, Box<N> window)
{
  const T* end = values + window.size();
  return SimpleFilter<Convolution<T, Box<N>>>(LINX_MOVE(window), std::vector<T>(values, end));
}

/**
 * @ingroup filtering
 * @brief Make a convolution kernel from a raster and origin position.
 */
template <typename T, Index N, typename THolder>
auto convolution(const Raster<T, N, THolder>& values, Position<N> origin)
{
  return convolution(values.data(), values.domain() - origin);
}

/**
 * @ingroup filtering
 * @brief Make a convolution kernel from a raster, with centered origin.
 * 
 * In case of even lengths, origin position is rounded down.
 */
template <typename T, Index N, typename THolder>
auto convolution(const Raster<T, N, THolder>& values)
{
  return convolution(values.data(), values.domain() - (values.shape() - 1) / 2); // FIXME implement center(Box)
}

/**
 * @ingroup filtering
 * @brief Make a sparse convolution kernel from a raster, with centered origin.
 */
template <typename T, Index N, typename THolder>
auto sparse_convolution(const Raster<T, N, THolder>& values) // FIXME overloads and correlation
{
  auto reversed = values;
  std::reverse(reversed.begin(), reversed.end());
  Mask<N> mask(values.domain() - (values.shape() - 1) / 2, reversed); // FIXME offset error?
  return SimpleFilter<Convolution<T, Mask<N>>>(mask, reversed(mask));
}

/**
 * @ingroup filtering
 * @brief Make a correlation kernel from values and a window.
 */
template <typename T, Index N = 2>
auto correlation(const T* values, Box<N> window)
{
  const T* end = values + window.size();
  return SimpleFilter<Correlation<T, Box<N>>>(LINX_MOVE(window), std::vector<T>(values, end));
}

/**
 * @ingroup filtering
 * @brief Make a correlation kernel from a raster and origin position.
 */
template <typename T, Index N, typename THolder>
auto correlation(const Raster<T, N, THolder>& values, Position<N> origin)
{
  return correlation(values.data(), values.domain() - origin);
}

/**
 * @ingroup filtering
 * @brief Make a correlation kernel from a raster, with centered origin.
 * 
 * In case of even lengths, origin position is rounded down.
 */
template <typename T, Index N, typename THolder>
auto correlation(const Raster<T, N, THolder>& values)
{
  return correlation(values.data(), values.domain() - (values.shape() - 1) / 2);
}

/**
 * @ingroup filtering
 * @brief Create a filter made of identical 1D correlation kernels along given axes.
 * 
 * Axes need not be different, e.g. to define some iterative kernel.
 */
template <typename T, Index I0, Index... Is>
auto correlation_along(const std::vector<T>& values)
{
  if constexpr (sizeof...(Is) == 0) {
    const auto radius = values.size() / 2;
    auto front = Position<I0 + 1>::zero();
    front[I0] = -radius; // FIXME +1?
    auto back = Position<I0 + 1>::zero();
    back[I0] = values.size() - radius - 1;
    return correlation<T, I0 + 1>(values.data(), {front, back}); // FIXME line-based window
  } else {
    return correlation_along<T, I0>(values) * correlation_along<T, Is...>(values);
  }
}

/**
 * @ingroup filtering
 * @brief Create a filter made of identical 1D convolution kernels along given axes.
 * 
 * Axis need not be different, e.g. to define some iterative kernel.
 */
template <typename T, Index I0, Index... Is>
auto convolution_along(const std::vector<T>& values)
{
  if constexpr (sizeof...(Is) == 0) {
    const auto radius = values.size() / 2;
    auto front = Position<I0 + 1>::zero();
    front[I0] = -radius; // FIXME +1?
    auto back = Position<I0 + 1>::zero();
    back[I0] = values.size() - radius - 1;
    return convolution<T, I0 + 1>(values.data(), {front, back}); // FIXME line-based window
  } else {
    return convolution_along<T, I0>(values) * convolution_along<T, Is...>(values);
  }
}

/**
 * @ingroup filtering
 * @brief Make a Prewitt gradient filter along given axes.
 * @tparam T The filter output type
 * @tparam IDerivation The derivation axis
 * @tparam IAveraging The averaging axes
 * @param sign The differentiation sign (-1 or 1)
 * 
 * The convolution kernel along the `IAveraging` axes is `{1, 1, 1}` and that along `IDerivation` is `{sign, 0, -sign}`.
 * For differenciation in the increasing-index direction, keep `sign = 1`;
 * for the opposite direction, set `sign = -1`.
 * 
 * For example, to compute the derivative along axis 1 backward, while averaging along axes 0 and 2, do:
 * \code
 * auto kernel = prewitt_gradient<int, 1, 0, 2>(-1);
 * auto dy = kernel * raster;
 * \endcode
 * 
 * @see `sobel_gradient()`
 * @see `scharr_gradient()`
 */
template <typename T, Index IDerivation, Index... IAveraging>
auto prewitt_gradient(T sign = 1)
{
  const auto derivation = convolution_along<T, IDerivation>({sign, 0, -sign});
  const auto averaging = convolution_along<T, IAveraging...>({1, 1, 1});
  return derivation * averaging;
}

/**
 * @ingroup filtering
 * @brief Make a Sobel gradient filter along given axes.
 * 
 * The convolution kernel along the `IAveraging` axes is `{1, 2, 1}` and that along `IDerivation` is `{sign, 0, -sign}`.
 * 
 * @see `prewitt_gradient()`
 * @see `scharr_gradient()`
 */
template <typename T, Index IDerivation, Index... IAveraging>
auto sobel_gradient(T sign = 1)
{
  const auto derivation = convolution_along<T, IDerivation>({sign, 0, -sign});
  const auto averaging = convolution_along<T, IAveraging...>({1, 2, 1});
  return derivation * averaging;
}

/**
 * @ingroup filtering
 * @brief Make a Scharr gradient filter along given axes.
 * 
 * The kernel along the `IAveraging` axes is `{3, 10, 3}` and that along `IDerivation` is `{sign, 0, -sign}`.
 * 
 * @see `prewitt_gradient()`
 * @see `sobel_gradient()`
 */
template <typename T, Index IDerivation, Index... IAveraging>
auto scharr_gradient(T sign = 1)
{
  const auto derivation = convolution_along<T, IDerivation>({sign, 0, -sign});
  const auto averaging = convolution_along<T, IAveraging...>({3, 10, 3});
  return derivation * averaging;
}

/**
 * @ingroup filtering
 * @brief Make a Laplace operator along given axes.
 * 
 * The convolution kernel is built as a sum of 1D kernels `{sign, -2 * sign, sign}`.
 */
template <typename T, Index... Is>
auto laplace_operator(T sign = 1)
{
  return FilterAgg(std::plus<T>(), convolution_along<T, Is>({sign, sign * -2, sign})...);
}

/**
 * @ingroup filtering
 * @brief Make a mean filter with a given structuring element.
 */
template <typename T, typename TWindow>
auto mean_filter(TWindow&& window)
{
  return SimpleFilter<MeanFilter<T, TWindow>>(MeanFilter<T, TWindow>(LINX_FORWARD(window))); // FIXME separable
}

/**
 * @ingroup filtering
 * @brief Make a median filter with a given structuring element.
 */
template <typename T, typename TWindow>
auto median_filter(TWindow&& window)
{
  return SimpleFilter<MedianFilter<T, TWindow>>(MedianFilter<T, TWindow>(LINX_FORWARD(window)));
}

/**
 * @ingroup filtering
 * @brief Make a minimun filter with a given structuring element.
 */
template <typename T, typename TWindow>
auto minimum_filter(TWindow&& window)
{
  return SimpleFilter<MinimumFilter<T, TWindow>>(MinimumFilter<T, TWindow>(LINX_FORWARD(window)));
}

/**
 * @ingroup filtering
 * @brief Make a maximum filter with a given structuring element.
 */
template <typename T, typename TWindow>
auto maximum_filter(TWindow&& window)
{
  return SimpleFilter<MaximumFilter<T, TWindow>>(MaximumFilter<T, TWindow>(LINX_FORWARD(window)));
}

/**
 * @ingroup filtering
 * @brief Make a erosion filter with a given structuring element.
 */
template <typename T, typename TWindow>
auto erosion(TWindow&& window)
{
  return SimpleFilter<BinaryErosion<T, TWindow>>(BinaryErosion<T, TWindow>(LINX_FORWARD(window)));
}

/**
 * @ingroup filtering
 * @brief Make a dilation filter with a given structuring element.
 */
template <typename T, typename TWindow>
auto dilation(TWindow&& window)
{
  return SimpleFilter<BinaryDilation<T, TWindow>>(BinaryDilation<T, TWindow>(LINX_FORWARD(window)));
}

} // namespace Linx

#endif
