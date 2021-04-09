// -*- C++ -*-

// Copyright (C) 2005-2019 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the terms
// of the GNU General Public License as published by the Free Software
// Foundation; either version 3, or (at your option) any later
// version.

// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

// Copyright (C) 2004 Ami Tavory and Vladimir Dreizin, IBM-HRL.

// Permission to use, copy, modify, sell, and distribute this software
// is hereby granted without fee, provided that the above copyright
// notice appears in all copies, and that both that copyright notice
// and this permission notice appear in supporting documentation. None
// of the above authors, nor IBM Haifa Research Laboratories, make any
// representation about the suitability of this software for any
// purpose. It is provided "as is" without express or implied
// warranty.

/**
 * @file sample_ranged_probe_fn.hpp
 * Contains a ranged probe policy.
 */

#ifndef PB_DS_SAMPLE_RANGED_PROBE_FN_HPP
#define PB_DS_SAMPLE_RANGED_PROBE_FN_HPP

namespace __gnu_pbds
{
  /// A sample ranged-probe functor.
  class sample_ranged_probe_fn
  {
  public:
    typedef std::size_t 	size_type;

    // Default constructor.
    sample_ranged_probe_fn();

    // Copy constructor.
    sample_ranged_probe_fn(const sample_ranged_probe_fn&);

    // Swaps content.
    inline void
    swap(sample_ranged_probe_fn&);

  protected:

    // Notifies the policy object that the container's __size has
    // changed to size.
    void
    notify_resized(size_type);

    // Transforms the const key reference r_key into the i-th position
    // within the table. This method is called for each collision within
    // the probe sequence.
    inline size_type
    operator()(key_const_reference, std::size_t, size_type) const;

  };
}
#endif // #ifndef PB_DS_SAMPLE_RANGED_PROBE_FN_HPP
