/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
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
#include	"bstpianoroll.h"

#include	"bstasciipixbuf.h"
#include	<gdk/gdkkeysyms.h>


/* --- defines --- */
/* helpers */
#define	STYLE(self)		(GTK_WIDGET (self)->style)
#define STATE(self)             (GTK_WIDGET (self)->state)
#define XTHICKNESS(self)        (STYLE (self)->xthickness)
#define YTHICKNESS(self)        (STYLE (self)->ythickness)
#define	ALLOCATION(self)	(&GTK_WIDGET (self)->allocation)
#define	N_OCTAVES(self)		(MAX_OCTAVE (self) - MIN_OCTAVE (self) + 1)
#define	MAX_OCTAVE(self)	(SFI_NOTE_OCTAVE ((self)->max_note))
#define	MAX_SEMITONE(self)	(SFI_NOTE_SEMITONE ((self)->max_note))
#define	MIN_OCTAVE(self)	(SFI_NOTE_OCTAVE ((self)->min_note))
#define	MIN_SEMITONE(self)	(SFI_NOTE_SEMITONE ((self)->min_note))

/* layout (requisition) */
#define	NOTE_HEIGHT(self)	((gint) ((self)->vzoom * 1.2))		/* factor must be between 1 .. 2 */
#define	OCTAVE_HEIGHT(self)	(14 * (self)->vzoom + 7 * NOTE_HEIGHT (self))	/* coord_to_note() */
#define KEYBOARD_X(self)        (XTHICKNESS (self))
#define KEYBOARD_WIDTH(self)    (35)
#define	KEYBOARD_RATIO(self)	(2.9 / 5.)	/* black/white key ratio */
#define	VPANEL_WIDTH(self)	(KEYBOARD_X (self) + KEYBOARD_WIDTH (self) + XTHICKNESS (self))
#define	HPANEL_HEIGHT(self)	((self)->hpanel_height)
#define	VPANEL_X(self)		(0)
#define	HPANEL_Y(self)		(0)
#define	CANVAS_X(self)		(VPANEL_X (self) + VPANEL_WIDTH (self))
#define	CANVAS_Y(self)		(HPANEL_Y (self) + HPANEL_HEIGHT (self))

/* layout (allocation) */
#define	CANVAS_WIDTH(self)	(ALLOCATION (self)->width - CANVAS_X (self))
#define	CANVAS_HEIGHT(self)	(ALLOCATION (self)->height - CANVAS_Y (self))

/* aliases */
#define	VPANEL_HEIGHT(self)	(CANVAS_HEIGHT (self))
#define	HPANEL_WIDTH(self)	(CANVAS_WIDTH (self))
#define	VPANEL_Y(self)		(CANVAS_Y (self))
#define	HPANEL_X(self)		(CANVAS_X (self))

/* appearance */
#define VPANEL_BG_COLOR(self)   (&STYLE (self)->bg[GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE])
#define HPANEL_BG_COLOR(self)   (&STYLE (self)->bg[GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE])
#define CANVAS_BG_COLOR(self)   (&STYLE (self)->base[GTK_WIDGET_STATE (self)])
#define	KEY_DEFAULT_VPIXELS	(4)
#define	QNOTE_HPIXELS		(30)	/* guideline */

/* behaviour */
#define AUTO_SCROLL_TIMEOUT	(33)
#define	AUTO_SCROLL_SCALE	(0.2)


/* --- prototypes --- */
static void	bst_piano_roll_dispose			(GObject		*object);
static void	bst_piano_roll_destroy			(GtkObject		*object);
static void	bst_piano_roll_finalize			(GObject		*object);
static void	bst_piano_roll_set_scroll_adjustments	(BstPianoRoll		*self,
							 GtkAdjustment		*hadjustment,
							 GtkAdjustment		*vadjustment);
static void	bst_piano_roll_size_request		(GtkWidget		*widget,
							 GtkRequisition		*requisition);
static void	bst_piano_roll_size_allocate		(GtkWidget		*widget,
							 GtkAllocation		*allocation);
static void	bst_piano_roll_style_set		(GtkWidget		*widget,
							 GtkStyle		*previous_style);
static void	bst_piano_roll_state_changed		(GtkWidget		*widget,
							 guint			 previous_state);
static void	bst_piano_roll_realize			(GtkWidget		*widget);
static void	bst_piano_roll_unrealize		(GtkWidget		*widget);
static gboolean	bst_piano_roll_focus_in			(GtkWidget		*widget,
							 GdkEventFocus		*event);
static gboolean	bst_piano_roll_focus_out		(GtkWidget		*widget,
							 GdkEventFocus		*event);
static gboolean	bst_piano_roll_expose			(GtkWidget		*widget,
							 GdkEventExpose		*event);
static gboolean	bst_piano_roll_key_press		(GtkWidget		*widget,
							 GdkEventKey		*event);
static gboolean	bst_piano_roll_key_release		(GtkWidget		*widget,
							 GdkEventKey		*event);
static gboolean	bst_piano_roll_button_press		(GtkWidget		*widget,
							 GdkEventButton		*event);
static gboolean	bst_piano_roll_motion			(GtkWidget		*widget,
							 GdkEventMotion		*event);
static gboolean	bst_piano_roll_button_release		(GtkWidget		*widget,
							 GdkEventButton		*event);
static void	piano_roll_update_adjustments		(BstPianoRoll		*self,
							 gboolean		 hadj,
							 gboolean		 vadj);
static void	piano_roll_scroll_adjustments		(BstPianoRoll		*self,
							 gint			 x_pixel,
							 gint			 y_pixel);
static void	piano_roll_adjustment_changed		(BstPianoRoll		*self);
static void	piano_roll_adjustment_value_changed	(BstPianoRoll		*self,
							 GtkAdjustment		*adjustment);
static void	bst_piano_roll_hsetup			(BstPianoRoll		*self,
							 guint			 ppqn,
							 guint			 qnpt,
							 guint			 max_ticks,
							 gfloat			 hzoom);

/* --- static variables --- */
static guint	signal_canvas_drag = 0;
static guint	signal_canvas_clicked = 0;
static guint	signal_piano_drag = 0;
static guint	signal_piano_clicked = 0;


/* --- functions --- */
G_DEFINE_TYPE (BstPianoRoll, bst_piano_roll, GTK_TYPE_CONTAINER);

static void
bst_piano_roll_class_init (BstPianoRollClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  gobject_class->dispose = bst_piano_roll_dispose;
  gobject_class->finalize = bst_piano_roll_finalize;

  object_class->destroy = bst_piano_roll_destroy;
  
  widget_class->size_request = bst_piano_roll_size_request;
  widget_class->size_allocate = bst_piano_roll_size_allocate;
  widget_class->realize = bst_piano_roll_realize;
  widget_class->unrealize = bst_piano_roll_unrealize;
  widget_class->style_set = bst_piano_roll_style_set;
  widget_class->state_changed = bst_piano_roll_state_changed;
  widget_class->expose_event = bst_piano_roll_expose;
  widget_class->focus_in_event = bst_piano_roll_focus_in;
  widget_class->focus_out_event = bst_piano_roll_focus_out;
  widget_class->key_press_event = bst_piano_roll_key_press;
  widget_class->key_release_event = bst_piano_roll_key_release;
  widget_class->button_press_event = bst_piano_roll_button_press;
  widget_class->motion_notify_event = bst_piano_roll_motion;
  widget_class->button_release_event = bst_piano_roll_button_release;

  class->set_scroll_adjustments = bst_piano_roll_set_scroll_adjustments;
  class->canvas_clicked = NULL;
  
  signal_canvas_drag = g_signal_new ("canvas-drag", G_OBJECT_CLASS_TYPE (class),
				     G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstPianoRollClass, canvas_drag),
				     NULL, NULL,
				     bst_marshal_NONE__POINTER,
				     G_TYPE_NONE, 1, G_TYPE_POINTER);
  signal_canvas_clicked = g_signal_new ("canvas-clicked", G_OBJECT_CLASS_TYPE (class),
					G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstPianoRollClass, canvas_clicked),
					NULL, NULL,
					bst_marshal_NONE__UINT_UINT_INT_BOXED,
					G_TYPE_NONE, 4, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INT,
					GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  signal_piano_drag = g_signal_new ("piano-drag", G_OBJECT_CLASS_TYPE (class),
				    G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstPianoRollClass, piano_drag),
				    NULL, NULL,
				    bst_marshal_NONE__POINTER,
				    G_TYPE_NONE, 1, G_TYPE_POINTER);
  signal_piano_clicked = g_signal_new ("piano-clicked", G_OBJECT_CLASS_TYPE (class),
				       G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstPianoRollClass, piano_clicked),
				       NULL, NULL,
				       bst_marshal_NONE__UINT_INT_BOXED,
				       G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_INT,
				       GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  widget_class->set_scroll_adjustments_signal =
    gtk_signal_new ("set_scroll_adjustments",
		    GTK_RUN_LAST,
		    GTK_CLASS_TYPE (object_class),
		    GTK_SIGNAL_OFFSET (BstPianoRollClass, set_scroll_adjustments),
		    bst_marshal_NONE__OBJECT_OBJECT,
		    GTK_TYPE_NONE, 2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);
}

static void
bst_piano_roll_init (BstPianoRoll *self)
{
  GtkWidget *widget = GTK_WIDGET (self);

  GTK_WIDGET_UNSET_FLAGS (self, GTK_NO_WINDOW);
  GTK_WIDGET_SET_FLAGS (self, GTK_CAN_FOCUS);
  gtk_widget_set_double_buffered (widget, FALSE);

  self->proxy = 0;
  self->vzoom = KEY_DEFAULT_VPIXELS;
  self->ppqn = 384;	/* default Parts (clock ticks) Per Quarter Note */
  self->qnpt = 1;
  self->max_ticks = 1;
  self->hzoom = 1;
  self->draw_qn_grid = TRUE;
  self->draw_qqn_grid = TRUE;
  self->release_closes_toplevel = TRUE;
  self->min_note = SFI_MIN_NOTE;
  self->max_note = SFI_MAX_NOTE;
  self->hpanel_height = 20;
  self->vpanel = NULL;
  self->hpanel = NULL;
  self->canvas = NULL;
  self->canvas_cursor = GDK_LEFT_PTR;
  self->vpanel_cursor = GDK_HAND2;
  self->hpanel_cursor = GDK_LEFT_PTR;
  self->hadjustment = NULL;
  self->vadjustment = NULL;
  self->scroll_timer = 0;
  self->selection_tick = 0;
  self->selection_duration = 0;
  self->selection_min_note = 0;
  self->selection_max_note = 0;
  bst_piano_roll_hsetup (self, 384, 4, 800 * 384, 1);
  bst_piano_roll_set_hadjustment (self, NULL);
  bst_piano_roll_set_vadjustment (self, NULL);
  self->init_vpos = TRUE;
  
  bst_ascii_pixbuf_ref ();
}

static void
bst_piano_roll_destroy (GtkObject *object)
{
  BstPianoRoll *self = BST_PIANO_ROLL (object);

  bst_piano_roll_set_proxy (self, 0);
  bst_piano_roll_set_hadjustment (self, NULL);
  bst_piano_roll_set_vadjustment (self, NULL);
  
  GTK_OBJECT_CLASS (bst_piano_roll_parent_class)->destroy (object);
}

static void
bst_piano_roll_dispose (GObject *object)
{
  BstPianoRoll *self = BST_PIANO_ROLL (object);

  bst_piano_roll_set_proxy (self, 0);

  G_OBJECT_CLASS (bst_piano_roll_parent_class)->dispose (object);
}

static void
bst_piano_roll_finalize (GObject *object)
{
  BstPianoRoll *self = BST_PIANO_ROLL (object);

  bst_piano_roll_set_proxy (self, 0);

  g_object_unref (self->hadjustment);
  self->hadjustment = NULL;
  g_object_unref (self->vadjustment);
  self->vadjustment = NULL;

  if (self->scroll_timer)
    {
      g_source_remove (self->scroll_timer);
      self->scroll_timer = 0;
    }
  bst_ascii_pixbuf_unref ();

  G_OBJECT_CLASS (bst_piano_roll_parent_class)->finalize (object);
}

gfloat
bst_piano_roll_set_vzoom (BstPianoRoll *self,
			  gfloat        vzoom)
{
  g_return_val_if_fail (BST_IS_PIANO_ROLL (self), 0);

  self->vzoom = vzoom; //  * KEY_DEFAULT_VPIXELS;
  self->vzoom = CLAMP (self->vzoom, 1, 16);

  gtk_widget_queue_resize (GTK_WIDGET (self));

  return self->vzoom;
}

void
bst_piano_roll_set_hadjustment (BstPianoRoll  *self,
				GtkAdjustment *adjustment)
{
  g_return_if_fail (BST_IS_PIANO_ROLL (self));
  if (adjustment)
    g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  if (self->hadjustment)
    {
      g_object_disconnect (self->hadjustment,
			   "any_signal", piano_roll_adjustment_changed, self,
			   "any_signal", piano_roll_adjustment_value_changed, self,
			   NULL);
      g_object_unref (self->hadjustment);
      self->hadjustment = NULL;
    }

  if (!adjustment)
    adjustment = g_object_new (GTK_TYPE_ADJUSTMENT, NULL);

  self->hadjustment = g_object_ref (adjustment);
  gtk_object_sink (GTK_OBJECT (adjustment));
  g_object_connect (self->hadjustment,
		    "swapped_signal::changed", piano_roll_adjustment_changed, self,
		    "swapped_signal::value-changed", piano_roll_adjustment_value_changed, self,
		    NULL);
}

void
bst_piano_roll_set_vadjustment (BstPianoRoll  *self,
				GtkAdjustment *adjustment)
{
  g_return_if_fail (BST_IS_PIANO_ROLL (self));
  if (adjustment)
    g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  if (self->vadjustment)
    {
      g_object_disconnect (self->vadjustment,
			   "any_signal", piano_roll_adjustment_changed, self,
			   "any_signal", piano_roll_adjustment_value_changed, self,
			   NULL);
      g_object_unref (self->vadjustment);
      self->vadjustment = NULL;
    }

  if (!adjustment)
    adjustment = g_object_new (GTK_TYPE_ADJUSTMENT, NULL);

  self->vadjustment = g_object_ref (adjustment);
  gtk_object_sink (GTK_OBJECT (adjustment));
  g_object_connect (self->vadjustment,
		    "swapped_signal::changed", piano_roll_adjustment_changed, self,
		    "swapped_signal::value-changed", piano_roll_adjustment_value_changed, self,
		    NULL);
}

static void
bst_piano_roll_set_scroll_adjustments (BstPianoRoll  *self,
				       GtkAdjustment *hadjustment,
				       GtkAdjustment *vadjustment)
{
  if (self->hadjustment != hadjustment)
    bst_piano_roll_set_hadjustment (self, hadjustment);
  if (self->vadjustment != vadjustment)
    bst_piano_roll_set_vadjustment (self, vadjustment);
}

static void
piano_roll_reset_backgrounds (BstPianoRoll *self)
{
  GtkWidget *widget = GTK_WIDGET (self);

  if (GTK_WIDGET_REALIZED (self))
    {
      GdkColor colors[BST_PIANO_ROLL_N_COLORS] = {
	/* C: */
	{ 0, 0x8080, 0x0000, 0x0000 },	/* dark red */
	{ 0, 0xa000, 0x0000, 0xa000 },	/* dark magenta */
	/* D: */
	{ 0, 0x0000, 0x8000, 0x8000 },	/* dark turquoise */
	{ 0, 0x0000, 0xff00, 0xff00 },	/* light turquoise */
	/* E: */
	{ 0, 0xff00, 0xff00, 0x0000 },	/* bright yellow */
	/* F: */
	{ 0, 0xff00, 0x4000, 0x4000 },	/* light red */
	{ 0, 0xff00, 0x8000, 0x0000 },	/* bright orange */
	/* G: */
	{ 0, 0xb000, 0x0000, 0x6000 },	/* dark pink */
	{ 0, 0xff00, 0x0000, 0x8000 },	/* light pink */
	/* A: */
	{ 0, 0x0000, 0x7000, 0x0000 },	/* dark green */
	{ 0, 0x4000, 0xff00, 0x4000 },	/* bright green */
	/* B: */
	{ 0, 0x4000, 0x4000, 0xff00 },	/* light blue */
      };
      guint i;

      for (i = 0; i < BST_PIANO_ROLL_N_COLORS; i++)
	gdk_gc_set_rgb_fg_color (self->color_gc[i], colors + i);

      gtk_style_set_background (widget->style, widget->window, GTK_WIDGET_STATE (self));
      gdk_window_set_background (self->vpanel, VPANEL_BG_COLOR (self));
      gdk_window_set_background (self->hpanel, HPANEL_BG_COLOR (self));
      gdk_window_set_background (self->canvas, CANVAS_BG_COLOR (self));
      gdk_window_clear (widget->window);
      gdk_window_clear (self->vpanel);
      gdk_window_clear (self->hpanel);
      gdk_window_clear (self->canvas);
      gtk_widget_queue_draw (widget);
    }
}

static void
bst_piano_roll_style_set (GtkWidget *widget,
			  GtkStyle  *previous_style)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);

  self->hpanel_height = 20;
  piano_roll_reset_backgrounds (self);
}

static void
bst_piano_roll_state_changed (GtkWidget *widget,
			      guint	 previous_state)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);

  piano_roll_reset_backgrounds (self);
}

static void
bst_piano_roll_size_request (GtkWidget	    *widget,
			     GtkRequisition *requisition)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);

  requisition->width = CANVAS_X (self) + 500 + XTHICKNESS (self);
  requisition->height = CANVAS_Y (self) + N_OCTAVES (self) * OCTAVE_HEIGHT (self) + YTHICKNESS (self);
}

static void
bst_piano_roll_size_allocate (GtkWidget	    *widget,
			      GtkAllocation *allocation)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);
  guint real_width = allocation->width;
  guint real_height = allocation->height;

  widget->allocation.x = allocation->x;
  widget->allocation.y = allocation->y;
  widget->allocation.width = MAX (CANVAS_X (self) + 1 + XTHICKNESS (self), allocation->width);
  widget->allocation.height = MAX (CANVAS_Y (self) + 1 + YTHICKNESS (self), allocation->height);
  widget->allocation.height = MIN (CANVAS_Y (self) + N_OCTAVES (self) * OCTAVE_HEIGHT (self) + YTHICKNESS (self),
				   widget->allocation.height);

  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (widget->window,
			      widget->allocation.x, widget->allocation.y,
			      real_width, real_height);
      gdk_window_move_resize (self->vpanel,
			      VPANEL_X (self), VPANEL_Y (self),
			      VPANEL_WIDTH (self), VPANEL_HEIGHT (self));
      gdk_window_move_resize (self->hpanel,
			      HPANEL_X (self), HPANEL_Y (self),
			      HPANEL_WIDTH (self), HPANEL_HEIGHT (self));
      gdk_window_move_resize (self->canvas,
			      CANVAS_X (self), CANVAS_Y (self),
			      CANVAS_WIDTH (self), CANVAS_HEIGHT (self));
    }
  piano_roll_update_adjustments (self, TRUE, TRUE);
}

static void
bst_piano_roll_realize (GtkWidget *widget)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);
  GdkWindowAttr attributes;
  GdkCursorType cursor_type;
  guint attributes_mask, i;
  
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  /* widget->window */
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.event_mask = gtk_widget_get_events (widget) | (GDK_EXPOSURE_MASK |
							    GDK_ENTER_NOTIFY_MASK |
							    GDK_LEAVE_NOTIFY_MASK);
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, self);

  /* self->vpanel */
  attributes.x = VPANEL_X (self);
  attributes.y = VPANEL_Y (self);
  attributes.width = VPANEL_WIDTH (self);
  attributes.height = VPANEL_HEIGHT (self);
  attributes.event_mask = gtk_widget_get_events (widget) | (GDK_EXPOSURE_MASK |
							    GDK_BUTTON_PRESS_MASK |
							    GDK_BUTTON_RELEASE_MASK |
							    GDK_BUTTON_MOTION_MASK |
							    GDK_POINTER_MOTION_HINT_MASK);
  self->vpanel = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (self->vpanel, self);
  gdk_window_show (self->vpanel);
  
  /* self->hpanel */
  attributes.x = HPANEL_X (self);
  attributes.y = HPANEL_Y (self);
  attributes.width = HPANEL_WIDTH (self);
  attributes.height = HPANEL_HEIGHT (self);
  attributes.event_mask = gtk_widget_get_events (widget) | (GDK_EXPOSURE_MASK);
  self->hpanel = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (self->hpanel, self);
  gdk_window_show (self->hpanel);

  /* self->canvas */
  attributes.x = CANVAS_X (self);
  attributes.y = CANVAS_Y (self);
  attributes.width = CANVAS_WIDTH (self);
  attributes.height = CANVAS_HEIGHT (self);
  attributes.event_mask = gtk_widget_get_events (widget) | (GDK_EXPOSURE_MASK |
							    GDK_BUTTON_PRESS_MASK |
							    GDK_BUTTON_RELEASE_MASK |
							    GDK_BUTTON_MOTION_MASK |
							    GDK_POINTER_MOTION_HINT_MASK |
							    GDK_KEY_PRESS_MASK |
							    GDK_KEY_RELEASE_MASK);
  self->canvas = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (self->canvas, self);
  gdk_window_show (self->canvas);

  /* allocate color GCs */
  for (i = 0; i < BST_PIANO_ROLL_N_COLORS; i++)
    self->color_gc[i] = gdk_gc_new (self->canvas);

  /* style setup */
  widget->style = gtk_style_attach (widget->style, widget->window);
  piano_roll_reset_backgrounds (self);

  /* update cursors */
  cursor_type = self->canvas_cursor;
  self->canvas_cursor = -1;
  bst_piano_roll_set_canvas_cursor (self, cursor_type);
  cursor_type = self->vpanel_cursor;
  self->vpanel_cursor = -1;
  bst_piano_roll_set_vpanel_cursor (self, cursor_type);
  cursor_type = self->hpanel_cursor;
  self->hpanel_cursor = -1;
  bst_piano_roll_set_hpanel_cursor (self, cursor_type);
}

static void
bst_piano_roll_unrealize (GtkWidget *widget)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);
  guint i;

  for (i = 0; i < BST_PIANO_ROLL_N_COLORS; i++)
    {
      g_object_unref (self->color_gc[i]);
      self->color_gc[i] = NULL;
    }
  gdk_window_set_user_data (self->canvas, NULL);
  gdk_window_destroy (self->canvas);
  self->canvas = NULL;
  gdk_window_set_user_data (self->hpanel, NULL);
  gdk_window_destroy (self->hpanel);
  self->hpanel = NULL;
  gdk_window_set_user_data (self->vpanel, NULL);
  gdk_window_destroy (self->vpanel);
  self->vpanel = NULL;
  self->init_vpos = TRUE;

  if (GTK_WIDGET_CLASS (bst_piano_roll_parent_class)->unrealize)
    GTK_WIDGET_CLASS (bst_piano_roll_parent_class)->unrealize (widget);
}

static gint
ticks_to_pixels (BstPianoRoll *self,
		 gint	       ticks)
{
  gfloat ppqn = self->ppqn;
  gfloat tpixels = QNOTE_HPIXELS;

  /* compute pixel span of a tick range */

  tpixels *= self->hzoom / ppqn * (gfloat) ticks;
  if (ticks)
    tpixels = MAX (tpixels, 1);
  return tpixels;
}

static gint
pixels_to_ticks (BstPianoRoll *self,
		 gint	       pixels)
{
  gfloat ppqn = self->ppqn;
  gfloat ticks = 1.0 / (gfloat) QNOTE_HPIXELS;

  /* compute tick span of a pixel range */

  ticks = ticks * ppqn / self->hzoom * (gfloat) pixels;
  if (pixels > 0)
    ticks = MAX (ticks, 1);
  else
    ticks = 0;
  return ticks;
}

static gint
tick_to_coord (BstPianoRoll *self,
	       gint	     tick)
{
  return ticks_to_pixels (self, tick) - self->x_offset;
}

static gint
coord_to_tick (BstPianoRoll *self,
	       gint	     x,
	       gboolean	     right_bound)
{
  guint tick;

  x += self->x_offset;
  tick = pixels_to_ticks (self, x);
  if (right_bound)
    {
      guint tick2 = pixels_to_ticks (self, x + 1);

      if (tick2 > tick)
	tick = tick2 - 1;
    }
  return tick;
}

#define	CROSSING_TACT		(1)
#define	CROSSING_QNOTE		(2)
#define	CROSSING_QNOTE_Q	(3)

static guint
coord_check_crossing (BstPianoRoll *self,
		      gint	    x,
		      guint	    crossing)
{
  guint ltick = coord_to_tick (self, x, FALSE);
  guint rtick = coord_to_tick (self, x, TRUE);
  guint lq = 0, rq = 0;

  /* catch _at_ tick boundary as well */
  rtick += 1;

  switch (crossing)
    {
    case CROSSING_TACT:
      lq = ltick / (self->ppqn * self->qnpt);
      rq = rtick / (self->ppqn * self->qnpt);
      break;
    case CROSSING_QNOTE:
      lq = ltick / self->ppqn;
      rq = rtick / self->ppqn;
      break;
    case CROSSING_QNOTE_Q:
      lq = ltick * 4 / self->ppqn;
      rq = rtick * 4 / self->ppqn;
      break;
    }

  return lq != rq;
}

#define	DRAW_NONE	(0)
#define	DRAW_START	(1)
#define	DRAW_MIDDLE	(2)
#define	DRAW_END	(3)

typedef struct {
  gint  octave;
  guint semitone;	/* 0 .. 11    within octave */
  guint key;		/* 0 .. 6     index of white key */
  guint key_frac;	/* 0 .. 4*z-1 fractional pixel index into key */
  guint wstate;		/* DRAW_ START/MIDDLE/END of white key */
  guint bstate;		/* DRAW_ NONE/START/MIDDLE/END of black key */
  guint bmatch : 1;	/* TRUE if on black key (differs from bstate!=NONE) */
  guint ces_fes : 1;	/* TRUE if on non-existant black key below C or F */
  guint valid : 1;	/* FALSE if min/max octave/semitone are exceeded */
  gint  valid_octave;
  guint valid_semitone;
} NoteInfo;

static gint
note_to_pixels (BstPianoRoll *self,
		gint	      note,
		gint	     *height_p,
		gint	     *ces_fes_height_p)
{
  gint octave, ythickness = 1, z = self->vzoom, h = NOTE_HEIGHT (self), semitone = SFI_NOTE_SEMITONE (note);
  gint oheight = OCTAVE_HEIGHT (self), y, zz = z + z, offs = 0, height = h;

  switch (semitone)
    {
    case 10:	offs += zz + h;
    case  8:	offs += zz + h;
    case  6:	offs += zz + h + zz + h;
    case  3:	offs += zz + h;
    case  1:	offs += z + h + (zz - h) / 2;
      break;
    case 11:	offs += h + zz;
    case  9:	offs += h + zz;
    case  7:	offs += h + zz;
    case  5:	offs += h + zz;
    case  4:	offs += h + zz;
    case  2:	offs += h + zz;
    case  0:	offs += z;
      break;
    }
  octave = N_OCTAVES (self) - 1 - SFI_NOTE_OCTAVE (note) + MIN_OCTAVE (self);
  y = octave * oheight;
  y += oheight - offs - h;

  /* spacing out by a bit looks nicer */
  if (z >= 4)
    {
      height += ythickness;
    }

  if (height_p)
    *height_p = height;
  if (ces_fes_height_p)
    *ces_fes_height_p = (semitone == 0 || semitone == 4 || semitone == 5 || semitone == 11) ? z : 0;

  return y;
}

static gint
note_to_coord (BstPianoRoll *self,
	       gint	     note,
	       gint	    *height_p,
	       gint	    *ces_fes_height_p)
{
  return note_to_pixels (self, note, height_p, ces_fes_height_p) - self->y_offset;
}

static gboolean
coord_to_note (BstPianoRoll *self,
	       gint          y,
	       NoteInfo	    *info)
{
  gint ythickness = 1, i, z = self->vzoom, h = NOTE_HEIGHT (self);
  gint end_shift, start_shift, black_shift = 0;
  gint oheight = OCTAVE_HEIGHT (self), kheight = 2 * z + h;

  y += self->y_offset;
  info->octave = y / oheight;
  i = y - info->octave * oheight;
  i = oheight - 1 - i;		/* octave increases with decreasing y */
  info->key = i / kheight;
  info->key_frac = i - info->key * kheight;
  i = info->key_frac;
  info->octave = N_OCTAVES (self) - 1 - info->octave + MIN_OCTAVE (self);

  /* pixel layout and note numbers:
   * Iz|h|zIz|h|zIz|h|zIz|h|zIz|h|zIz|h|zIz|h|zI
   * I 0 |#1#|2|#3#|4  I  5|#6#|7|#8#|9|#10|11 I
   * I   |###| |###|   I   |###| |###| |###|   I
   * I   +-+-+ +-+-+   I   +-+-+ +-+-+ +-+-+   I
   * I     I     I     I     I     I     I     I
   * +--0--+--1--+--2--+--3--+--4--+--5--+--6--+
   */

  /* figure black notes */
  end_shift = i >= z + h;
  start_shift = i < z; /* + ythickness; */
  info->semitone = 0;
  info->ces_fes = ((info->key == 0 && start_shift) ||
		   (info->key == 2 && end_shift) ||
		   (info->key == 3 && start_shift) ||
		   (info->key == 6 && end_shift));
  switch (info->key)
    {
    case 3:	info->semitone += 5;
    case 0:
      info->semitone += 0 + end_shift;
      black_shift = end_shift;
      break;
    case 5:	info->semitone += 2;
    case 4:	info->semitone += 5;
    case 1:
      info->semitone += 2 + (start_shift ? -1 : end_shift);
      black_shift = start_shift || end_shift;
      break;
    case 6:	info->semitone += 7;
    case 2:
      info->semitone += 4 - start_shift;
      black_shift = start_shift;
      break;
    }

  /* figure draw states */
  if (i < ythickness)
    info->wstate = DRAW_START;
  else if (i < kheight - ythickness)
    info->wstate = DRAW_MIDDLE;
  else
    info->wstate = DRAW_END;
  if (!black_shift)
    info->bstate = DRAW_NONE;
  else if (i < z)
    info->bstate = DRAW_MIDDLE;
  else if (i < z + ythickness)
    info->bstate = DRAW_END;
  else if (i < z + h + ythickness)
    info->bstate = DRAW_START;
  else
    info->bstate = DRAW_MIDDLE;

  /* behaviour fixup, ignore black note borders */
  if (black_shift && info->bstate == DRAW_END)
    {
      info->bmatch = FALSE;
      info->semitone += 1;
    }
  else if (black_shift && info->bstate == DRAW_START)
    {
      info->bmatch = FALSE;
      info->semitone -= 1;
    }
  else
    info->bmatch = TRUE;

  /* validate note */
  if (y < 0 ||		/* we calc junk in this case, flag invalidity */
      info->octave > MAX_OCTAVE (self) ||
      (info->octave == MAX_OCTAVE (self) && info->semitone > MAX_SEMITONE (self)))
    {
      info->valid_octave = MAX_OCTAVE (self);
      info->valid_semitone = MAX_SEMITONE (self);
      info->valid = FALSE;
    }
  else if (info->octave < MIN_OCTAVE (self) ||
	   (info->octave == MIN_OCTAVE (self) && info->semitone < MIN_SEMITONE (self)))
    {
      info->valid_octave = MIN_OCTAVE (self);
      info->valid_semitone = MIN_SEMITONE (self);
      info->valid = FALSE;
    }
  else
    {
      info->valid_octave = info->octave;
      info->valid_semitone = info->semitone;
      info->valid = TRUE;
    }
  
  return info->bmatch != 0;
}

static void
bst_piano_roll_overlap_grow_vpanel_area (BstPianoRoll *self,
					 GdkRectangle *area)
{
  /* grow vpanel exposes by surrounding white keys */
  area->y -= OCTAVE_HEIGHT (self) / 7;			/* fudge 1 key upwards */
  area->height += OCTAVE_HEIGHT (self) / 7;
  area->height += OCTAVE_HEIGHT (self) / 7;		/* fudge 1 key downwards */
}

static void
bst_piano_roll_draw_vpanel (BstPianoRoll *self,
			    gint	  y,
			    gint	  ybound)
{
  GdkWindow *drawable = self->vpanel;
  GdkGC *black_gc = STYLE (self)->fg_gc[GTK_STATE_NORMAL];
  GdkGC *dark_gc = STYLE (self)->dark_gc[GTK_STATE_NORMAL];
  GdkGC *light_gc = STYLE (self)->light_gc[GTK_STATE_NORMAL];
  gint i, start_x = KEYBOARD_X (self), white_x = KEYBOARD_WIDTH (self) - 1, black_x = white_x * KEYBOARD_RATIO (self);

  y = MAX (y, 0);

  /* draw vertical frame lines */
  gdk_draw_line (drawable, dark_gc, start_x + white_x, y, start_x + white_x, ybound - 1);

  /* outer vpanel shadow */
  gtk_paint_shadow (STYLE (self), drawable,
                    STATE (self), GTK_SHADOW_OUT,
                    NULL, NULL, NULL,
                    0, -YTHICKNESS (self),
                    VPANEL_WIDTH (self), VPANEL_HEIGHT (self) + 2 * YTHICKNESS (self));

  /* kludge, draw keys into right-hand shadow to make it look less dark */
  white_x += 1;

  /* draw horizontal lines */
  for (i = y; i < ybound; i++)
    {
      gint x = black_x + 1;
      NoteInfo info;

      coord_to_note (self, i, &info);
      switch (info.bstate)
	{
	case DRAW_START:
	  gdk_draw_line (drawable, dark_gc, start_x + 0, i, start_x + black_x, i);
	  break;
	case DRAW_MIDDLE:
	  gdk_draw_line (drawable, black_gc, start_x + 0, i, start_x + black_x - 1, i);
	  gdk_draw_line (drawable, dark_gc, start_x + black_x, i, start_x + black_x, i);
	  break;
	case DRAW_END:
	  gdk_draw_line (drawable, light_gc, start_x + 0, i, start_x + black_x, i);
	  break;
	default:
	  x = 0;
	}
      switch (info.wstate)
	{
	case DRAW_START:
	  gdk_draw_line (drawable, dark_gc, start_x + x, i, start_x + white_x, i);
	  if (info.semitone == 0)	/* C */
	    {
	      gint pbheight, ypos, ythickness = 1, overlap = 1;
	      gint pbwidth = white_x - black_x + overlap;
	      GdkPixbuf *pixbuf;

	      pbheight = OCTAVE_HEIGHT (self) / 7;
	      pbwidth /= 2;
	      ypos = i - pbheight + ythickness;
	      pixbuf = bst_ascii_pixbuf_new ('C', pbwidth, pbheight);
	      gdk_pixbuf_render_to_drawable (pixbuf, drawable, light_gc, 0, 0,
					     start_x + black_x, ypos, -1, -1,
					     GDK_RGB_DITHER_MAX, 0, 0);
	      g_object_unref (pixbuf);
	      if (info.octave < 0)
		{
		  guint indent = pbwidth * 0.4;

		  /* render a minus '-' for negative octaves into the 'C' */
		  pixbuf = bst_ascii_pixbuf_new ('-', pbwidth - indent, pbheight - 1);
		  gdk_pixbuf_render_to_drawable (pixbuf, drawable, light_gc, 0, 0,
						 start_x + black_x + indent, ypos, -1, -1,
						 GDK_RGB_DITHER_MAX, 0, 0);
		  g_object_unref (pixbuf);
		}
	      pixbuf = bst_ascii_pixbuf_new (ABS (info.octave) + '0', pbwidth, pbheight);
	      gdk_pixbuf_render_to_drawable (pixbuf, drawable, light_gc, 0, 0,
					     start_x + black_x + pbwidth - overlap, ypos, -1, -1,
					     GDK_RGB_DITHER_MAX, 0, 0);
	      g_object_unref (pixbuf);
	    }
	  break;
	case DRAW_MIDDLE:
	  // gdk_draw_line (drawable, white_gc, start_x + x, i, start_x + white_x, i);
	  break;
	case DRAW_END:
	  gdk_draw_line (drawable, light_gc, start_x + x, i, start_x + white_x, i);
	  break;
	}
    }
}

static void
bst_piano_roll_draw_canvas (BstPianoRoll *self,
			    gint          x,
			    gint	  y,
			    gint          xbound,
			    gint          ybound)
{
  GdkWindow *window = self->canvas;
  GdkGC *light_gc, *dark_gc = STYLE (self)->dark_gc[GTK_STATE_NORMAL];
  gint i, dlen, line_width = 0; /* line widths != 0 interfere with dash-settings on some X servers */
  BsePartNoteSeq *pseq;

  /* draw selection */
  if (self->selection_duration)
    {
      gint x1, x2, y1, y2, height;

      x1 = tick_to_coord (self, self->selection_tick);
      x2 = tick_to_coord (self, self->selection_tick + self->selection_duration);
      y1 = note_to_coord (self, self->selection_max_note, &height, NULL);
      y2 = note_to_coord (self, self->selection_min_note, &height, NULL);
      y2 += height;
      gdk_draw_rectangle (window, GTK_WIDGET (self)->style->bg_gc[GTK_STATE_SELECTED], TRUE,
			  x1, y1, MAX (x2 - x1, 0), MAX (y2 - y1, 0));
    }

  /* draw horizontal grid lines */
  for (i = y; i < ybound; i++)
    {
      NoteInfo info;

      coord_to_note (self, i, &info);
      if (info.wstate != DRAW_START)
	continue;

      if (info.semitone == 0)	/* C */
	{
          GdkGC *draw_gc = self->color_gc[0 /* C */];
	  gdk_gc_set_line_attributes (draw_gc, line_width, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
	  gdk_draw_line (window, draw_gc, x, i, xbound - 1, i);
	}
      else if (info.semitone == 5) /* F */
	{
          GdkGC *draw_gc = self->color_gc[6 /* F */];
	  guint8 dash[3] = { 2, 2, 0 };
	  
	  gdk_gc_set_line_attributes (draw_gc, line_width, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
	  dlen = dash[0] + dash[1];
	  gdk_gc_set_dashes (draw_gc, (self->x_offset + x + 1) % dlen, dash, 2);
	  gdk_draw_line (window, draw_gc, x, i, xbound - 1, i);
	  gdk_gc_set_line_attributes (draw_gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
	}
      else
	{
	  guint8 dash[3] = { 1, 1, 0 };
	  
	  gdk_gc_set_line_attributes (dark_gc, line_width, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
	  dlen = dash[0] + dash[1];
	  gdk_gc_set_dashes (dark_gc, (self->x_offset + x + 1) % dlen, dash, 2);
	  gdk_draw_line (window, dark_gc, x, i, xbound - 1, i);
	  gdk_gc_set_line_attributes (dark_gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
	}
    }

  /* draw vertical grid lines */
  for (i = x; i < xbound; i++)
    {
      if (coord_check_crossing (self, i, CROSSING_TACT))
	{
	  gdk_gc_set_line_attributes (dark_gc, line_width, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
	  gdk_draw_line (window, dark_gc, i, y, i, ybound - 1);
	}
      else if (self->draw_qn_grid && coord_check_crossing (self, i, CROSSING_QNOTE))
	{
	  guint8 dash[3] = { 2, 2, 0 };

	  gdk_gc_set_line_attributes (dark_gc, line_width, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
	  dlen = dash[0] + dash[1];
	  gdk_gc_set_dashes (dark_gc, (self->y_offset + y + 1) % dlen, dash, 2);
	  gdk_draw_line (window, dark_gc, i, y, i, ybound - 1);
	  gdk_gc_set_line_attributes (dark_gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
	}
      else if (self->draw_qqn_grid && coord_check_crossing (self, i, CROSSING_QNOTE_Q))
	{
	  guint8 dash[3] = { 1, 1, 0 };

	  gdk_gc_set_line_attributes (dark_gc, line_width, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
	  dlen = dash[0] + dash[1];
	  gdk_gc_set_dashes (dark_gc, (self->y_offset + y + 1) % dlen, dash, 2);
	  gdk_draw_line (window, dark_gc, i, y, i, ybound - 1);
	  gdk_gc_set_line_attributes (dark_gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
	}
    }

  /* draw notes */
  light_gc = STYLE (self)->light_gc[GTK_STATE_NORMAL];
  dark_gc = STYLE (self)->dark_gc[GTK_STATE_NORMAL];
  pseq = self->proxy ? bse_part_list_notes_crossing (self->proxy,
						     coord_to_tick (self, x, FALSE),
						     coord_to_tick (self, xbound, FALSE)) : NULL;
  for (i = 0; pseq && i < pseq->n_pnotes; i++)
    {
      BsePartNote *pnote = pseq->pnotes[i];
      gint semitone = SFI_NOTE_SEMITONE (pnote->note);
      guint start = pnote->tick, end = start + pnote->duration;
      GdkGC *xdark_gc, *xlight_gc, *xnote_gc;
      gint x1, x2, y1, y2, height;
      gboolean selected = pnote->selected;

      selected |= (pnote->tick >= self->selection_tick &&
		   pnote->tick < self->selection_tick + self->selection_duration &&
		   pnote->note >= self->selection_min_note &&
		   pnote->note <= self->selection_max_note);
      if (selected)
	{
	  xdark_gc = STYLE (self)->bg_gc[GTK_STATE_SELECTED];
	  xnote_gc = STYLE (self)->fg_gc[GTK_STATE_SELECTED];
	  xlight_gc = STYLE (self)->bg_gc[GTK_STATE_SELECTED];
	}
      else
	{
	  xdark_gc = STYLE (self)->black_gc;
	  xnote_gc = self->color_gc[semitone];
	  xlight_gc = dark_gc;
	}
      x1 = tick_to_coord (self, start);
      x2 = tick_to_coord (self, end);

      y1 = note_to_coord (self, pnote->note, &height, NULL);
      y2 = y1 + height - 1;
      gdk_draw_line (window, xdark_gc, x1, y2, x2, y2);
      gdk_draw_line (window, xdark_gc, x2, y1, x2, y2);
      gdk_draw_rectangle (window, xnote_gc, TRUE, x1, y1, MAX (x2 - x1, 1), MAX (y2 - y1, 1));
      if (y2 - y1 >= 3)	/* work for zoom to micro size */
	{
	  if (xlight_gc)
	    {
	      gdk_draw_line (window, xlight_gc, x1, y1, x2, y1);
	      gdk_draw_line (window, xlight_gc, x1, y1, x1, y2);
	    }
	}
    }
}

static void
bst_piano_roll_overlap_grow_hpanel_area (BstPianoRoll *self,
					 GdkRectangle *area)
{
  gint i, x = area->x, xbound = x + area->width;

  /* grow hpanel exposes by surrounding tacts */
  i = coord_to_tick (self, x, FALSE);
  i /= self->ppqn * self->qnpt;
  if (i > 0)
    i -= 1;		/* fudge 1 tact to the left */
  i *= self->ppqn * self->qnpt;
  x = tick_to_coord (self, i);
  i = coord_to_tick (self, xbound + 1, TRUE);
  i /= self->ppqn * self->qnpt;
  i += 1;		/* fudge 1 tact to the right */
  i *= self->ppqn * self->qnpt;
  xbound = tick_to_coord (self, i);

  area->x = x;
  area->width = xbound - area->x;
}

static void
bst_piano_roll_draw_hpanel (BstPianoRoll *self,
			    gint	  x,
			    gint	  xbound)
{
  GdkWindow *drawable = self->hpanel;
  GdkFont *font = gtk_style_get_font (STYLE (self));
  GdkGC *fg_gc = STYLE (self)->fg_gc[STATE (self)];
  gint text_y = HPANEL_HEIGHT (self) - (HPANEL_HEIGHT (self) - gdk_string_height (font, "0123456789:")) / 2;
  gchar buffer[64];
  gint i;

  /* draw tact/note numbers */
  for (i = x; i < xbound; i++)
    {
      guint next_pixel, width;

      /* drawing qnote numbers is not of much use if we can't even draw
       * the qnote quarter grid, so we special case draw_qqn_grid here
       */

      if (coord_check_crossing (self, i, CROSSING_TACT))
	{
	  guint tact = coord_to_tick (self, i, TRUE) + 1;

	  tact /= (self->ppqn * self->qnpt);
	  next_pixel = tick_to_coord (self, (tact + 1) * (self->ppqn * self->qnpt));

	  g_snprintf (buffer, 64, "%u", tact + 1);

	  /* draw this tact if there's enough space */
	  width = gdk_string_width (font, buffer);
	  if (i + width / 2 < (i + next_pixel) / 2)
	    gdk_draw_string (self->hpanel, font, fg_gc,
			     i - width / 2, text_y,
			     buffer);
	}
      else if (self->draw_qqn_grid && coord_check_crossing (self, i, CROSSING_QNOTE))
	{
          guint tact = coord_to_tick (self, i, TRUE) + 1, qn = tact;

	  tact /= (self->ppqn * self->qnpt);
	  qn /= self->ppqn;
	  next_pixel = tick_to_coord (self, (qn + 1) * self->ppqn);

	  g_snprintf (buffer, 64, ":%u", /* tact + 1, */ qn % self->qnpt + 1);

	  /* draw this tact if there's enough space */
	  width = gdk_string_width (font, buffer);
	  if (i + width < (i + next_pixel) / 2)		/* don't half width, leave some more space */
	    gdk_draw_string (self->hpanel, font, fg_gc,
			     i - width / 2, text_y,
			     buffer);
	}
    }

  /* draw outer hpanel shadow */
  gtk_paint_shadow (STYLE (self), drawable,
                    STATE (self), GTK_SHADOW_OUT,
                    NULL, NULL, NULL,
                    -XTHICKNESS (self), 0,
                    HPANEL_WIDTH (self) + 2 * XTHICKNESS (self), HPANEL_HEIGHT (self));
}

static void
bst_piano_roll_draw_window (BstPianoRoll *self,
                            gint          x,
                            gint          y,
                            gint          xbound,
                            gint          ybound)
{
  GdkWindow *drawable = GTK_WIDGET (self)->window;
  GtkWidget *widget = GTK_WIDGET (self);
  GdkGC *bg_gc = STYLE (self)->bg_gc[GTK_WIDGET_STATE (self)];
  gint width = HPANEL_X (self);
  gint height = VPANEL_Y (self);

  gdk_draw_rectangle (drawable, bg_gc, TRUE,
                      0, 0, width, height);

  /* draw hpanel and vpanel scrolling boundaries */
  gtk_paint_shadow (widget->style, drawable,
                    widget->state, GTK_SHADOW_ETCHED_IN,
                    NULL, NULL, NULL,
                    - XTHICKNESS (self), - YTHICKNESS (self),
                    width + XTHICKNESS (self), height + YTHICKNESS (self));
  /* draw outer scrollpanel shadow */
  gtk_paint_shadow (widget->style, drawable,
                    widget->state, GTK_SHADOW_OUT,
                    NULL, NULL, NULL,
                    0, 0, width + XTHICKNESS (self), height + YTHICKNESS (self));
  /* draw inner scrollpanel corner */
  gtk_paint_shadow (widget->style, drawable,
                    widget->state, GTK_SHADOW_IN,
                    NULL, NULL, NULL,
                    width - XTHICKNESS (self), height - YTHICKNESS (self),
                    2 * XTHICKNESS (self), 2 * YTHICKNESS (self));
}

static gboolean
bst_piano_roll_expose (GtkWidget      *widget,
		       GdkEventExpose *event)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);
  GdkRectangle area = event->area;
  
  /* with gtk_widget_set_double_buffered (self, FALSE) in init and
   * with gdk_window_begin_paint_region()/gdk_window_end_paint()
   * around our redraw functions, we can decide on our own on what
   * windows we want double buffering.
   */
  if (event->window == widget->window)
    {
      gdk_window_begin_paint_rect (event->window, &area);
      bst_piano_roll_draw_window (self, area.x, area.y, area.x + area.width, area.y + area.height);
      gdk_window_end_paint (event->window);
    }
  else if (event->window == self->vpanel)
    {
      bst_piano_roll_overlap_grow_vpanel_area (self, &area);
      gdk_window_begin_paint_rect (event->window, &area);
      bst_piano_roll_overlap_grow_vpanel_area (self, &area);
      bst_piano_roll_draw_vpanel (self, area.y, area.y + area.height);
      gdk_window_end_paint (event->window);
    }
  else if (event->window == self->hpanel)
    {
      bst_piano_roll_overlap_grow_hpanel_area (self, &area);
      gdk_window_begin_paint_rect (event->window, &area);
      bst_piano_roll_overlap_grow_hpanel_area (self, &area);
      bst_piano_roll_draw_hpanel (self, area.x, area.x + area.width);
      gdk_window_end_paint (event->window);
    }
  else if (event->window == self->canvas)
    {
      gdk_window_begin_paint_rect (event->window, &area);
      // gdk_window_clear_area (widget->window, area.x, area.y, area.width, area.height);
      bst_piano_roll_draw_canvas (self, area.x, area.y, area.x + area.width, area.y + area.height);
      gdk_window_end_paint (event->window);
    }
  return FALSE;
}

static void
piano_roll_queue_expose (BstPianoRoll *self,
			 GdkWindow    *window,
			 guint	       note,
			 guint	       tick_start,
			 guint	       tick_end)
{
  gint x1 = tick_to_coord (self, tick_start);
  gint x2 = tick_to_coord (self, tick_end);
  gint height, cfheight, y1 = note_to_coord (self, note, &height, &cfheight);
  GdkRectangle area;

  area.x = x1 - 3;		/* add fudge */
  area.y = y1 - cfheight;
  area.width = x2 - x1 + 3 + 3;	/* add fudge */
  area.height = height + 2 * cfheight;
  if (window == self->vpanel)
    {
      area.x = 0;
      area.width = VPANEL_WIDTH (self);
    }
  else if (window == self->hpanel)
    {
      area.y = 0;
      area.height = HPANEL_HEIGHT (self);
    }
  gdk_window_invalidate_rect (window, &area, TRUE);
}

static void
piano_roll_adjustment_changed (BstPianoRoll *self)
{
}

static void
piano_roll_adjustment_value_changed (BstPianoRoll  *self,
				     GtkAdjustment *adjustment)
{
  if (adjustment == self->hadjustment)
    {
      gint x = self->x_offset, diff;

      self->x_offset = ticks_to_pixels (self, adjustment->value);
      diff = x - self->x_offset;
      if (diff && GTK_WIDGET_DRAWABLE (self))
	{
	  GdkRectangle area = { 0, };

	  gdk_window_scroll (self->hpanel, diff, 0);
	  gdk_window_scroll (self->canvas, diff, 0);
	  area.x = diff < 0 ? CANVAS_WIDTH (self) + diff : 0;
	  area.y = 0;
	  area.width = ABS (diff);
	  area.height = CANVAS_HEIGHT (self);
	  gdk_window_invalidate_rect (self->canvas, &area, TRUE);
	  area.x = diff < 0 ? HPANEL_WIDTH (self) + diff : 0;
	  area.y = 0;
	  area.width = ABS (diff);
	  area.height = HPANEL_HEIGHT (self);
	  gdk_window_invalidate_rect (self->hpanel, &area, TRUE);
	}
    }
  if (adjustment == self->vadjustment)
    {
      gint y = self->y_offset, diff;

      self->y_offset = adjustment->value;
      diff = y - self->y_offset;
      if (diff && GTK_WIDGET_DRAWABLE (self))
	{
	  GdkRectangle area = { 0, };

	  gdk_window_scroll (self->vpanel, 0, diff);
	  gdk_window_scroll (self->canvas, 0, diff);
	  area.x = 0;
	  area.y = diff < 0 ? VPANEL_HEIGHT (self) + diff : 0;
	  area.width = VPANEL_WIDTH (self);
	  area.height = ABS (diff);
	  gdk_window_invalidate_rect (self->vpanel, &area, TRUE);
	  area.x = 0;
	  area.y = diff < 0 ? CANVAS_HEIGHT (self) + diff : 0;
	  area.width = CANVAS_WIDTH (self);
	  area.height = ABS (diff);
	  gdk_window_invalidate_rect (self->canvas, &area, TRUE);
	}
    }
}

static void
piano_roll_update_adjustments (BstPianoRoll *self,
			       gboolean      hadj,
			       gboolean      vadj)
{
  gdouble hv = self->hadjustment->value;
  gdouble vv = self->vadjustment->value;

  if (hadj)
    {
      self->hadjustment->lower = 0;
      self->hadjustment->upper = self->max_ticks;
      self->hadjustment->page_size = pixels_to_ticks (self, CANVAS_WIDTH (self));
      self->hadjustment->step_increment = self->ppqn;
      self->hadjustment->page_increment = self->ppqn * self->qnpt;
      self->hadjustment->value = CLAMP (self->hadjustment->value,
					self->hadjustment->lower,
					self->hadjustment->upper - self->hadjustment->page_size);
      gtk_adjustment_changed (self->hadjustment);
    }
  if (vadj)
    {
      self->vadjustment->lower = 0;
      self->vadjustment->upper = OCTAVE_HEIGHT (self) * N_OCTAVES (self);
      self->vadjustment->page_size = CANVAS_HEIGHT (self);
      self->vadjustment->page_increment = self->vadjustment->page_size / 2;
      self->vadjustment->step_increment = OCTAVE_HEIGHT (self) / 7;
      if (self->init_vpos && self->proxy)
	{
	  self->init_vpos = FALSE;
	  self->vadjustment->value = (self->vadjustment->upper -
				      self->vadjustment->lower -
				      self->vadjustment->page_size) / 2;
	}
      self->vadjustment->value = CLAMP (self->vadjustment->value,
					self->vadjustment->lower,
					self->vadjustment->upper - self->vadjustment->page_size);
      gtk_adjustment_changed (self->vadjustment);
    }
  if (hv != self->hadjustment->value)
    gtk_adjustment_value_changed (self->hadjustment);
  if (vv != self->vadjustment->value)
    gtk_adjustment_value_changed (self->vadjustment);
}

static void
piano_roll_scroll_adjustments (BstPianoRoll *self,
			       gint          x_pixel,
			       gint          y_pixel)
{
  gdouble hv = self->hadjustment->value;
  gdouble vv = self->vadjustment->value;
  gint xdiff, ydiff;
  gint ticks;

  xdiff = x_pixel * AUTO_SCROLL_SCALE;
  ydiff = y_pixel * AUTO_SCROLL_SCALE;

  ticks = pixels_to_ticks (self, ABS (xdiff));
  if (x_pixel > 0)
    ticks = MAX (ticks, 1);
  else if (x_pixel < 0)
    ticks = MIN (-1, -ticks);
  self->hadjustment->value += ticks;
  self->hadjustment->value = CLAMP (self->hadjustment->value,
				    self->hadjustment->lower,
				    self->hadjustment->upper - self->hadjustment->page_size);
  if (y_pixel > 0)
    ydiff = MAX (ydiff, 1);
  else if (y_pixel < 0)
    ydiff = MIN (-1, ydiff);
  self->vadjustment->value += ydiff;
  self->vadjustment->value = CLAMP (self->vadjustment->value,
				    self->vadjustment->lower,
				    self->vadjustment->upper - self->vadjustment->page_size);
  if (hv != self->hadjustment->value)
    gtk_adjustment_value_changed (self->hadjustment);
  if (vv != self->vadjustment->value)
    gtk_adjustment_value_changed (self->vadjustment);
}

static void
bst_piano_roll_hsetup (BstPianoRoll *self,
		       guint	     ppqn,
		       guint	     qnpt,
		       guint	     max_ticks,
		       gfloat	     hzoom)
{
  guint old_ppqn = self->ppqn;
  guint old_qnpt = self->qnpt;
  guint old_max_ticks = self->max_ticks;
  gfloat old_hzoom = self->hzoom;

  /* here, we setup all things necessary to determine our
   * horizontal layout. we have to avoid resizes at
   * least if just max_ticks changes, since the tick range
   * might need to grow during pointer grabs
   */
  self->ppqn = MAX (ppqn, 1);
  self->qnpt = CLAMP (qnpt, 3, 4);
  self->max_ticks = MAX (max_ticks, 1);
  self->hzoom = CLAMP (hzoom, 0.01, 100);

  if (old_ppqn != self->ppqn ||
      old_qnpt != self->qnpt ||
      old_max_ticks != self->max_ticks ||	// FIXME: shouldn't always cause a redraw
      old_hzoom != self->hzoom)
    {
      if (self->hadjustment)
	{
	  self->x_offset = ticks_to_pixels (self, self->hadjustment->value);
	  piano_roll_update_adjustments (self, TRUE, FALSE);
	}
      self->draw_qn_grid = ticks_to_pixels (self, self->ppqn) >= 3;
      self->draw_qqn_grid = ticks_to_pixels (self, self->ppqn / 4) >= 5;
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

gfloat
bst_piano_roll_set_hzoom (BstPianoRoll *self,
			  gfloat        hzoom)
{
  g_return_val_if_fail (BST_IS_PIANO_ROLL (self), 0);

  bst_piano_roll_hsetup (self, self->ppqn, self->qnpt, self->max_ticks, hzoom);

  return self->hzoom;
}

static gboolean
bst_piano_roll_focus_in (GtkWidget     *widget,
			 GdkEventFocus *event)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);

  GTK_WIDGET_SET_FLAGS (self, GTK_HAS_FOCUS);
  // bst_piano_roll_draw_focus (self);
  return TRUE;
}

static gboolean
bst_piano_roll_focus_out (GtkWidget	*widget,
			  GdkEventFocus *event)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);

  GTK_WIDGET_UNSET_FLAGS (self, GTK_HAS_FOCUS);
  // bst_piano_roll_draw_focus (self);
  return TRUE;
}

static void
bst_piano_roll_canvas_drag_abort (BstPianoRoll *self)
{
  if (self->canvas_drag)
    {
      self->canvas_drag = FALSE;
      self->drag.type = BST_DRAG_ABORT;
      g_signal_emit (self, signal_canvas_drag, 0, &self->drag);
    }
}

static gboolean
bst_piano_roll_canvas_drag (BstPianoRoll *self,
			    gint	  coord_x,
			    gint	  coord_y,
			    gboolean      initial)
{
  if (self->canvas_drag)
    {
      NoteInfo info;
      
      self->drag.current_tick = coord_to_tick (self, MAX (coord_x, 0), FALSE);
      coord_to_note (self, MAX (coord_y, 0), &info);
      self->drag.current_note = SFI_NOTE_GENERIC (info.valid_octave, info.valid_semitone);
      self->drag.current_valid = info.valid && !info.ces_fes;
      if (initial)
	{
	  self->drag.start_tick = self->drag.current_tick;
	  self->drag.start_note = self->drag.current_note;
	  self->drag.start_valid = self->drag.current_valid;
	}
      g_signal_emit (self, signal_canvas_drag, 0, &self->drag);
      if (self->drag.state == BST_DRAG_HANDLED)
	self->canvas_drag = FALSE;
      else if (self->drag.state == BST_DRAG_ERROR)
	bst_piano_roll_canvas_drag_abort (self);
      else if (initial && self->drag.state == BST_DRAG_UNHANDLED)
	return TRUE;
    }
  return FALSE;
}

static void
bst_piano_roll_piano_drag_abort (BstPianoRoll *self)
{
  if (self->piano_drag)
    {
      self->piano_drag = FALSE;
      self->drag.type = BST_DRAG_ABORT;
      g_signal_emit (self, signal_piano_drag, 0, &self->drag);
    }
}

static gboolean
bst_piano_roll_piano_drag (BstPianoRoll *self,
			   gint	         coord_x,
			   gint	         coord_y,
			   gboolean      initial)
{
  if (self->piano_drag)
    {
      NoteInfo info;
      
      self->drag.current_tick = 0;
      coord_to_note (self, MAX (coord_y, 0), &info);
      self->drag.current_note = SFI_NOTE_GENERIC (info.valid_octave, info.valid_semitone);
      self->drag.current_valid = info.valid && !info.ces_fes;
      if (initial)
	{
	  self->drag.start_tick = self->drag.current_tick;
	  self->drag.start_note = self->drag.current_note;
	  self->drag.start_valid = self->drag.current_valid;
	}
      g_signal_emit (self, signal_piano_drag, 0, &self->drag);
      if (self->drag.state == BST_DRAG_HANDLED)
	self->piano_drag = FALSE;
      else if (self->drag.state == BST_DRAG_ERROR)
	bst_piano_roll_piano_drag_abort (self);
      else if (initial && self->drag.state == BST_DRAG_UNHANDLED)
	return TRUE;
    }
  return FALSE;
}

static gboolean
bst_piano_roll_button_press (GtkWidget	    *widget,
			     GdkEventButton *event)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);
  gboolean handled = FALSE;
  
  if (!GTK_WIDGET_HAS_FOCUS (widget))
    gtk_widget_grab_focus (widget);

  if (event->window == self->canvas && !self->canvas_drag)
    {
      handled = TRUE;
      self->drag.proll = self;
      self->drag.type = BST_DRAG_START;
      self->drag.mode = bst_drag_modifier_start (event->state);
      self->drag.button = event->button;
      self->drag.state = BST_DRAG_UNHANDLED;
      self->canvas_drag = TRUE;
      if (bst_piano_roll_canvas_drag (self, event->x, event->y, TRUE) == TRUE)
	{
	  self->canvas_drag = FALSE;
	  g_signal_emit (self, signal_canvas_clicked, 0, self->drag.button, self->drag.start_tick, self->drag.start_note, event);
	}
    }
  else if (event->window == self->vpanel && !self->piano_drag)
    {
      handled = TRUE;
      self->drag.proll = self;
      self->drag.type = BST_DRAG_START;
      self->drag.mode = bst_drag_modifier_start (event->state);
      self->drag.button = event->button;
      self->drag.state = BST_DRAG_UNHANDLED;
      self->piano_drag = TRUE;
      if (bst_piano_roll_piano_drag (self, event->x, event->y, TRUE) == TRUE)
	{
	  self->piano_drag = FALSE;
	  g_signal_emit (self, signal_piano_clicked, 0, self->drag.button, self->drag.start_tick, self->drag.start_note, event);
	}
    }

  return handled;
}

static gboolean
timeout_scroller (gpointer data)
{
  BstPianoRoll *self;
  guint remain = 1;

  GDK_THREADS_ENTER ();
  self = BST_PIANO_ROLL (data);
  if (self->canvas_drag && GTK_WIDGET_DRAWABLE (self))
    {
      gint x, y, width, height, xdiff = 0, ydiff = 0;
      GdkModifierType modifiers;

      gdk_window_get_size (self->canvas, &width, &height);
      gdk_window_get_pointer (self->canvas, &x, &y, &modifiers);
      if (x < 0)
	xdiff = x;
      else if (x >= width)
	xdiff = x - width + 1;
      if (y < 0)
	ydiff = y;
      else if (y >= height)
	ydiff = y - height + 1;
      if (xdiff || ydiff)
	{
	  piano_roll_scroll_adjustments (self, xdiff, ydiff);
	  self->drag.type = BST_DRAG_MOTION;
	  self->drag.mode = bst_drag_modifier_next (modifiers, self->drag.mode);
	  bst_piano_roll_canvas_drag (self, x, y, FALSE);
	}
      else
	self->scroll_timer = remain = 0;
    }
  else if (self->piano_drag && GTK_WIDGET_DRAWABLE (self))
    {
      gint x, y, height, ydiff = 0;
      GdkModifierType modifiers;

      gdk_window_get_size (self->vpanel, NULL, &height);
      gdk_window_get_pointer (self->vpanel, &x, &y, &modifiers);
      if (y < 0)
	ydiff = y;
      else if (y >= height)
	ydiff = y - height + 1;
      if (ydiff)
	{
	  piano_roll_scroll_adjustments (self, 0, ydiff);
	  self->drag.type = BST_DRAG_MOTION;
	  self->drag.mode = bst_drag_modifier_next (modifiers, self->drag.mode);
	  bst_piano_roll_piano_drag (self, x, y, FALSE);
	}
      else
	self->scroll_timer = remain = 0;
    }
  else
    self->scroll_timer = remain = 0;
  GDK_THREADS_LEAVE ();

  return remain;
}

static gboolean
bst_piano_roll_motion (GtkWidget      *widget,
		       GdkEventMotion *event)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);
  gboolean handled = FALSE;

  /* FIXME:
     optimize part for non-L shaped seletion updates
     fix tick boundary ;(
  */
  
  if (event->window == self->canvas && self->canvas_drag)
    {
      gint width, height;
      handled = TRUE;
      self->drag.type = BST_DRAG_MOTION;
      self->drag.mode = bst_drag_modifier_next (event->state, self->drag.mode);
      bst_piano_roll_canvas_drag (self, event->x, event->y, FALSE);
      gdk_window_get_size (self->canvas, &width, &height);
      if (!self->scroll_timer && (event->x < 0 || event->x >= width ||
				  event->y < 0 || event->y >= height))
	self->scroll_timer = g_timeout_add_full (G_PRIORITY_DEFAULT,
						 AUTO_SCROLL_TIMEOUT,
						 timeout_scroller,
						 self, NULL);
      /* trigger motion events (since we use motion-hint) */
      gdk_window_get_pointer (self->canvas, NULL, NULL, NULL);
    }
  if (event->window == self->vpanel && self->piano_drag)
    {
      gint height;
      handled = TRUE;
      self->drag.type = BST_DRAG_MOTION;
      self->drag.mode = bst_drag_modifier_next (event->state, self->drag.mode);
      bst_piano_roll_piano_drag (self, event->x, event->y, FALSE);
      gdk_window_get_size (self->vpanel, NULL, &height);
      if (!self->scroll_timer && (event->y < 0 || event->y >= height))
	self->scroll_timer = g_timeout_add_full (G_PRIORITY_DEFAULT,
						 AUTO_SCROLL_TIMEOUT,
						 timeout_scroller,
						 self, NULL);
      /* trigger motion events (since we use motion-hint) */
      gdk_window_get_pointer (self->vpanel, NULL, NULL, NULL);
    }

  return handled;
}

static gboolean
bst_piano_roll_button_release (GtkWidget      *widget,
			       GdkEventButton *event)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);
  gboolean handled = FALSE;

  if (event->window == self->canvas && self->canvas_drag && event->button == self->drag.button)
    {
      handled = TRUE;
      self->drag.type = BST_DRAG_DONE;
      self->drag.mode = bst_drag_modifier_next (event->state, self->drag.mode);
      bst_piano_roll_canvas_drag (self, event->x, event->y, FALSE);
      self->canvas_drag = FALSE;
    }
  else if (event->window == self->vpanel && self->piano_drag && event->button == self->drag.button)
    {
      handled = TRUE;
      self->drag.type = BST_DRAG_DONE;
      self->drag.mode = bst_drag_modifier_next (event->state, self->drag.mode);
      bst_piano_roll_piano_drag (self, event->x, event->y, FALSE);
      self->piano_drag = FALSE;
    }

  return handled;
}

static gboolean
bst_piano_roll_key_press (GtkWidget   *widget,
			  GdkEventKey *event)
{
  BstPianoRoll *self = BST_PIANO_ROLL (widget);
  gboolean handled = FALSE;

  if (event->keyval == GDK_Escape)
    {
      bst_piano_roll_canvas_drag_abort (self);
      bst_piano_roll_piano_drag_abort (self);
      handled = TRUE;
    }
  return handled;
}

static gboolean
bst_piano_roll_key_release (GtkWidget   *widget,
			    GdkEventKey *event)
{
  // BstPianoRoll *self = BST_PIANO_ROLL (widget);
  gboolean handled = FALSE;

  return handled;
}

static void
piano_roll_update (BstPianoRoll *self,
		   guint         tick,
		   guint         duration,
		   gint          min_note,
		   gint          max_note)
{
  gint note;

  duration = MAX (duration, 1);
  for (note = min_note; note <= max_note; note++)
    piano_roll_queue_expose (self, self->canvas, note, tick, tick + duration - 1);
}

static void
piano_roll_release_proxy (BstPianoRoll *self)
{
  gxk_toplevel_delete (GTK_WIDGET (self));
  bst_piano_roll_set_proxy (self, 0);
}

void
bst_piano_roll_set_proxy (BstPianoRoll *self,
			  SfiProxy      proxy)
{
  g_return_if_fail (BST_IS_PIANO_ROLL (self));
  if (proxy)
    {
      g_return_if_fail (BSE_IS_ITEM (proxy));
      g_return_if_fail (bse_item_get_project (proxy) != 0);
    }

  if (self->proxy)
    {
      bse_proxy_disconnect (self->proxy,
			    "any_signal", piano_roll_release_proxy, self,
			    "any_signal", piano_roll_update, self,
			    NULL);
      bse_item_unuse (self->proxy);
    }
  self->proxy = proxy;
  if (self->proxy)
    {
      bse_item_use (self->proxy);
      bse_proxy_connect (self->proxy,
			 "swapped_signal::release", piano_roll_release_proxy, self,
			 // "swapped_signal::property-notify::uname", piano_roll_update_name, self,
			 "swapped_signal::range-changed", piano_roll_update, self,
			 NULL);
      self->min_note = bse_part_get_min_note (self->proxy);
      self->max_note = bse_part_get_max_note (self->proxy);
      self->max_ticks = bse_part_get_max_tick (self->proxy);
      bst_piano_roll_hsetup (self, self->ppqn, self->qnpt, self->max_ticks, self->hzoom);
    }
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
piano_roll_queue_region (BstPianoRoll *self,
			 guint         tick,
			 guint         duration,
			 gint          min_note,
			 gint          max_note)
{
  if (self->proxy && duration)	/* let the part extend the area by spanning notes if necessary */
    bse_part_queue_notes (self->proxy, tick, duration, min_note, max_note);
  piano_roll_update (self, tick, duration, min_note, max_note);
}

void
bst_piano_roll_set_view_selection (BstPianoRoll *self,
				   guint         tick,
				   guint         duration,
				   gint          min_note,
				   gint          max_note)
{
  g_return_if_fail (BST_IS_PIANO_ROLL (self));
  
  if (min_note > max_note || !duration)	/* invalid selection */
    {
      tick = 0;
      duration = 0;
      min_note = 0;
      max_note = 0;
    }
  
  if (self->selection_duration && duration)
    {
      /* if at least one corner of the old an the new selection
       * matches, it's probably worth updating only diff-regions
       */
      if ((tick == self->selection_tick ||
	   tick + duration == self->selection_tick + self->selection_duration) &&
	  (min_note == self->selection_min_note ||
	   max_note == self->selection_max_note))
	{
	  guint start, end;
	  gint note_min, note_max;
	  /* difference on the left */
	  start = MIN (tick, self->selection_tick);
	  end = MAX (tick, self->selection_tick);
	  if (end != start)
	    piano_roll_queue_region (self, start, end - start,
				     MIN (min_note, self->selection_min_note),
				     MAX (max_note, self->selection_max_note));
	  /* difference on the right */
	  start = MIN (tick + duration, self->selection_tick + self->selection_duration);
	  end = MAX (tick + duration, self->selection_tick + self->selection_duration);
	  if (end != start)
	    piano_roll_queue_region (self, start, end - start,
				     MIN (min_note, self->selection_min_note),
				     MAX (max_note, self->selection_max_note));
	  start = MIN (tick, self->selection_tick);
	  end = MAX (tick + duration, self->selection_tick + self->selection_duration);
	  /* difference on the top */
	  note_max = MAX (max_note, self->selection_max_note);
	  note_min = MIN (max_note, self->selection_max_note);
	  if (note_max != note_min)
	    piano_roll_queue_region (self, start, end - start, note_min, note_max);
	  /* difference on the bottom */
	  note_max = MAX (min_note, self->selection_min_note);
	  note_min = MIN (min_note, self->selection_min_note);
	  if (note_max != note_min)
	    piano_roll_queue_region (self, start, end - start, note_min, note_max);
	}
      else
	{
	  /* simply update new and old selection */
	  piano_roll_queue_region (self, self->selection_tick, self->selection_duration,
				   self->selection_min_note, self->selection_max_note);
	  piano_roll_queue_region (self, tick, duration, min_note, max_note);
	}
    }
  else if (self->selection_duration)
    piano_roll_queue_region (self, self->selection_tick, self->selection_duration,
			     self->selection_min_note, self->selection_max_note);
  else /* duration != 0 */
    piano_roll_queue_region (self, tick, duration, min_note, max_note);
  self->selection_tick = tick;
  self->selection_duration = duration;
  self->selection_min_note = min_note;
  self->selection_max_note = max_note;
}

gint
bst_piano_roll_get_vpanel_width (BstPianoRoll *self)
{
  g_return_val_if_fail (BST_IS_PIANO_ROLL (self), 0);

  return VPANEL_WIDTH (self);
}

void
bst_piano_roll_get_paste_pos (BstPianoRoll *self,
			      guint        *tick_p,
			      gint         *note_p)
{
  guint tick, semitone;
  gint octave;
  
  g_return_if_fail (BST_IS_PIANO_ROLL (self));

  if (GTK_WIDGET_DRAWABLE (self))
    {
      NoteInfo info;
      gint x, y;

      gdk_window_get_pointer (self->canvas, &x, &y, NULL);
      if (x < 0 || y < 0 || x >= CANVAS_WIDTH (self) || y >= CANVAS_HEIGHT (self))
	{
	  /* fallback value if the pointer is outside the window */
	  x = CANVAS_WIDTH (self) / 3;
	  y = CANVAS_HEIGHT (self) / 3;
	}
      tick = coord_to_tick (self, x, FALSE);
      coord_to_note (self, y, &info);
      semitone = info.valid_semitone;
      octave = info.valid_octave;
    }
  else
    {
      semitone = 6;
      octave = (MIN_OCTAVE (self) + MAX_OCTAVE (self)) / 2;
      tick = 0;
    }
  if (note_p)
    *note_p = SFI_NOTE_MAKE_VALID (SFI_NOTE_GENERIC (octave, semitone));
  if (tick_p)
    *tick_p = tick;
}

void
bst_piano_roll_set_canvas_cursor (BstPianoRoll *self,
				  GdkCursorType cursor_type)
{
  g_return_if_fail (BST_IS_PIANO_ROLL (self));

  if (cursor_type != self->canvas_cursor)
    {
      self->canvas_cursor = cursor_type;
      if (GTK_WIDGET_REALIZED (self))
	gxk_window_set_cursor_type (self->canvas, self->canvas_cursor);
    }
}

void
bst_piano_roll_set_vpanel_cursor (BstPianoRoll *self,
				  GdkCursorType cursor_type)
{
  g_return_if_fail (BST_IS_PIANO_ROLL (self));

  if (cursor_type != self->vpanel_cursor)
    {
      self->vpanel_cursor = cursor_type;
      if (GTK_WIDGET_REALIZED (self))
	gxk_window_set_cursor_type (self->vpanel, self->vpanel_cursor);
    }
}

void
bst_piano_roll_set_hpanel_cursor (BstPianoRoll *self,
				  GdkCursorType cursor_type)
{
  g_return_if_fail (BST_IS_PIANO_ROLL (self));

  if (cursor_type != self->hpanel_cursor)
    {
      self->hpanel_cursor = cursor_type;
      if (GTK_WIDGET_REALIZED (self))
        gxk_window_set_cursor_type (self->hpanel, self->hpanel_cursor);
    }
}
