/// @copyright 2022, Antoine Basset (CNES)
// This file is part of Litl <github.com/kabasset/Raster>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef _LITLRASTER_GRID_H
#define _LITLRASTER_GRID_H

#include "LitlRaster/Box.h"

#include <boost/operators.hpp>

namespace Litl {

/**
 * @brief An ND bounding box, defined by its front and back positions, both inclusive.
 * @details
 * Like `Position`, this class stores no pixel values, but coordinates.
 */
template <Index N = 2>
class Grid : boost::additive<Grid<N>, Grid<N>>, boost::additive<Grid<N>, Position<N>>, boost::additive<Grid<N>, Index> {

public:
  /**
   * @brief The dimension parameter.
   */
  static constexpr Index Dimension = N;

  /**
   * @brief A position iterator.
   * @see `Box::Iterator`
   */
  class Iterator;

  /// @{
  /// @group_construction

  /**
   * @brief Constructor.
   */
  Grid(Box<N> box, Position<N> step) : m_box(std::move(box)), m_step(std::move(step)) {
    Position<N> back(m_box.back());
    for (std::size_t i = 0; i < back.size(); ++i) {
      back[i] -= (m_box.length(i) - 1) % m_step[i];
    }
    m_box = Box<N>(m_box.front(), back);
  }

  /// @group_properties

  /**
   * @brief Get the bounding box.
   */
  const Box<N>& box() const {
    return m_box;
  }

  /**
   * @brief Get the front position.
   */
  const Position<N>& front() const {
    return m_box.front();
  }

  /**
   * @brief Get the back position.
   */
  const Position<N>& back() const {
    return m_box.back();
  }

  /**
   * @brief Get the step.
   */
  const Position<N>& step() const {
    return m_step;
  }

  /**
   * @brief Get the number of dimensions.
   */
  Index dimension() const {
    return m_box.dimension();
  }

  /**
   * @brief Get the number of grid nodes.
   */
  Index size() const {
    Index out = length<0>();
    for (Index i = 1; i < dimension(); ++i) {
      out *= length(i);
    }
    return out;
  }

  /**
   * @brief Get the box length along given axis.
   */
  template <Index I>
  Index length() const {
    return m_box.template length<I>() / m_step[I];
  }

  /**
   * @brief Get the box length along given axis.
   */
  Index length(Index i) const {
    return m_box.length(i) / m_step[i];
  }

  /// @group_elements

  /**
   * @brief Iterator to the front position.
   */
  Iterator begin() const {
    return Iterator::begin(*this);
  }

  /**
   * @brief End iterator.
   */
  Iterator end() const {
    return Iterator::end(*this);
  }

  /// @group_operations

  /**
   * @brief Check whether two boxes are equal.
   */
  bool operator==(const Grid<N>& other) const {
    return m_box == other.m_box && m_step == other.m_step;
  }

  /**
   * @brief Check whether two boxes are different.
   */
  bool operator!=(const Grid<N>& other) const {
    return m_box != other.m_box || m_step != other.m_step;
  }

  /// @group_modifiers

  /**
   * @brief Shift the box by a given vector.
   */
  Grid<N>& operator+=(const Position<N>& shift) {
    m_box += shift;
    return *this;
  }

  /**
   * @brief Shift the box by a given vector.
   */
  Grid<N>& operator-=(const Position<N>& shift) {
    m_box -= shift;
    return *this;
  }

  /**
    * @brief Add a scalar to each coordinate.
    */
  Grid<N>& operator+=(Index scalar) {
    m_box += scalar;
    return *this;
  }

  /**
   * @brief Subtract a scalar to each coordinate.
   */
  Grid<N>& operator-=(Index scalar) {
    m_box -= scalar;
    return *this;
  }

  /**
   * @brief Add 1 to each coordinate.
   */
  Grid<N>& operator++() {
    return *this += 1;
  }

  /**
   * @brief Subtract 1 to each coordinate.
   */
  Grid<N>& operator--() {
    return *this -= 1;
  }

  /**
   * @brief Copy.
   */
  Grid<N> operator+() {
    return *this;
  }

  /**
   * @brief Invert the sign of each coordinate.
   */
  Grid<N> operator-() {
    return {-m_box, -m_step};
  }

  /// @}

private:
  /**
   * @brief The bounding box.
   */
  Box<N> m_box;

  /**
   * @brief The step along each axis.
   */
  Position<N> m_step;
};

} // namespace Litl

#include "LitlRaster/GridIterator.h"

#endif
