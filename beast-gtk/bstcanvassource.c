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
#include	"bstcanvassource.h"

#include	"../PKG_config.h"

#include	"bstparamview.h"
#include	"bstgconfig.h"



/* --- defines --- */
#define	ICON_WIDTH	((gdouble) 64)
#define	ICON_HEIGHT	((gdouble) 64)
#define CHANNEL_WIDTH	((gdouble) 10)
#define CHANNEL_HEIGHT	((gdouble) ICON_HEIGHT)
#define	ICON_X		((gdouble) CHANNEL_WIDTH)
#define	ICON_Y		((gdouble) 0)
#define ICHANNEL_realX	((gdouble) 0)
#define OCHANNEL_realX	((gdouble) CHANNEL_WIDTH + ICON_WIDTH)
#define ICHANNEL_X(cs)	(cs->swap_channels ? OCHANNEL_realX : ICHANNEL_realX)
#define OCHANNEL_X(cs)	(cs->swap_channels ? ICHANNEL_realX : OCHANNEL_realX)
#define ICHANNEL_Y	((gdouble) 0)
#define OCHANNEL_Y	((gdouble) 0)
#define	TOTAL_WIDTH	((gdouble) CHANNEL_WIDTH + ICON_WIDTH + CHANNEL_WIDTH)
#define	TOTAL_HEIGHT	((gdouble) ICON_HEIGHT + TEXT_HEIGHT)
#define TEXT_X		((gdouble) CHANNEL_WIDTH + ICON_WIDTH / 2) /* anchor: north */
#define TEXT_Y		((gdouble) ICON_HEIGHT)
#define TEXT_HEIGHT	((gdouble) 13)
#define RGBA_BLACK	(0x000000ff)


/* --- signals --- */
enum
{
  SIGNAL_UPDATE_LINKS,
  SIGNAL_LAST
};
typedef void    (*SignalUpdateLinks)            (BstCanvasSource       *source,
						 gpointer         func_data);


/* --- prototypes --- */
static void	bst_canvas_source_class_init	(BstCanvasSourceClass	*class);
static void	bst_canvas_source_init		(BstCanvasSource	*csource);
static void	bst_canvas_source_build		(BstCanvasSource	*csource);
static void	bst_canvas_source_destroy	(GtkObject		*object);
static gboolean bst_canvas_source_event		(GnomeCanvasItem        *item,
						 GdkEvent               *event);
static gboolean bst_canvas_source_child_event	(BstCanvasSource	*csource,
						 GdkEvent               *event,
						 GnomeCanvasItem        *child);
static void     bst_canvas_source_changed       (BstCanvasSource        *csource);
static void	bst_canvas_icon_set		(GnomeCanvasItem	*item,
						 BseIcon         	*icon);


/* --- static variables --- */
static gpointer              parent_class = NULL;
static BstCanvasSourceClass *bst_canvas_source_class = NULL;
static guint                 csource_signals[SIGNAL_LAST] = { 0 };


/* --- functions --- */
GtkType
bst_canvas_source_get_type (void)
{
  static GtkType canvas_source_type = 0;
  
  if (!canvas_source_type)
    {
      GtkTypeInfo canvas_source_info =
      {
	"BstCanvasSource",
	sizeof (BstCanvasSource),
	sizeof (BstCanvasSourceClass),
	(GtkClassInitFunc) bst_canvas_source_class_init,
	(GtkObjectInitFunc) bst_canvas_source_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      canvas_source_type = gtk_type_unique (GNOME_TYPE_CANVAS_GROUP, &canvas_source_info);
    }
  
  return canvas_source_type;
}

static void
bst_canvas_source_class_init (BstCanvasSourceClass *class)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GnomeCanvasItemClass *canvas_item_class = GNOME_CANVAS_ITEM_CLASS (class);
  /* GnomeCanvasGroupClass *canvas_group_class = GNOME_CANVAS_GROUP_CLASS (class); */
  
  bst_canvas_source_class = class;
  parent_class = g_type_class_peek_parent (class);
  
  object_class->destroy = bst_canvas_source_destroy;
  canvas_item_class->event = bst_canvas_source_event;
  class->update_links = NULL;

  csource_signals[SIGNAL_UPDATE_LINKS] =
    gtk_signal_new ("update-links",
		    GTK_RUN_LAST,
		    GTK_CLASS_TYPE (object_class),
		    GTK_SIGNAL_OFFSET (BstCanvasSourceClass, update_links),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);
}

static void
bst_canvas_source_init (BstCanvasSource *csource)
{
  GtkObject *object = GTK_OBJECT (csource);
  
  csource->source = 0;
  csource->source_view = NULL;
  csource->source_info = NULL;
  csource->icon_item = NULL;
  csource->text = NULL;
  csource->channel_items = NULL;
  csource->swap_channels = BST_SNET_SWAP_IO_CHANNELS;
  csource->in_move = FALSE;
  csource->move_dx = 0;
  csource->move_dy = 0;
  g_object_connect (object,
		    "signal::notify", bst_canvas_source_changed, NULL,
		    NULL);
}

static void
source_channels_changed (BstCanvasSource *csource)
{
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  GNOME_CANVAS_NOTIFY (csource);
  bst_canvas_source_update_links (csource);
}

static void
source_name_changed (BstCanvasSource *csource)
{
  BswProxy project;
  gchar *pname;
  gchar *name;

  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  name = bsw_item_get_name_or_type (csource->source);
  project = bsw_item_get_project (csource->source);
  pname = bsw_item_get_name_or_type (project);
  pname = g_strconcat (pname, ": ", name, NULL);

  if (csource->text)
    g_object_set (csource->text, "text", name, NULL);

  if (csource->source_view)
    gtk_window_set_title (GTK_WINDOW (csource->source_view), pname);
  if (csource->source_info)
    gtk_window_set_title (GTK_WINDOW (csource->source_info), pname);

  g_free (pname);
}

static void
source_icon_changed (BstCanvasSource *csource)
{
  BseIcon *icon;

  /* update icon in group, revert to a stock icon if none is available
   */
  icon = bse_object_get_icon (bse_object_from_id (csource->source));
  if (!icon)
    icon = bst_icon_from_stock (BST_ICON_NOICON);
  bst_canvas_icon_set (csource->icon_item, icon);
}

static void
bst_canvas_source_destroy (GtkObject *object)
{
  BstCanvasSource *csource = BST_CANVAS_SOURCE (object);
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (object);

  while (group->item_list)
    gtk_object_destroy (group->item_list->data);

  if (csource->source)
    {
      g_object_disconnect (bse_object_from_id (csource->source),
			   "any_signal", gtk_object_destroy, csource,
			   "any_signal", source_channels_changed, csource,
			   "any_signal", source_name_changed, csource,
			   "any_signal", source_icon_changed, csource,
			   NULL);
      csource->source = 0;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

#define EPSILON 1e-6

static void
bse_object_set_parasite_coords (BseObject *object,
				gdouble    x,
				gdouble    y)
{
  gfloat coords[2];
  
  g_return_if_fail (BSE_IS_OBJECT (object));
  
  coords[0] = x;
  coords[1] = y;
  
  /* g_print ("set-coords: %p %f %f\n", object, x, y); */
  
  bse_parasite_set_floats (object, "BstRouterCoords", 2, coords);
}

static gboolean
bse_object_get_parasite_coords (BseObject *object,
				gdouble   *x,
				gdouble   *y)
{
  gfloat coords[2];
  
  g_return_val_if_fail (BSE_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (x != NULL && y != NULL, FALSE);
  
  if (bse_parasite_get_floats (object, "BstRouterCoords", 2, coords) == 2)
    {
      *x = coords[0];
      *y = coords[1];
      /* g_print ("get-coords: %p %f %f\n", object, *x, *y); */
      
      return BSE_EPSILON_CMP (coords[0], 0) || BSE_EPSILON_CMP (coords[1], 0);
    }
  
  return FALSE;
}

GnomeCanvasItem*
bst_canvas_source_new (GnomeCanvasGroup *group,
		       BswProxy		 source,
		       gdouble           world_x,
		       gdouble           world_y)
{
  BstCanvasSource *csource;
  GnomeCanvasItem *item;
  
  g_return_val_if_fail (GNOME_IS_CANVAS_GROUP (group), NULL);
  g_return_val_if_fail (BSE_IS_SOURCE (bse_object_from_id (source)), NULL);

  item = gnome_canvas_item_new (group,
				BST_TYPE_CANVAS_SOURCE,
				NULL);
  csource = BST_CANVAS_SOURCE (item);
  csource->source = source;
  g_object_connect (bse_object_from_id (csource->source),
		    "swapped_signal::destroy", gtk_object_destroy, csource,
		    "swapped_signal::io_changed", source_channels_changed, csource,
		    "swapped_signal::notify::name", source_name_changed, csource,
		    "swapped_signal::icon-changed", source_icon_changed, csource,
		    NULL);

  if (bse_object_get_parasite_coords (bse_object_from_id (csource->source), &world_x, &world_y))
    {
      world_x = - world_x;
      world_y = - world_y;
      gnome_canvas_item_w2i (item, &world_x, &world_y);
      gnome_canvas_item_move (item, world_x, world_y);
    }
  else
    {
      gnome_canvas_item_w2i (item, &world_x, &world_y);
      gnome_canvas_item_move (item, world_x, world_y);
    }
  bst_canvas_source_build (BST_CANVAS_SOURCE (item));
  
  GNOME_CANVAS_NOTIFY (item);
  
  return item;
}

void
bst_canvas_source_update_links (BstCanvasSource *csource)
{
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  if (csource->source)
    gtk_signal_emit (GTK_OBJECT (csource), csource_signals[SIGNAL_UPDATE_LINKS]);
}

void
bst_canvas_source_popup_view (BstCanvasSource *csource)
{
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  if (!csource->source_view)
    {
      GtkWidget *param_view;

      param_view = bst_param_view_new (csource->source);
      gtk_widget_show (param_view);
      csource->source_view = bst_adialog_new (GTK_OBJECT (csource), &csource->source_view, param_view,
					      BST_ADIALOG_POPUP_POS, NULL);
      source_name_changed (csource);
    }
  gtk_widget_showraise (csource->source_view);
}

void
bst_canvas_source_toggle_view (BstCanvasSource *csource)
{
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  if (!csource->source_view || !GTK_WIDGET_VISIBLE (csource->source_view))
    bst_canvas_source_popup_view (csource);
  else
    gtk_widget_hide (csource->source_view);
}

static void
csource_info_update (BstCanvasSource *csource)
{
  GtkWidget *frame = (csource->source_info // && (force_update || GTK_WIDGET_VISIBLE (csource->source_info))
		      ? bst_adialog_get_child (csource->source_info)
		      : NULL);

  if (frame)
    {
      GtkWidget *label = GTK_BIN (frame)->child;
      GString *gstring = g_string_new ("");
      guint i;

      /* construct information
       */
      if (bsw_source_n_ichannels (csource->source))
	g_string_printfa (gstring, "Input Channels:\n");
      for (i = 0; i < bsw_source_n_ichannels (csource->source); i++)
	g_string_printfa (gstring,
			  "  %s:\n"
			  "    %s\n",
			  bsw_source_ichannel_name (csource->source, i),
			  bsw_source_ichannel_blurb (csource->source, i));
      if (bsw_source_n_ochannels (csource->source))
	{
	  if (bsw_source_n_ichannels (csource->source))
	    g_string_append_c (gstring, '\n');
	  g_string_printfa (gstring, "Output Channels:\n");
	}
      for (i = 0; i < bsw_source_n_ochannels (csource->source); i++)
	g_string_printfa (gstring,
			  "  %s:\n"
			  "    %s\n",
			  bsw_source_ochannel_name (csource->source, i),
			  bsw_source_ochannel_blurb (csource->source, i));
      g_string_printfa (gstring,
			"%s:\n"
			"  %s\n",
			"Description",
			bsw_item_get_type_blurb (csource->source));
      gtk_label_set_text (GTK_LABEL (label), gstring->str);
      g_string_free (gstring, TRUE);
    }
}

void
bst_canvas_source_popup_info (BstCanvasSource *csource)
{
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  if (!csource->source_info)
    csource->source_info = bst_adialog_new (GTK_OBJECT (csource),
					    &csource->source_info,
					    gtk_widget_new (GTK_TYPE_FRAME,
							    "visible", TRUE,
							    "border_width", 5,
							    "label", "Module Info",
							    "child", gtk_widget_new (GTK_TYPE_LABEL,
										     "visible", TRUE,
										     "justify", GTK_JUSTIFY_LEFT,
										     "xpad", 5,
										     NULL),
							    NULL),
					    BST_ADIALOG_POPUP_POS,
					    NULL);
  csource_info_update (csource);
  source_name_changed (csource);
  gtk_widget_showraise (csource->source_info);
}

void
bst_canvas_source_toggle_info (BstCanvasSource *csource)
{
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));

  if (!csource->source_info || !GTK_WIDGET_VISIBLE (csource->source_info))
    bst_canvas_source_popup_info (csource);
  else
    gtk_widget_hide (csource->source_info);
}

BstCanvasSource*
bst_canvas_source_at (GnomeCanvas *canvas,
		      gdouble      world_x,
		      gdouble      world_y)
{
  return (BstCanvasSource*) gnome_canvas_typed_item_at (canvas, BST_TYPE_CANVAS_SOURCE, world_x, world_y);
}

gboolean
bst_canvas_source_is_jchannel (BstCanvasSource *csource,
			       guint            ichannel)
{
  g_return_val_if_fail (BST_IS_CANVAS_SOURCE (csource), FALSE);

  if (!csource->source)
    return FALSE;

  return bsw_source_is_joint_ichannel (csource->source, ichannel);
}

void
bst_canvas_source_ichannel_pos (BstCanvasSource *csource,
				guint            ochannel,
				gdouble         *x_p,
				gdouble         *y_p)
{
  gdouble x = 0, y = 0;
  
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));
  
  x = ICHANNEL_X (csource) + CHANNEL_WIDTH / 2;
  if (csource->source)
    y = CHANNEL_HEIGHT / bsw_source_n_ichannels (csource->source);
  y *= ochannel + 0.5;
  y += ICHANNEL_Y;
  gnome_canvas_item_i2w (GNOME_CANVAS_ITEM (csource), &x, &y);
  if (x_p)
    *x_p = x;
  if (y_p)
    *y_p = y;
}

void
bst_canvas_source_ochannel_pos (BstCanvasSource *csource,
				guint            ichannel,
				gdouble         *x_p,
				gdouble         *y_p)
{
  gdouble x, y;
  
  g_return_if_fail (BST_IS_CANVAS_SOURCE (csource));
  
  x = OCHANNEL_X (csource) + CHANNEL_WIDTH / 2;
  if (csource->source)
    y = CHANNEL_HEIGHT / bsw_source_n_ochannels (csource->source);
  y *= ichannel + 0.5;
  y += OCHANNEL_Y;
  gnome_canvas_item_i2w (GNOME_CANVAS_ITEM (csource), &x, &y);
  if (x_p)
    *x_p = x;
  if (y_p)
    *y_p = y;
}

guint
bst_canvas_source_ichannel_at (BstCanvasSource *csource,
			       gdouble	        x,
			       gdouble	        y)
{
  guint channel = ~0;

  g_return_val_if_fail (BST_IS_CANVAS_SOURCE (csource), 0);

  gnome_canvas_item_w2i (GNOME_CANVAS_ITEM (csource), &x, &y);

  x -= ICHANNEL_X (csource);
  y -= ICHANNEL_Y;
  if (x > 0 && x < CHANNEL_WIDTH &&
      y > 0 && y < CHANNEL_HEIGHT &&
      bsw_source_n_ichannels (csource->source))
    {
      y /= CHANNEL_HEIGHT / bsw_source_n_ichannels (csource->source);
      channel = y;
    }

  return channel;
}

guint
bst_canvas_source_ochannel_at (BstCanvasSource *csource,
			       gdouble	        x,
			       gdouble	        y)
{
  guint channel = ~0;

  g_return_val_if_fail (BST_IS_CANVAS_SOURCE (csource), 0);

  gnome_canvas_item_w2i (GNOME_CANVAS_ITEM (csource), &x, &y);

  x -= OCHANNEL_X (csource);
  y -= OCHANNEL_Y;
  if (x > 0 && x < CHANNEL_WIDTH &&
      y > 0 && y < CHANNEL_HEIGHT &&
      bsw_source_n_ochannels (csource->source))
    {
      y /= CHANNEL_HEIGHT / bsw_source_n_ochannels (csource->source);
      channel = y;
    }

  return channel;
}

static void
bst_canvas_icon_set (GnomeCanvasItem *item,
		     BseIcon         *icon)
{
  GdkPixbuf *pixbuf;
  
  bse_icon_ref (icon);
#if 0
  ArtPixBuf *apixbuf;
  apixbuf = (icon->bytes_per_pixel > 3
	     ? art_pixbuf_new_const_rgba
	     : art_pixbuf_new_const_rgb) (icon->pixels,
					  icon->width,
					  icon->height,
					  icon->width *
					  icon->bytes_per_pixel);
  pixbuf = gdk_pixbuf_new_from_art_pixbuf (apixbuf);
#endif
  pixbuf = gdk_pixbuf_new_from_data (icon->pixels, GDK_COLORSPACE_RGB, TRUE,
				     8, icon->width, icon->height,
				     icon->width * icon->bytes_per_pixel,
				     NULL, NULL);
  g_object_set (GTK_OBJECT (item),
		"pixbuf", pixbuf,
		"x_in_pixels", FALSE,
		"y_in_pixels", FALSE,
		"anchor", GTK_ANCHOR_NORTH_WEST,
		// "x_set", TRUE,
		// "y_set", TRUE,
		NULL);
  gtk_object_set_data_full (GTK_OBJECT (item),
			    "BseIcon",
			    icon,
			    (GtkDestroyNotify) bse_icon_unref);
  gdk_pixbuf_unref (pixbuf);
}

static void
channel_item_remove (BstCanvasSource *csource,
		     GnomeCanvasItem *item)
{
  csource->channel_items = g_slist_remove (csource->channel_items, item);
}

static void
bst_canvas_source_build_channels (BstCanvasSource *csource,
				  gboolean         is_input,
				  gint             color1,
				  gint		   color1_fade,
				  gint		   color2,
				  gint		   color2_fade)
{
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (csource);
  const guint alpha = 0xa0;
  gint n_channels, color1_delta = 0, color2_delta = 0;
  gdouble x1, x2, y1, y2;
  gdouble d_y;
  guint i;

  if (is_input)
    {
      n_channels = bsw_source_n_ichannels (csource->source);
      x1 = ICHANNEL_X (csource);
      y1 = ICHANNEL_Y;
    }
  else
    {
      n_channels = bsw_source_n_ochannels (csource->source);
      x1 = OCHANNEL_X (csource);
      y1 = OCHANNEL_Y;
    }
  x2 = x1 + CHANNEL_WIDTH;
  y2 = y1 + CHANNEL_HEIGHT;
  d_y = y2 - y1;
  d_y /= n_channels;
  if (n_channels > 1)
    {
      gint cd_red, cd_blue, cd_green;

      cd_red = ((color1_fade & 0xff0000) - (color1 & 0xff0000)) / (n_channels - 1);
      cd_green = ((color1_fade & 0x00ff00) - (color1 & 0x00ff00)) / (n_channels - 1);
      cd_blue = ((color1_fade & 0x0000ff) - (color1 & 0x0000ff)) / (n_channels - 1);
      color1_delta = (cd_red & ~0xffff) + (cd_green & ~0xff) + cd_blue;

      cd_red = ((color2_fade & 0xff0000) - (color2 & 0xff0000)) / (n_channels - 1);
      cd_green = ((color2_fade & 0x00ff00) - (color2 & 0x00ff00)) / (n_channels - 1);
      cd_blue = ((color2_fade & 0x0000ff) - (color2 & 0x0000ff)) / (n_channels - 1);
      color2_delta = (cd_red & ~0xffff) + (cd_green & ~0xff) + cd_blue;
    }
  else if (n_channels == 0)
    {
      GnomeCanvasItem *item;

      item = g_object_connect (gnome_canvas_item_new (group,
						      GNOME_TYPE_CANVAS_RECT,
						      "fill_color_rgba", (0xc3c3c3 << 8) | alpha,
						      "outline_color_rgba", RGBA_BLACK,
						      "x1", x1,
						      "y1", y1,
						      "x2", x2,
						      "y2", y2,
						      NULL),
			       "swapped_signal::destroy", channel_item_remove, csource,
			       "swapped_signal::event", bst_canvas_source_child_event, csource,
			       NULL);
      csource->channel_items = g_slist_prepend (csource->channel_items, item);
    }

  for (i = 0; i < n_channels; i++)
    {
      GnomeCanvasItem *item;
      gboolean is_jchannel = is_input && bsw_source_is_joint_ichannel (csource->source, i);
      guint tmp_color = is_jchannel ? color2 : color1;

      y2 = y1 + d_y;
      item = g_object_connect (gnome_canvas_item_new (group,
						      GNOME_TYPE_CANVAS_RECT,
						      "fill_color_rgba", (tmp_color << 8) | alpha,
						      "outline_color_rgba", RGBA_BLACK,
						      "x1", x1,
						      "y1", y1,
						      "x2", x2,
						      "y2", y2,
						      NULL),
			       "swapped_signal::destroy", channel_item_remove, csource,
			       "swapped_signal::event", bst_canvas_source_child_event, csource,
			       NULL);
      csource->channel_items = g_slist_prepend (csource->channel_items, item);
      color1 += color1_delta;
      color2 += color2_delta;
      y1 = y2;
    }
}

static void
bst_canvas_source_build (BstCanvasSource *csource)
{
  GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (csource);
  GnomeCanvasPoints *gpoints;
  GnomeCanvasItem *item;

  /* order of creation is important to enforce stacking */

  /* add icon to group
   */
  csource->icon_item = g_object_connect (gnome_canvas_item_new (group,
								GNOME_TYPE_CANVAS_PIXBUF,
								"x", ICON_X,
								"y", ICON_Y,
								"width", ICON_WIDTH,
								"height", ICON_HEIGHT,
								NULL),
					 "signal::destroy", gtk_widget_destroyed, &csource->icon_item,
					 "swapped_signal::event", bst_canvas_source_child_event, csource,
					 NULL);
  source_icon_changed (csource);

  /* add text item, invoke name_changed callback to setup the text value
   */
  csource->text = g_object_connect (gnome_canvas_item_new (group,
							   GNOME_TYPE_CANVAS_TEXT,
							   "fill_color", "black",
							   "anchor", GTK_ANCHOR_NORTH,
							   "justification", GTK_JUSTIFY_CENTER,
							   "x", TEXT_X,
							   "y", TEXT_Y,
							   // "font", "Serif 12",
							   "font", "Sans 12",
							   NULL),
				    "signal::destroy", gtk_widget_destroyed, &csource->text,
				    "swapped_signal::event", bst_canvas_source_child_event, csource,
				    NULL);
  source_name_changed (csource);

  /* add input and output channel items
   */
  while (csource->channel_items)
    gtk_object_destroy (csource->channel_items->data);
  bst_canvas_source_build_channels (csource,
				    TRUE, /* input channels */
				    0xffff00, 0x808000,	  /* ichannels */
				    0x00afff, 0x005880);  /* jchannels */
  bst_canvas_source_build_channels (csource,
				    FALSE, /* output channels */
				    0xff0000, 0x800000,
				    0, 0); /* unused */

  /* put line above and below the icon
   */
  if (0)
    {
      gpoints = gnome_canvas_points_newv (2, ICON_X, ICON_Y, ICON_X + ICON_WIDTH, ICON_Y);
      item = g_object_connect (gnome_canvas_item_new (group,
						      GNOME_TYPE_CANVAS_LINE,
						      "fill_color_rgba", RGBA_BLACK,
						      "points", gpoints,
						      NULL),
			       "swapped_signal::event", bst_canvas_source_child_event, csource,
			       NULL);
      gpoints->coords[1] += ICON_HEIGHT;
      gpoints->coords[3] += ICON_HEIGHT;
      item = g_object_connect (gnome_canvas_item_new (group,
						      GNOME_TYPE_CANVAS_LINE,
						      "fill_color_rgba", RGBA_BLACK,
						      "points", gpoints,
						      NULL),
			       "swapped_signal::event", bst_canvas_source_child_event, csource,
			       NULL);
      gnome_canvas_points_free (gpoints);
    }
  
  /* put a line at the bottom (ontop of the text) to close
   * text bounding rectangle
   */
  gpoints = gnome_canvas_points_newv (2,
				      CHANNEL_WIDTH, TEXT_Y,
				      CHANNEL_WIDTH + ICON_WIDTH, TEXT_Y);
  item = g_object_connect (gnome_canvas_item_new (group,
						  GNOME_TYPE_CANVAS_LINE,
						  "fill_color_rgba", RGBA_BLACK,
						  "points", gpoints,
						  NULL),
			   "swapped_signal::event", bst_canvas_source_child_event, csource,
			   NULL);
  gnome_canvas_points_free (gpoints);
  
  /* put an outer rectangle, make it transparent in aa mode,
   * so we can receive mouse events everywhere
   */
  item = g_object_connect (gnome_canvas_item_new (group,
						  GNOME_TYPE_CANVAS_RECT,
						  "outline_color_rgba", RGBA_BLACK, /* covers buggy canvas lines */
						  "x1", 0.0,
						  "y1", 0.0,
						  "x2", TOTAL_WIDTH,
						  "y2", TOTAL_HEIGHT,
						  (GNOME_CANVAS_ITEM (csource)->canvas->aa
						   ? "fill_color_rgba"
						   : NULL), 0x00000000,
						  NULL),
			   "swapped_signal::event", bst_canvas_source_child_event, csource,
			   NULL);
}

static void
bst_canvas_source_changed (BstCanvasSource *csource)
{
  if (csource->source)
    {
      GnomeCanvasItem *item = GNOME_CANVAS_ITEM (csource);
      gdouble x = 0, y = 0;

      gnome_canvas_item_w2i (item, &x, &y);
      bse_object_set_parasite_coords (bse_object_from_id (csource->source), x, y);
    }
}

static gboolean
bst_canvas_source_event (GnomeCanvasItem *item,
			 GdkEvent        *event)
{
  BstCanvasSource *csource = BST_CANVAS_SOURCE (item);
  gboolean handled = FALSE;
  
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      if (event->button.button == 2)
	{
	  GdkCursor *fleur;
	  gdouble x = event->button.x, y = event->button.y;
	  
	  gnome_canvas_item_w2i (item, &x, &y);
	  csource->move_dx = x;
	  csource->move_dy = y;
	  csource->in_move = TRUE;
	  
	  fleur = gdk_cursor_new (GDK_FLEUR);
	  gnome_canvas_item_grab (item,
				  GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				  fleur,
				  event->button.time);
	  gdk_cursor_destroy (fleur);
	  handled = TRUE;
	}
      break;
    case GDK_MOTION_NOTIFY:
      if (csource->in_move)
	{
	  gdouble x = event->motion.x, y = event->motion.y;
	  
	  gnome_canvas_item_w2i (item, &x, &y);
	  gnome_canvas_item_move (item, x - csource->move_dx, y - csource->move_dy);
	  GNOME_CANVAS_NOTIFY (item);
	  handled = TRUE;
	}
      break;
    case GDK_BUTTON_RELEASE:
      if (event->button.button == 2 && csource->in_move)
	{
	  csource->in_move = FALSE;
	  gnome_canvas_item_ungrab (item, event->button.time);
	  handled = TRUE;
	}
      break;
    default:
      break;
    }
  
  if (!handled && GNOME_CANVAS_ITEM_CLASS (parent_class)->event)
    handled = GNOME_CANVAS_ITEM_CLASS (parent_class)->event (item, event);
  
  return handled;
}

static gboolean
bst_canvas_source_child_event (BstCanvasSource *csource,
			       GdkEvent        *event,
			       GnomeCanvasItem *child)
{
  GnomeCanvasItem *item = GNOME_CANVAS_ITEM (csource);
  GtkWidget *widget = GTK_WIDGET (item->canvas);
  gboolean handled = FALSE;
  
  csource = BST_CANVAS_SOURCE (item);

  switch (event->type)
    {
    case GDK_ENTER_NOTIFY:
      if (!GTK_WIDGET_HAS_FOCUS (widget))
	gtk_widget_grab_focus (widget);
      handled = TRUE;
      break;
    case GDK_LEAVE_NOTIFY:
      handled = TRUE;
      break;
    case GDK_KEY_PRESS:
      switch (event->key.keyval)
	{
	case 'L':
	case 'l':
	  gnome_canvas_item_lower_to_bottom (item);
	  GNOME_CANVAS_NOTIFY (item);
	  break;
	case 'R':
	case 'r':
	  gnome_canvas_item_raise_to_top (item);
	  GNOME_CANVAS_NOTIFY (item);
	  break;
	}
      handled = TRUE;
      break;
    case GDK_KEY_RELEASE:
      handled = TRUE;
      break;
    default:
      break;
    }
  
  return handled;
}
