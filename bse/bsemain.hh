// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MAIN_H__
#define __BSE_MAIN_H__
#include	<bse/bse.hh>	/* initialization */
#include        <bse/bsetype.hh>
G_BEGIN_DECLS

// == BSE Initialization ==
void		bse_init_textdomain_only (void);
void		bse_init_async		(gint		*argc,
					 gchar	      ***argv,
					 const char     *app_name,
					 SfiInitValue    values[]);             // prototyped in bse.hh
SfiGlueContext* bse_init_glue_context	(const gchar    *client);               // prototyped in bse.hh
const char*     bse_check_version	(guint		 required_major,
                                         guint		 required_minor,
                                         guint		 required_micro);       // prototyped in bse.hh

/* initialization for internal utilities */
void		bse_init_inprocess	(gint		*argc,
					 gchar	      ***argv,
					 const char     *app_name,
					 SfiInitValue    values[]);
void		bse_init_test		(gint		*argc,
					 gchar	      ***argv,
					 SfiInitValue    values[]);
/* BSE thread pid (or 0) */
guint           bse_main_getpid         (void);
/* messaging */
void            bse_message_setup_thread_handler (void);
void            bse_message_to_default_handler   (const BseMessage *msg);
/* --- global macros --- */
#define	BSE_THREADS_ENTER()			// bse_main_global_lock ()
#define	BSE_THREADS_LEAVE()			// bse_main_global_unlock ()
#define	BSE_SEQUENCER_LOCK()			sfi_mutex_lock (&bse_main_sequencer_mutex)
#define	BSE_SEQUENCER_UNLOCK()			sfi_mutex_unlock (&bse_main_sequencer_mutex)
#define	BSE_DBG_EXT     			(bse_main_args->debug_extensions != FALSE)
#define	BSE_CONFIG(field)			(bse_main_args->field)
/* --- argc/argv overide settings --- */
typedef struct {
  BirnetInitSettings    birnet;
  guint  	        n_processors;
  /* # values to pad around wave chunk blocks per channel */
  guint  	        wave_chunk_padding;
  guint  	        wave_chunk_big_pad;
  /* data (file) cache block size (aligned to power of 2) */
  guint  	        dcache_block_size;
  /* amount of bytes to spare for memory cache */
  guint  	        dcache_cache_memory;
  guint  	        midi_kammer_note;
  /* kammer frequency, normally 440Hz, historically 435Hz */
  gfloat 	        kammer_freq;
  const gchar          *path_binaries;
  const gchar          *bse_rcfile;
  const gchar          *override_plugin_globs;
  const gchar          *override_script_path;
  const gchar	       *override_sample_path;
  bool                  allow_randomization;	/* init-value "allow-randomization" - enables non-deterministic behavior */
  bool                  force_fpu;		/* init-value "force-fpu" */
  bool		        load_core_plugins;	/* init-value "load-core-plugins" */
  bool		        load_core_scripts;	/* init-value "load-core-scripts" */
  bool		        debug_extensions;	/* init-value "debug-extensions" */
  bool                  load_drivers_early;
  bool                  dump_driver_list;
  int                   latency;
  int                   mixing_freq;
  int                   control_freq;
  SfiRing              *pcm_drivers;
  SfiRing              *midi_drivers;
} BseMainArgs;

/* --- debuging channels --- */
typedef struct {
  SfiDebugChannel       *sequencer;     /* --bse-trace-sequencer */
} BseTraceArgs;

/* --- internal --- */
void    _bse_init_c_wrappers    ();
extern BseMainArgs     *bse_main_args;
extern BseTraceArgs     bse_trace_args;
extern GMainContext    *bse_main_context;
extern BirnetMutex	bse_main_sequencer_mutex;

G_END_DECLS

#endif /* __BSE_MAIN_H__ */
