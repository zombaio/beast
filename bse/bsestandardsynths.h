/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_STANDARD_SYNTHS_H__
#define __BSE_STANDARD_SYNTHS_H__

#include        <bse/bseproject.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- typedefs & structures --- */
typedef struct
{
  BseSNet	*snet;
  BseSource	*wave;
} BseStandardSynth;

BseStandardSynth*	bse_project_standard_piano	(BseProject	*project);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_STANDARD_SYNTHS_H__ */
