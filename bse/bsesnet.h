/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * bsesnet.h: bse synthesis network
 */
#ifndef	__BSE_SNET_H__
#define	__BSE_SNET_H__

#include	<bse/bsesuper.h>
#include	<bse/bseglobals.h> /* FIXME */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- object type macros --- */
#define BSE_TYPE_SNET		   (BSE_TYPE_ID (BseSNet))
#define BSE_SNET(object)	   (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SNET, BseSNet))
#define BSE_SNET_CLASS(class)	   (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SNET, BseSNetClass))
#define BSE_IS_SNET(object)	   (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SNET))
#define BSE_IS_SNET_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SNET))
#define BSE_SNET_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SNET, BseSNetClass))
#define BSE_SNET_USER_SYNTH(src)   ((BSE_OBJECT_FLAGS (src) & BSE_SNET_FLAG_USER_SYNTH) != 0)


/* --- BseSNet flags --- */
typedef enum	/*< skip >*/
{
  BSE_SNET_FLAG_USER_SYNTH	= 1 << (BSE_SUPER_FLAGS_USHIFT + 0)
} BseSNetFlags;
#define BSE_SNET_FLAGS_USHIFT	(BSE_SUPER_FLAGS_USHIFT + 1)


/* --- BseSNet object --- */
typedef struct
{
  gchar     *name;
  guint      context : 31;
  guint	     input : 1;
  GslModule *src_omodule;	/* output */
  guint	     src_ostream;
  GslModule *dest_imodule;	/* input */
  guint	     dest_istream;
} BseSNetPort;
struct _BseSNet
{
  BseSuper	 parent_object;

  GList		*sources;	/* of type BseSource* */
  GSList	*iport_names;
  GSList	*oport_names;
  gpointer	 port_array;	/* of type BseSNetPort* */

  guint		 cid_counter;
  guint		 n_cids;
  guint		*cids;
};
struct _BseSNetClass
{
  BseSuperClass parent_class;
};


/* --- prototypes --- */
guint		bse_snet_create_context		(BseSNet	*snet,
						 GslTrans	*trans);
const gchar*	bse_snet_iport_name_register	(BseSNet	*snet,
						 const gchar	*tmpl_name);
void		bse_snet_iport_name_unregister	(BseSNet	*snet,
						 const gchar	*name);
const gchar*	bse_snet_oport_name_register	(BseSNet	*snet,
						 const gchar	*tmpl_name);
void		bse_snet_oport_name_unregister	(BseSNet	*snet,
						 const gchar	*name);
void		bse_snet_set_iport_src		(BseSNet	*snet,
						 const gchar	*port_name,
						 guint		 snet_context,
						 GslModule	*omodule,
						 guint		 ostream,
						 GslTrans	*trans);
void		bse_snet_set_iport_dest		(BseSNet	*snet,
						 const gchar	*port_name,
						 guint		 snet_context,
						 GslModule	*imodule,
						 guint		 istream,
						 GslTrans	*trans);
void		bse_snet_set_oport_src		(BseSNet	*snet,
						 const gchar	*port_name,
						 guint		 snet_context,
						 GslModule	*omodule,
						 guint		 ostream,
						 GslTrans	*trans);
void		bse_snet_set_oport_dest		(BseSNet	*snet,
						 const gchar	*port_name,
						 guint		 snet_context,
						 GslModule	*imodule,
						 guint		 istream,
						 GslTrans	*trans);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SNET_H__ */
