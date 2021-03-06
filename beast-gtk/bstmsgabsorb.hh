// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_MSG_ABSORB_H__
#define __BST_MSG_ABSORB_H__

#include "bstutils.hh"

/* --- access config file --- */
#define BST_STRDUP_ABSORBRC_FILE()  (g_strconcat (g_get_home_dir (), "/.beast/absorbrc", NULL))

/* --- prototypes --- */
void		       _bst_msg_absorb_config_init	(void);
void		       bst_msg_absorb_config_apply	(SfiSeq         *seq);
GParamSpec*	       bst_msg_absorb_config_pspec      (void);
gboolean               bst_msg_absorb_config_adjust     (const gchar *config_blurb, bool enabled, bool update_version);
gboolean               bst_msg_absorb_config_match      (const gchar    *config_blurb);
void                   bst_msg_absorb_config_update     (const gchar    *config_blurb);
GtkWidget*	       bst_msg_absorb_config_box	(void);
Bst::MsgAbsorbStringSeq* bst_msg_absorb_config_get_global (void);
void                     bst_msg_absorb_config_box_set    (GtkWidget *box, Bst::MsgAbsorbStringSeq *mass);
Bst::MsgAbsorbStringSeq* bst_msg_absorb_config_box_get    (GtkWidget *box);

/* --- config file --- */
void                   bst_msg_absorb_config_save       (void);
void                   bst_msg_absorb_config_load       (void);

#endif /* __BST_MSG_ABSORB_H__ */
