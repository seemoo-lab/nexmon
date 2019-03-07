/* Prototypes of memory model helper functions.
   Copyright (C) 2015-2016 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_MEMMODEL_H
#define GCC_MEMMODEL_H

/* Return the memory model from a host integer.  */
static inline enum memmodel
memmodel_from_int (unsigned HOST_WIDE_INT val)
{
  return (enum memmodel) (val & MEMMODEL_MASK);
}

/* Return the base memory model from a host integer.  */
static inline enum memmodel
memmodel_base (unsigned HOST_WIDE_INT val)
{
  return (enum memmodel) (val & MEMMODEL_BASE_MASK);
}

/* Return TRUE if the memory model is RELAXED.  */
static inline bool
is_mm_relaxed (enum memmodel model)
{
  return (model & MEMMODEL_BASE_MASK) == MEMMODEL_RELAXED;
}

/* Return TRUE if the memory model is CONSUME.  */
static inline bool
is_mm_consume (enum memmodel model)
{
  return (model & MEMMODEL_BASE_MASK) == MEMMODEL_CONSUME;
}

/* Return TRUE if the memory model is ACQUIRE.  */
static inline bool
is_mm_acquire (enum memmodel model)
{
  return (model & MEMMODEL_BASE_MASK) == MEMMODEL_ACQUIRE;
}

/* Return TRUE if the memory model is RELEASE.  */
static inline bool
is_mm_release (enum memmodel model)
{
  return (model & MEMMODEL_BASE_MASK) == MEMMODEL_RELEASE;
}

/* Return TRUE if the memory model is ACQ_REL.  */
static inline bool
is_mm_acq_rel (enum memmodel model)
{
  return (model & MEMMODEL_BASE_MASK) == MEMMODEL_ACQ_REL;
}

/* Return TRUE if the memory model is SEQ_CST.  */
static inline bool
is_mm_seq_cst (enum memmodel model)
{
  return (model & MEMMODEL_BASE_MASK) == MEMMODEL_SEQ_CST;
}

/* Return TRUE if the memory model is a SYNC variant.  */
static inline bool
is_mm_sync (enum memmodel model)
{
  return (model & MEMMODEL_SYNC);
}

#endif  /* GCC_MEMMODEL_H  */
