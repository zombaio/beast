/* BEAST - Bedevilled Audio System
 * Copyright (C) 2000 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include	"bstradiotools.h"

#include	<ctype.h>


/* --- signals --- */
enum {
  SIGNAL_SET_TOOL,
  SIGNAL_LAST
};
typedef gboolean (*SetTool) (GtkObject *object,
			     guint      tool_id);


/* --- prototypes --- */
static void	  bst_radio_tools_class_init		(BstRadioToolsClass	*klass);
static void	  bst_radio_tools_init			(BstRadioTools		*rtools,
							 BstRadioToolsClass     *class);
static void	  bst_radio_tools_destroy		(GtkObject		*object);
static void	  bst_radio_tools_do_set_tool		(BstRadioTools		*rtools,
							 guint         		 tool_id);


/* --- static variables --- */
static gpointer            parent_class = NULL;
static BstRadioToolsClass *bst_radio_tools_class = NULL;
static guint               radio_tools_signals[SIGNAL_LAST] = { 0 };


/* --- functions --- */
GtkType
bst_radio_tools_get_type (void)
{
  static GtkType radio_tools_type = 0;
  
  if (!radio_tools_type)
    {
      GtkTypeInfo radio_tools_info =
      {
	"BstRadioTools",
	sizeof (BstRadioTools),
	sizeof (BstRadioToolsClass),
	(GtkClassInitFunc) bst_radio_tools_class_init,
	(GtkObjectInitFunc) bst_radio_tools_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      radio_tools_type = gtk_type_unique (GTK_TYPE_OBJECT, &radio_tools_info);
    }
  
  return radio_tools_type;
}

static void
bst_radio_tools_class_init (BstRadioToolsClass *class)
{
  GtkObjectClass *object_class;
  
  parent_class = gtk_type_class (GTK_TYPE_OBJECT);
  object_class = GTK_OBJECT_CLASS (class);
  bst_radio_tools_class = class;
  
  object_class->destroy = bst_radio_tools_destroy;
  
  class->tooltips = NULL;
  class->set_tool = bst_radio_tools_do_set_tool;

  radio_tools_signals[SIGNAL_SET_TOOL] =
    gtk_signal_new ("set_tool",
		    GTK_RUN_LAST | GTK_RUN_NO_RECURSE,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (BstRadioToolsClass, set_tool),
		    gtk_marshal_NONE__UINT,
		    GTK_TYPE_NONE,
		    1, GTK_TYPE_UINT);
  gtk_object_class_add_signals (object_class, radio_tools_signals, SIGNAL_LAST);
}

static void
bst_radio_tools_init (BstRadioTools      *rtools,
		      BstRadioToolsClass *class)
{
  // GtkObject *object = GTK_OBJECT (rtools);

  rtools->block_tool_id = FALSE;
  rtools->tool_id = 0;
  rtools->n_tools = 0;
  rtools->tools = NULL;
  rtools->widgets = NULL;
  
  if (!class->tooltips)
    {
      class->tooltips = gtk_tooltips_new ();
      gtk_object_ref (GTK_OBJECT (class->tooltips));
      gtk_object_sink (GTK_OBJECT (class->tooltips));
      gtk_signal_connect (GTK_OBJECT (class->tooltips),
			  "destroy",
			  gtk_widget_destroyed,
			  &class->tooltips);
    }
  else
    gtk_object_ref (GTK_OBJECT (class->tooltips));
}

static void
bst_radio_tools_destroy (GtkObject *object)
{
  BstRadioTools *rtools = BST_RADIO_TOOLS (object);

  bst_radio_tools_clear_tools (rtools);

  while (rtools->widgets)
    gtk_widget_destroy (rtools->widgets->data);

  gtk_object_unref (GTK_OBJECT (BST_RADIO_TOOLS_GET_CLASS (rtools)->tooltips));
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

BstRadioTools*
bst_radio_tools_new (void)
{
  GtkObject *object;
  
  object = gtk_object_new (BST_TYPE_RADIO_TOOLS, NULL);
  
  return BST_RADIO_TOOLS (object);
}

static void
bst_radio_tools_do_set_tool (BstRadioTools *rtools,
			     guint          tool_id)
{
  GSList *slist, *next;

  rtools->tool_id = tool_id;

  rtools->block_tool_id = TRUE;
  for (slist = rtools->widgets; slist; slist = next)
    {
      GtkWidget *widget = slist->data;

      next = slist->next;
      if (GTK_IS_TOGGLE_BUTTON (widget))
	{
	  tool_id = GPOINTER_TO_UINT (gtk_object_get_user_data (GTK_OBJECT (widget)));
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), tool_id == rtools->tool_id);
	}
    }
  rtools->block_tool_id = FALSE;
}

void
bst_radio_tools_set_tool (BstRadioTools *rtools,
			  guint          tool_id)
{
  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));

  if (rtools->tool_id != tool_id && !rtools->block_tool_id)
    gtk_signal_emit (GTK_OBJECT (rtools), radio_tools_signals[SIGNAL_SET_TOOL], tool_id);
}

void
bst_radio_tools_add_category (BstRadioTools    *rtools,
			      guint             tool_id,
			      BseCategory      *category,
			      BstRadioToolFlags flags)
{
  gchar *tip;

  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));
  g_return_if_fail (category != NULL);
  g_return_if_fail (flags != 0);

#if 0
  guint i, next_uc = 0;

  /* strip first namespace prefix from type name */
  name = bse_type_name (category->type);
  for (i = 0; name[i] != 0; i++)
    if (i && toupper(name[i]) == name[i])
      {
	next_uc = i;
	break;
      }
  if (toupper(name[0]) == name[0] && next_uc > 0)
    name += next_uc;
#endif
  
  tip = g_strconcat (category->category + category->mindex + 1,
		     " [",
		     bse_type_name (category->type),
		     "]",
		     NULL);
  bst_radio_tools_add_tool (rtools,
			    tool_id,
			    category->category + category->mindex + 1,
			    tip,
			    bse_type_blurb (category->type),
			    category->icon,
			    flags);
  g_free (tip);
}

void
bst_radio_tools_add_tool (BstRadioTools    *rtools,
			  guint             tool_id,
			  const gchar      *tool_name,
			  const gchar      *tool_tip,
			  const gchar      *tool_blurb,
			  BseIcon          *tool_icon,
			  BstRadioToolFlags flags)
{
  guint i;

  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));
  g_return_if_fail (tool_name != NULL);
  g_return_if_fail (flags != 0);
  if (!tool_tip)
    tool_tip = tool_name;
  if (!tool_blurb)
    tool_blurb = tool_tip;
  if (!tool_icon)
    tool_icon = bst_icon_from_stock (BST_ICON_NOICON);

  i = rtools->n_tools++;
  rtools->tools = g_renew (BstRadioTool, rtools->tools, rtools->n_tools);
  rtools->tools[i].tool_id = tool_id;
  rtools->tools[i].name = g_strdup (tool_name);
  rtools->tools[i].tip = g_strdup (tool_tip);
  rtools->tools[i].blurb = g_strdup (tool_blurb);
  rtools->tools[i].icon = bse_icon_ref (tool_icon);
  rtools->tools[i].flags = flags;
}

void
bst_radio_tools_clear_tools (BstRadioTools *rtools)
{
  guint i;

  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));

  for (i = 0; i < rtools->n_tools; i++)
    {
      g_free (rtools->tools[i].name);
      g_free (rtools->tools[i].tip);
      g_free (rtools->tools[i].blurb);
      bse_icon_unref (rtools->tools[i].icon);
    }
  rtools->n_tools = 0;
  g_free (rtools->tools);
  rtools->tools = NULL;
}

static void
rtools_widget_destroyed (BstRadioTools *rtools,
			 GtkWidget     *widget)
{
  rtools->widgets = g_slist_remove (rtools->widgets, widget);
}

static void
rtools_toggle_toggled (BstRadioTools   *rtools,
		       GtkToggleButton *toggle)
{
  guint tool_id;

  if (!rtools->block_tool_id)
    {
      tool_id = GPOINTER_TO_UINT (gtk_object_get_user_data (GTK_OBJECT (toggle)));
      bst_radio_tools_set_tool (rtools, toggle->active ? tool_id : 0);
    }
}

void
bst_radio_tools_build_toolbar (BstRadioTools *rtools,
			       GtkToolbar    *toolbar)
{
  guint i;

  g_return_if_fail (BST_IS_RADIO_TOOLS (rtools));
  g_return_if_fail (GTK_IS_TOOLBAR (toolbar));

  for (i = 0; i < rtools->n_tools; i++)
    {
      GtkWidget *button, *forest;

      if (!(rtools->tools[i].flags & BST_RADIO_TOOLS_TOOLBAR))
	continue;

      forest = bst_forest_from_bse_icon (rtools->tools[i].icon, BST_TOOLBAR_ICON_WIDTH, BST_TOOLBAR_ICON_HEIGHT);
      button = gtk_toolbar_append_element (toolbar,
					   GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL,
					   rtools->tools[i].name,
					   rtools->tools[i].tip, NULL,
					   forest,
					   NULL, NULL);
      gtk_widget_set (button,
		      "user_data", GUINT_TO_POINTER (rtools->tools[i].tool_id),
		      "object_signal::toggled", rtools_toggle_toggled, rtools,
		      "object_signal::destroy", rtools_widget_destroyed, rtools,
		      NULL);
      rtools->widgets = g_slist_prepend (rtools->widgets, button);
    }

  BST_RADIO_TOOLS_GET_CLASS (rtools)->set_tool (rtools, rtools->tool_id);
}

static void
toggle_apply_blurb (GtkToggleButton *toggle,
		    GtkWidget       *text)
{
  gpointer tool_id = gtk_object_get_data (GTK_OBJECT (toggle), "user_data");
  gpointer blurb_id = gtk_object_get_data (GTK_OBJECT (text), "user_data");

  if (tool_id == blurb_id && !toggle->active)
    bst_wrap_text_set (text, NULL, FALSE, GUINT_TO_POINTER (~0));
  else if (toggle->active && tool_id != blurb_id)
    bst_wrap_text_set (text,
		       gtk_object_get_data (GTK_OBJECT (toggle), "blurb"),
		       TRUE,
		       tool_id);
}

GtkWidget*
bst_radio_tools_build_palette (BstRadioTools *rtools,
			       gboolean       show_descriptions,
			       GtkReliefStyle relief)
{
  GtkWidget *vbox, *table, *text = NULL;
  guint i, n = 0, row = 4;
  
  g_return_val_if_fail (BST_IS_RADIO_TOOLS (rtools), NULL);
  
  vbox = gtk_widget_new (GTK_TYPE_VBOX,
			 "visible", TRUE,
			 "homogeneous", FALSE,
			 "resize_mode", GTK_RESIZE_QUEUE,
			 NULL);
  table = gtk_widget_new (GTK_TYPE_TABLE,
			  "visible", TRUE,
			  "homogeneous", TRUE,
			  NULL);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
  if (show_descriptions)
    {
      text = bst_wrap_text_create (NULL, FALSE, GUINT_TO_POINTER (~0));
      gtk_widget_ref (text);
      gtk_object_sink (GTK_OBJECT (text));
    }
  
  for (i = 0; i < rtools->n_tools; i++)
    {
      GtkWidget *button, *forest;
      
      if (!(rtools->tools[i].flags & BST_RADIO_TOOLS_PALETTE))
	continue;
      
      forest = bst_forest_from_bse_icon (rtools->tools[i].icon, BST_PALETTE_ICON_WIDTH, BST_PALETTE_ICON_HEIGHT);
      button = gtk_widget_new (GTK_TYPE_TOGGLE_BUTTON,
			       "visible", TRUE,
			       "draw_indicator", FALSE,
			       "relief", relief,
			       "child", forest,
			       "user_data", GUINT_TO_POINTER (rtools->tools[i].tool_id),
			       "object_signal::toggled", rtools_toggle_toggled, rtools,
			       "object_signal::destroy", rtools_widget_destroyed, rtools,
			       text ? "signal::toggled" : NULL, toggle_apply_blurb, text,
			       NULL);
      gtk_tooltips_set_tip (BST_RADIO_TOOLS_GET_CLASS (rtools)->tooltips, button, rtools->tools[i].tip, NULL);
      gtk_object_set_data_full (GTK_OBJECT (button), "blurb", g_strdup (rtools->tools[i].blurb), g_free);
      rtools->widgets = g_slist_prepend (rtools->widgets, button);
      gtk_table_attach (GTK_TABLE (table),
			button,
			n % row, n % row + 1,
			n / 4, n / 4 + 1,
			GTK_SHRINK, GTK_SHRINK, // GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			0, 0);
      n++;
    }
  
  if (n && text)
    {
      GtkWidget *hbox;

      hbox = gtk_widget_new (GTK_TYPE_HBOX,
			     "visible", TRUE,
			     NULL);
      gtk_box_pack_start (GTK_BOX (hbox), text, TRUE, TRUE, 5);
      gtk_widget_new (GTK_TYPE_FRAME,
		      "visible", TRUE,
		      "label", "Description",
		      "label_xalign", 0.5,
		      "border_width", 5,
		      "parent", vbox,
		      "child", hbox,
		      "width", 1,
		      "height", 100,
		      NULL);
    }
  if (text)
    gtk_widget_unref (text);
  
  BST_RADIO_TOOLS_GET_CLASS (rtools)->set_tool (rtools, rtools->tool_id);
  
  return vbox;
}
