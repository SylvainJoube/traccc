/** TRACCC library, part of the ACTS project (R&D line)
 *
 * (c) 2021 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#pragma once

#if ALGEBRA_PLUGINS_INCLUDE_ARRAY
#include "plugins/algebra/array_definitions.hpp"
#elif ALGEBRA_PLUGINS_INCLUDE_EIGEN
#include "plugins/algebra/eigen_definitions.hpp"
#elif ALGEBRA_PLUGINS_INCLUDE_SMATRIX
#include "plugins/algebra/smatrix_definitions.hpp"
#elif ALGEBRA_PLUGINS_INCLUDE_VC
#include "plugins/algebra/vc_definitions.hpp"
#elif ALGEBRA_PLUGINS_INCLUDE_VECMEM
#include "plugins/algebra/vecmem_definitions.hpp"
#endif

#include <Eigen/Core>

namespace traccc {

using geometry_id = uint64_t;
using event_id = uint64_t;
using channel_id = unsigned int;

using vector2 = __plugin::point2;
using point2 = __plugin::point2;
using variance2 = __plugin::point2;
using point3 = __plugin::point3;
using vector3 = __plugin::point3;
using variance3 = __plugin::point3;
using transform3 = __plugin::transform3;

// Fixme: Need to utilize algebra plugin for vector and matrix
template <unsigned int kSize>
using traccc_vector = Eigen::Matrix<scalar, kSize, 1>;

template <unsigned int kRows, unsigned int kCols>
using traccc_matrix = Eigen::Matrix<scalar, kRows, kCols>;

template <unsigned int kSize>
using traccc_sym_matrix = Eigen::Matrix<scalar, kSize, kSize>;

}  // namespace traccc
