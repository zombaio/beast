/* BSW - Bedevilled Sound Engine Wrapper
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
#include "bswcommon.h"

#include "bswprivate.h"
#include "gslcommon.h"


/* --- macros --- */
#define BSW_IS_VITER_INT(vi)	((vi) && (vi)->type == BSW_TYPE_VITER_INT)
#define BSW_IS_VITER_STRING(vi)	((vi) && (vi)->type == BSW_TYPE_VITER_STRING)
#define BSW_IS_VITER_BOXED(vi)	((vi) && (vi)->type == BSW_TYPE_VITER_BOXED)
#define BSW_IS_VITER_PROXY(vi)	((vi) && (vi)->type == BSW_TYPE_VITER_PROXY)

#define	PREALLOC_BLOCK_SIZE	(8)


/* --- structures --- */
struct _BswVIter
{
  GType type;
  GType	boxed_type;
  guint n_items;
  guint pos;
  union {
    guint    v_int;
    gchar   *v_string;
    gpointer v_boxed;
    BswProxy v_proxy;
  } *items;
  guint prealloc;
};


/* --- BSW proxy --- */
GType
bsw_proxy_get_type (void)
{
  static GType type = 0;
  if (!type)
    type = g_pointer_type_register_static ("BswProxy");
  return type;
}

void
bsw_value_set_proxy (GValue  *value,
		     BswProxy proxy)
{
  g_return_if_fail (BSW_VALUE_HOLDS_PROXY (value));

  value->data[0].v_pointer = (gpointer) proxy;
}

BswProxy
bsw_value_get_proxy (const GValue *value)
{
  g_return_val_if_fail (BSW_VALUE_HOLDS_PROXY (value), 0);

  return (BswProxy) value->data[0].v_pointer;
}

GParamSpec*
bsw_param_spec_proxy (const gchar *name,
		      const gchar *nick,
		      const gchar *blurb,
		      GParamFlags  flags)
{
  GParamSpec *pspec = g_param_spec_pointer (name, nick, blurb, flags);

  pspec->value_type = BSW_TYPE_PROXY;

  return pspec;
}


/* --- BSW value iterators --- */
static gpointer
iter_copy (gpointer boxed)
{
  return boxed ? bsw_viter_copy (boxed) : NULL;
}

static void
iter_free (gpointer boxed)
{
  if (boxed)
    bsw_viter_free (boxed);
}

GType
bsw_viter_int_get_type (void)
{
  static GType type = 0;
  if (!type)
    type = g_boxed_type_register_static ("BswVIterInt", iter_copy, iter_free);
  return type;
}

GType
bsw_viter_string_get_type (void)
{
  static GType type = 0;
  if (!type)
    type = g_boxed_type_register_static ("BswVIterString", iter_copy, iter_free);
  return type;
}

GType
bsw_viter_boxed_get_type (void)
{
  static GType type = 0;
  if (!type)
    type = g_boxed_type_register_static ("BswVIterBoxed", iter_copy, iter_free);
  return type;
}

GType
bsw_viter_proxy_get_type (void)
{
  static GType type = 0;
  if (!type)
    type = g_boxed_type_register_static ("BswVIterProxy", iter_copy, iter_free);
  return type;
}

GType
bsw_viter_type (BswVIter *iter)
{
  g_return_val_if_fail (BSW_IS_VITER (iter), 0);

  return iter->type;
}

BswVIter*
bsw_viter_create (GType type,
		  guint prealloc)
{
  BswVIter *iter;

  g_return_val_if_fail (type != G_TYPE_BOXED && G_TYPE_IS_BOXED (type), NULL);

  iter = gsl_new_struct (BswVIter, 1);
  if (type == BSW_TYPE_VITER_INT ||
      type == BSW_TYPE_VITER_STRING ||
      type == BSW_TYPE_VITER_PROXY)
   {
     iter->type = type;
     iter->boxed_type = 0;
   }
  else /* (G_TYPE_IS_BOXED (type)) */
    {
      iter->type = BSW_TYPE_VITER_BOXED;
      iter->boxed_type = type;
    }

  g_return_val_if_fail (BSW_IS_VITER (iter), NULL);

  iter->n_items = 0;
  iter->prealloc = prealloc;
  iter->pos = 0;

  iter->items = g_malloc0 (sizeof (iter->items[0]) * iter->prealloc);

  return iter;
}

BswVIter*
bsw_viter_copy (BswVIter *iter)
{
  BswVIter *diter;
  guint i;

  g_return_val_if_fail (BSW_IS_VITER (iter), NULL);

  if (BSW_IS_VITER_BOXED (iter))
    diter = bsw_viter_create (iter->boxed_type, iter->n_items);
  else
    diter = bsw_viter_create (iter->type, iter->n_items);
  if (BSW_IS_VITER_STRING (diter))
    for (i = 0; i < iter->n_items; i++)
      diter->items[i].v_string = g_strdup (iter->items[i].v_string);
  else if (BSW_IS_VITER_BOXED (diter))
    for (i = 0; i < iter->n_items; i++)
      diter->items[i].v_boxed = iter->items[i].v_boxed ? g_boxed_copy (diter->boxed_type, iter->items[i].v_boxed) : NULL;
  else
    for (i = 0; i < iter->n_items; i++)
      memcpy (&diter->items[i], &iter->items[i], sizeof (iter->items[i]));
  diter->n_items = iter->n_items;

  return diter;
}

void
bsw_viter_free (BswVIter *iter)
{
  guint i;

  g_return_if_fail (BSW_IS_VITER (iter));

  if (BSW_IS_VITER_STRING (iter))
    for (i = 0; i < iter->n_items; i++)
      g_free (iter->items[i].v_string);
  else if (BSW_IS_VITER_BOXED (iter))
    for (i = 0; i < iter->n_items; i++)
      if (iter->items[i].v_boxed)
	g_boxed_free (iter->boxed_type, iter->items[i].v_boxed);
  g_free (iter->items);
  iter->type = 0;
  gsl_delete_struct (BswVIter, iter);
}

void
bsw_viter_rewind (BswVIter *iter)
{
  g_return_if_fail (BSW_IS_VITER (iter));

  iter->pos = 0;
}

guint
bsw_viter_n_left (BswVIter *iter)
{
  g_return_val_if_fail (BSW_IS_VITER (iter), 0);

  return iter->n_items - iter->pos;
}

void
bsw_viter_next (BswVIter *iter)
{
  g_return_if_fail (BSW_IS_VITER (iter));
  g_return_if_fail (iter->pos < iter->n_items);

  iter->pos++;
}

void
bsw_viter_prev (BswVIter *iter)
{
  g_return_if_fail (BSW_IS_VITER (iter));
  g_return_if_fail (iter->pos > 0);

  iter->pos--;
}

void
bsw_viter_jump (BswVIter *iter,
		guint     nth)
{
  g_return_if_fail (BSW_IS_VITER (iter));
  g_return_if_fail (nth <= iter->n_items);

  iter->pos = nth;
}

gint
bsw_viter_get_int (BswVIterInt *iter)
{
  g_return_val_if_fail (BSW_IS_VITER_INT (iter), 0);
  g_return_val_if_fail (iter->pos < iter->n_items, 0);

  return iter->items[iter->pos].v_int;
}

gchar*
bsw_viter_get_string (BswVIterString *iter)
{
  g_return_val_if_fail (BSW_IS_VITER_STRING (iter), NULL);
  g_return_val_if_fail (iter->pos < iter->n_items, NULL);

  return iter->items[iter->pos].v_string;
}

gpointer
bsw_viter_get_boxed (BswVIterBoxed *iter)
{
  g_return_val_if_fail (BSW_IS_VITER_BOXED (iter), NULL);
  g_return_val_if_fail (iter->pos < iter->n_items, NULL);

  return iter->items[iter->pos].v_boxed;
}

BswProxy
bsw_viter_get_proxy (BswVIterProxy *iter)
{
  g_return_val_if_fail (BSW_IS_VITER_PROXY (iter), 0);
  g_return_val_if_fail (iter->pos < iter->n_items, 0);

  return iter->items[iter->pos].v_proxy;
}

static inline guint
bsw_viter_grow1 (BswVIter *iter)
{
  guint i;

  i = iter->n_items++;
  if (iter->n_items > iter->prealloc)
    {
      iter->prealloc += PREALLOC_BLOCK_SIZE;
      iter->items = g_realloc (iter->items, sizeof (iter->items[0]) * iter->prealloc);
      memset (iter->items + i, 0, sizeof (iter->items[0]) * (iter->prealloc - i));
    }
  return i;
}

void
bsw_viter_append_int (BswVIterInt *iter,
		      gint         v_int)
{
  guint i;

  g_return_if_fail (BSW_IS_VITER_INT (iter));

  i = bsw_viter_grow1 (iter);
  iter->items[i].v_int = v_int;
}

void
bsw_viter_append_string_take_ownership (BswVIterString *iter,
					const gchar    *string)
{
  guint i;

  g_return_if_fail (BSW_IS_VITER_STRING (iter));

  i = bsw_viter_grow1 (iter);
  iter->items[i].v_string = (gchar*) string;
}

void
bsw_viter_append_boxed_take_ownership (BswVIterBoxed *iter,
				       gpointer       boxed)
{
  guint i;

  g_return_if_fail (BSW_IS_VITER_BOXED (iter));

  i = bsw_viter_grow1 (iter);
  iter->items[i].v_boxed = boxed;
}

void
bsw_viter_append_boxed (BswVIterBoxed *iter,
			gconstpointer  boxed)
{
  guint i;

  g_return_if_fail (BSW_IS_VITER_BOXED (iter));

  i = bsw_viter_grow1 (iter);
  if (boxed)
    iter->items[i].v_boxed = g_boxed_copy (iter->boxed_type, boxed);
  else
    iter->items[i].v_boxed = NULL;
}

void
bsw_viter_append_proxy (BswVIterProxy *iter,
			BswProxy       proxy)
{
  guint i;

  g_return_if_fail (BSW_IS_VITER_PROXY (iter));

  i = bsw_viter_grow1 (iter);
  iter->items[i].v_proxy = proxy;
}


/* --- BSW Part Note --- */
static gpointer
part_note_copy (gpointer boxed)
{
  BswPartNote *pnote = NULL;

  if (boxed)
    {
      pnote = gsl_new_struct (BswPartNote, 1);

      memcpy (pnote, boxed, sizeof (*pnote));
    }
  return pnote;
}

void
bsw_part_note_free (BswPartNote *pnote)
{
  if (pnote)
    gsl_delete_struct (BswPartNote, pnote);
}

GType
bsw_part_note_get_type (void)
{
  static GType type = 0;
  if (!type)
    type = g_boxed_type_register_static ("BswPartNote", part_note_copy, (GBoxedFreeFunc) bsw_part_note_free);
  return type;
}

BswPartNote*
bsw_part_note (guint  tick,
	       guint  duration,
	       gfloat freq,
	       gfloat velocity)
{
  BswPartNote *pnote = gsl_new_struct (BswPartNote, 1);

  pnote->tick = tick;
  pnote->freq = freq;
  pnote->duration = duration;
  pnote->velocity = velocity;

  return pnote;
}

static gpointer
note_description_copy (gpointer boxed)
{
  BswNoteDescription *info = NULL;

  if (boxed)
    {
      info = gsl_new_struct (BswNoteDescription, 1);

      memcpy (info, boxed, sizeof (*info));
      info->name = g_strdup (info->name);
    }
  return info;
}

void
bsw_note_description_free (BswNoteDescription *info)
{
  if (info)
    {
      g_free (info->name);
      gsl_delete_struct (BswNoteDescription, info);
    }
}

GType
bsw_note_description_get_type (void)
{
  static GType type = 0;
  if (!type)
    type = g_boxed_type_register_static ("BswNoteDescription", note_description_copy, (GBoxedFreeFunc) bsw_note_description_free);
  return type;
}

BswNoteDescription*
bsw_note_description (guint note,
		      gint  fine_tune)
{
  BswNoteDescription *info = gsl_new_struct (BswNoteDescription, 1);

  if (note >= BSE_MIN_NOTE && note <= BSE_MAX_NOTE)
    {
      info->note = note;
      bse_note_examine (info->note,
			&info->octave,
			&info->half_tone,
			&info->upshift,
			&info->letter);
      info->fine_tune = CLAMP (fine_tune, BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE);
      info->freq = bse_note_to_tuned_freq (info->note, info->fine_tune);
      info->name = bse_note_to_string (info->note);
      info->max_fine_tune = BSE_MAX_FINE_TUNE;
      info->kammer_note = BSE_KAMMER_NOTE;
    }
  else
    {
      memset (info, 0, sizeof (info));
      info->note = BSE_NOTE_VOID;
      info->name = NULL;
      info->max_fine_tune = BSE_MAX_FINE_TUNE;
      info->kammer_note = BSE_KAMMER_NOTE;
    }
  return info;
}


/* --- BSW value block --- */
static gpointer
vblock_copy (gpointer boxed)
{
  return boxed ? bsw_value_block_ref (boxed) : NULL;
}

static void
vblock_free (gpointer boxed)
{
  if (boxed)
    bsw_value_block_unref (boxed);
}

GType
bsw_value_block_get_type (void)
{
  static GType type = 0;
  if (!type)
    type = g_boxed_type_register_static ("BswValueBlock", vblock_copy, vblock_free);
  return type;
}

BswValueBlock*
bsw_value_block_new (guint n_values)
{
  BswValueBlock *vblock = g_malloc0 (sizeof (BswValueBlock) + sizeof (vblock->values[0]) * (MAX (n_values, 1) - 1));

  vblock->ref_count = 1;
  vblock->n_values = n_values;

  return vblock;
}

BswValueBlock*
bsw_value_block_ref (BswValueBlock *vblock)
{
  g_return_val_if_fail (vblock != NULL, NULL);
  g_return_val_if_fail (vblock->ref_count > 0, NULL);

  vblock->ref_count++;

  return vblock;
}

void
bsw_value_block_unref (BswValueBlock *vblock)
{
  g_return_if_fail (vblock != NULL);
  g_return_if_fail (vblock->ref_count > 0);

  vblock->ref_count--;
  if (!vblock->ref_count)
    g_free (vblock);
}


/* --- BSW icons --- */
#define STATIC_REF_COUNT (1 << 31)

BswIcon*
bsw_icon_ref_static (BswIcon *icon)
{
  g_return_val_if_fail (icon != NULL, NULL);
  g_return_val_if_fail (icon->ref_count > 0, NULL);

  icon->ref_count |= STATIC_REF_COUNT;

  return icon;
}

BswIcon*
bsw_icon_ref (BswIcon *icon)
{
  g_return_val_if_fail (icon != NULL, NULL);
  g_return_val_if_fail (icon->ref_count > 0, NULL);

  if (!(icon->ref_count & STATIC_REF_COUNT))
    icon->ref_count += 1;

  return icon;
}

void
bsw_icon_unref (BswIcon *icon)
{
  g_return_if_fail (icon != NULL);
  g_return_if_fail (icon->ref_count > 0);

  if (!(icon->ref_count & STATIC_REF_COUNT))
    {
      icon->ref_count -= 1;
      if (!icon->ref_count)
	{
	  g_free (icon->pixels);
	  g_free (icon);
	}
    }
}
