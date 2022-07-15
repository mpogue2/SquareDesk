/** @addtogroup other_math
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  KFR is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with KFR.

  If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
  Buying a commercial license is mandatory as soon as you develop commercial activities without
  disclosing the source code of your own applications.
  See https://www.kfrlib.com for details.
 */
#pragma once

#include "impl/modzerobessel.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION T1 modzerobessel(const T1& x)
{
    return intrinsics::modzerobessel(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION internal::expression_function<fn::modzerobessel, E1> modzerobessel(E1&& x)
{
    return { fn::modzerobessel(), std::forward<E1>(x) };
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
