/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_DEFS_H__
#define __BST_DEFS_H__

#include	"bstutils.h"
#include	"gnomeforest.h"
#include	"glewidgets.h"
#include	"bstzoomedwindow.h"
#include	"bstfreeradiobutton.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BEAST mainmenu operations --- */
typedef enum
{
  BST_OP_NONE,

  /* project operations
   */
  BST_OP_PROJECT_NEW,
  BST_OP_PROJECT_OPEN,
  BST_OP_PROJECT_SAVE,
  BST_OP_PROJECT_SAVE_AS,
  BST_OP_PROJECT_NEW_SONG,
  BST_OP_PROJECT_NEW_SNET,
  BST_OP_PROJECT_NEW_MIDI_SYNTH,
  BST_OP_PROJECT_CLOSE,
  BST_OP_PROJECT_PLAY,
  BST_OP_PROJECT_STOP,

  /* spawn new dialogs
   */
  BST_OP_DIALOG_PREFERENCES,
  BST_OP_DIALOG_DEVICE_MONITOR,

  /* debugging */
  BST_OP_REBUILD,
  BST_OP_REFRESH,

  /* song operations
   */
  BST_OP_PATTERN_ADD,
  BST_OP_PATTERN_DELETE,
  BST_OP_PATTERN_EDITOR,
  BST_OP_WAVE_LOAD,
  BST_OP_WAVE_DELETE,
  BST_OP_WAVE_EDITOR,
  BST_OP_INSTRUMENT_ADD,
  BST_OP_INSTRUMENT_DELETE,
  BST_OP_EFFECT_ADD,
  BST_OP_EFFECT_DELETE,

  /* super operations
   */
  BST_OP_UNDO_LAST,
  BST_OP_REDO_LAST,

  /* application wide
   */
  BST_OP_EXIT,

  /* help dialogs
   */
#define BST_OP_HELP_FIRST	BST_OP_HELP_ABOUT
  BST_OP_HELP_ABOUT,
  BST_OP_HELP_FAQ,
  BST_OP_HELP_KEYTABLE,
  BST_OP_HELP_HEART,
  BST_OP_HELP_NETWORKS,
#define	BST_OP_HELP_LAST	BST_OP_HELP_NETWORKS

  BST_OP_LAST
} BstOps;


/* --- constants & defines --- */
#define	BST_TAG_DIAMETER	  (20)
#define BST_STRDUP_RC_FILE()	  (g_strconcat (g_get_home_dir (), "/.beastrc", NULL))
#define BST_BUTTON_ICON_WIDTH	  (32)
#define BST_BUTTON_ICON_HEIGHT	  (32)
#define BST_DRAG_ICON_WIDTH	  (BST_BUTTON_ICON_WIDTH)
#define BST_DRAG_ICON_HEIGHT	  (BST_BUTTON_ICON_HEIGHT)
#define BST_TOOLBAR_ICON_WIDTH    (BST_BUTTON_ICON_WIDTH)
#define BST_TOOLBAR_ICON_HEIGHT   (BST_BUTTON_ICON_WIDTH)
#define BST_PALETTE_ICON_WIDTH    (BST_TOOLBAR_ICON_WIDTH)
#define BST_PALETTE_ICON_HEIGHT   (BST_TOOLBAR_ICON_HEIGHT)
#define BST_DRAG_BUTTON_COPY	  (1)
#define BST_DRAG_BUTTON_COPY_MASK (GDK_BUTTON1_MASK)
#define BST_DRAG_BUTTON_MOVE	  (2)
#define BST_DRAG_BUTTON_MOVE_MASK (GDK_BUTTON2_MASK)
#define BST_DRAG_BUTTON_CONTEXT   (3) /* delete, clone, linkdup */

/* --- debug stuff --- */
typedef enum                    /*< skip >*/
{ /* keep in sync with bstmain.c */
  BST_DEBUG_KEYTABLE		= (1 << 0),
  BST_DEBUG_SAMPLES		= (1 << 1)
} BstDebugFlags;
extern BstDebugFlags bst_debug_flags;
#ifdef G_ENABLE_DEBUG
#  define BST_IF_DEBUG(type)	if (!(bst_debug_flags & BST_DEBUG_ ## type)) { } else
#else  /* !G_ENABLE_DEBUG */
#  define BST_IF_DEBUG(type)	while (0) /* don't exec */
#endif /* !G_ENABLE_DEBUG */



extern void bst_update_can_operate (GtkWidget   *some_widget);

#define	GNOME_CANVAS_NOTIFY(object)	G_STMT_START { \
    if (GTK_IS_OBJECT (object)) \
      g_signal_emit_by_name (object, "notify::generic-change", NULL); \
} G_STMT_END

extern GtkTooltips *bst_global_tooltips;
#define	BST_TOOLTIPS	(*&bst_global_tooltips)




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_DEFS_H__ */
