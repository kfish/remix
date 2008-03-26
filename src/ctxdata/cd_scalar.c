/*
 * CtxData -- Context oriented data types
 *
 * Copyright (C) 2001 Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO), Australia.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * CDScalar: Polymorphic scalar values
 *
 * Conrad Parker <conrad@metadecks.org>, August 2001
 */


#include "ctxdata.h"

CDScalar
cd_scalar_invalid (void * ctx, CDScalarType type)
{
  return CD_POINTER(NULL);
}

int
cd_scalar_eq (void * ctx, CDScalarType type, CDScalar d1, CDScalar d2)
{
  switch (type) {
  case CD_TYPE_CHAR: return (d1.s_char == d2.s_char); break;
  case CD_TYPE_UCHAR: return (d1.s_uchar == d2.s_uchar); break;
  case CD_TYPE_INT: return (d1.s_int == d2.s_int); break;
  case CD_TYPE_UINT: return (d1.s_uint == d2.s_uint); break;
  case CD_TYPE_LONG: return (d1.s_long == d2.s_long); break;
  case CD_TYPE_ULONG: return (d1.s_ulong == d2.s_ulong); break;
  case CD_TYPE_FLOAT: return (d1.s_float == d2.s_float); break;
  case CD_TYPE_DOUBLE: return (d1.s_double == d2.s_double); break;
  case CD_TYPE_STRING: return (d1.s_string == d2.s_string); break;
  case CD_TYPE_POINTER: return (d1.s_pointer == d2.s_pointer); break;
  default: return 0;
  }
}
