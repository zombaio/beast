/* BseWaveOsc - BSE Wave Oscillator
 * Copyright (C) 2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsewaveosc.h"

#include <bse/gslengine.h>
#include <bse/gslwavechunk.h>
#include <bse/gslfilter.h>



/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_WAVE,
  PARAM_FM_PERC
};


/* --- prototypes --- */
static void	bse_wave_osc_init		(BseWaveOsc		*wosc);
static void	bse_wave_osc_class_init		(BseWaveOscClass	*class);
static void	bse_wave_osc_class_finalize	(BseWaveOscClass	*class);
static void	bse_wave_osc_destroy		(BseObject		*object);
static void	bse_wave_osc_set_property	(BseWaveOsc		*wosc,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	bse_wave_osc_get_property	(BseWaveOsc		*wosc,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void     bse_wave_osc_prepare		(BseSource		*source);
static void	bse_wave_osc_context_create	(BseSource		*source,
						 guint			 context_handle,
						 GslTrans		*trans);
static void     bse_wave_osc_reset		(BseSource		*source);
static void	bse_wave_osc_update_modules	(BseWaveOsc		*wosc);


/* --- variables --- */
static GType		 type_id_wave_osc = 0;
static gpointer		 parent_class = NULL;
static const GTypeInfo type_info_wave_osc = {
  sizeof (BseWaveOscClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_wave_osc_class_init,
  (GClassFinalizeFunc) bse_wave_osc_class_finalize,
  NULL /* class_data */,
  
  sizeof (BseWaveOsc),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_wave_osc_init,
};


/* --- functions --- */
static void
bse_wave_osc_class_init (BseWaveOscClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel, ichannel;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_wave_osc_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_wave_osc_get_property;

  object_class->destroy = bse_wave_osc_destroy;
  
  source_class->prepare = bse_wave_osc_prepare;
  source_class->context_create = bse_wave_osc_context_create;
  source_class->reset = bse_wave_osc_reset;
  
  bse_object_class_add_param (object_class, "Wave",
			      PARAM_WAVE,
			      g_param_spec_object ("wave", "Wave", "Wave to play",
						   BSE_TYPE_WAVE,
						   BSE_PARAM_DEFAULT));
  
  ichannel = bse_source_class_add_ichannel (source_class, "freq_in", "Frequency Input");
  g_assert (ichannel == BSE_WAVE_OSC_ICHANNEL_FREQ);
  ichannel = bse_source_class_add_ichannel (source_class, "sync_in", "Syncronization Input");
  g_assert (ichannel == BSE_WAVE_OSC_ICHANNEL_SYNC);
  ochannel = bse_source_class_add_ochannel (source_class, "wave_out", "Wave Output");
  g_assert (ochannel == BSE_WAVE_OSC_OCHANNEL_WAVE);
  ochannel = bse_source_class_add_ochannel (source_class, "sync_out", "Syncronization Output");
  g_assert (ochannel == BSE_WAVE_OSC_OCHANNEL_SYNC);
  ochannel = bse_source_class_add_ochannel (source_class, "gate_out", "Gate Output");
  g_assert (ochannel == BSE_WAVE_OSC_OCHANNEL_GATE);
}

static void
bse_wave_osc_class_finalize (BseWaveOscClass *class)
{
}

static void
bse_wave_osc_init (BseWaveOsc *wosc)
{
  wosc->wave = NULL;
  wosc->vars.start_offset = 0;
  wosc->vars.play_dir = 1;
  wosc->vars.index = NULL;
}

static void
bse_wave_osc_destroy (BseObject *object)
{
  BseWaveOsc *wosc = BSE_WAVE_OSC (object);

  if (wosc->wave)
    {
      g_object_unref (wosc->wave);
      wosc->wave = NULL;
    }

  /* chain parent class' handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
notify_wave_changed (BseWaveOsc *wosc)
{
  g_object_notify (G_OBJECT (wosc), "wave");
}

static void
wave_uncross (BseItem *owner,
	      BseItem *ref_item)
{
  BseWaveOsc *wosc = BSE_WAVE_OSC (owner);

  wosc->wave = NULL;
  wosc->vars.index = NULL;
  g_object_notify (G_OBJECT (wosc), "wave");

  bse_wave_osc_update_modules (wosc);
  if (BSE_SOURCE_PREPARED (wosc))
    {
      /* need to make sure our modules know about BseWave vanishing
       * before we return
       */
      gsl_engine_wait_on_trans ();
    }
}

static void
bse_wave_osc_set_property (BseWaveOsc  *wosc,
			   guint        param_id,
			   GValue      *value,
			   GParamSpec  *pspec)
{
  switch (param_id)
    {
    case PARAM_WAVE:
      if (wosc->wave)
	{
	  g_object_disconnect (wosc->wave,
			       "any_signal", notify_wave_changed, wosc,
			       NULL);
	  bse_item_cross_unref (BSE_ITEM (wosc), BSE_ITEM (wosc->wave));
	  wosc->wave = NULL;
	  wosc->vars.index = NULL;
	}
      wosc->wave = g_value_get_object (value);
      if (wosc->wave)
	{
	  bse_item_cross_ref (BSE_ITEM (wosc), BSE_ITEM (wosc->wave), wave_uncross);
	  g_object_connect (wosc->wave,
			    "swapped_signal::notify::name", notify_wave_changed, wosc,
			    NULL);
	  if (BSE_SOURCE_PREPARED (wosc))
	    wosc->vars.index = bse_wave_get_index_for_modules (wosc->wave);
	}
      bse_wave_osc_update_modules (wosc);
      if (BSE_SOURCE_PREPARED (wosc))
	{
	  /* need to make sure our modules know about BseWave vanishing
	   * before we return
	   */
	  gsl_engine_wait_on_trans ();
	}
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (wosc, param_id, pspec);
      break;
    }
}

static void
bse_wave_osc_get_property (BseWaveOsc   *wosc,
			   guint        param_id,
			   GValue      *value,
			   GParamSpec  *pspec)
{
  switch (param_id)
    {
    case PARAM_WAVE:
      g_value_set_object (value, wosc->wave);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (wosc, param_id, pspec);
      break;
    }
}

static void
bse_wave_osc_prepare (BseSource *source)
{
#if 0 // shame, BseWave might not be prepared already, so defer to context_create
  BseWaveOsc *wosc = BSE_WAVE_OSC (source);

  wosc->vars.index = wosc->wave ? bse_wave_get_index_for_modules (wosc->wave) : NULL;
#endif

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

#define	ORDER	8	/* <= BSE_MAX_BLOCK_PADDING !! */

typedef struct
{
  BseWaveOscVars    vars;
  gfloat	    play_freq, last_sync_level, last_freq_level;
  GslWaveChunk     *wchunk;
  gfloat            zero_padding, part_step;
  GslWaveChunkBlock block;
  gfloat           *x;			/* pointer into block */
  guint             cur_pos, istep;
  gdouble           a[ORDER + 1];	/* order */
  gdouble           b[ORDER + 1];	/* reversed order */
  gdouble           y[ORDER + 1];
  guint             j;			/* y[] index */
} WaveOscModule;
#define FRAC_SHIFT	(16)
#define FRAC_MASK	((1 << FRAC_SHIFT) - 1)

static inline void
wmod_set_freq (WaveOscModule *wmod,
	       gfloat	      play_freq,
	       gboolean	      change_wave)
{
  gfloat step;
  guint i, istep;

  wmod->play_freq = play_freq;
  if (!wmod->vars.index)
    return;

  if (change_wave)
    {
      if (wmod->wchunk)
	gsl_wave_chunk_unuse_block (wmod->wchunk, &wmod->block);
      wmod->wchunk = bse_wave_index_lookup_best (wmod->vars.index, wmod->play_freq);
      wmod->zero_padding = 2;
      wmod->part_step = wmod->zero_padding * wmod->wchunk->mix_freq;
      wmod->part_step /= wmod->wchunk->osc_freq * BSE_MIX_FREQ_d;
      g_print ("wave lookup: want=%f got=%f length=%lu\n",
	       play_freq, wmod->wchunk->osc_freq, wmod->wchunk->wave_length);
      wmod->block.offset = wmod->vars.start_offset;
      gsl_wave_chunk_use_block (wmod->wchunk, &wmod->block);
      // wmod->last_sync_level = 0;
      // wmod->last_freq_level = 0;
      wmod->x = wmod->block.start;
    }
  step = wmod->part_step * wmod->play_freq;
  istep = step * (FRAC_MASK + 1.) + 0.5;

  if (istep != wmod->istep)
    {
      gfloat nyquist_fact = GSL_PI / 22050., cutoff_freq = 18000, stop_freq = 24000;
      gfloat filt_fact = CLAMP (1. / step, 1. / (6. * wmod->zero_padding), 1. / wmod->zero_padding);
      gfloat freq_c = cutoff_freq * nyquist_fact * filt_fact;
      gfloat freq_r = stop_freq * nyquist_fact * filt_fact;

      wmod->istep = istep;
      gsl_filter_tscheb2_lp (ORDER, freq_c, freq_r / freq_c, 0.18, wmod->a, wmod->b);
      for (i = 0; i < ORDER + 1; i++)
	wmod->a[i] *= wmod->zero_padding;	/* scale to compensate for zero-padding */
      for (i = 0; i < (ORDER + 1) / 2; i++)	/* reverse bs */
	{
	  gfloat t = wmod->b[ORDER - i];
	  
	  wmod->b[ORDER - i] = wmod->b[i];
	  wmod->b[i] = t;
	}
      g_print ("filter: fc=%f fr=%f st=%f is=%u\n", freq_c/GSL_PI*2, freq_r/GSL_PI*2, step, wmod->istep);
    }
}

static void
wmod_access (GslModule *module,
	     gpointer   data)
{
  WaveOscModule *wmod = module->user_data;
  BseWaveOscVars *vars = data;

  if (wmod->vars.index != vars->index)
    {
      if (wmod->wchunk)
	gsl_wave_chunk_unuse_block (wmod->wchunk, &wmod->block);
      wmod->wchunk = NULL;
    }
  wmod->vars = *vars;
  wmod_set_freq (wmod, wmod->play_freq, wmod->wchunk == NULL);
}

static void
wmod_free (gpointer        data,
	   const GslClass *klass)
{
  WaveOscModule *wmod = data;

  if (wmod->vars.index && wmod->wchunk)
    gsl_wave_chunk_unuse_block (wmod->wchunk, &wmod->block);
  g_free (wmod);
}

static void
bse_wave_osc_update_modules (BseWaveOsc *wosc)
{
  if (BSE_SOURCE_PREPARED (wosc))
    bse_source_access_omodules (BSE_SOURCE (wosc),
				BSE_WAVE_OSC_OCHANNEL_WAVE,
				wmod_access,
				g_memdup (&wosc->vars, sizeof (wosc->vars)),
				g_free,
				NULL);
}

#define WMOD_MIX_VARIANT_NAME	wmod_mix_nofreq_nosync
#define WMOD_MIX_VARIANT	(0)
#include "bsewaveosc-aux.c"
#undef  WMOD_MIX_VARIANT
#undef  WMOD_MIX_VARIANT_NAME

#define WMOD_MIX_VARIANT_NAME	wmod_mix_nofreq_sync
#define WMOD_MIX_VARIANT	(WMOD_MIX_WITH_SYNC)
#include "bsewaveosc-aux.c"
#undef  WMOD_MIX_VARIANT
#undef  WMOD_MIX_VARIANT_NAME

#define WMOD_MIX_VARIANT_NAME	wmod_mix_freq_nosync
#define WMOD_MIX_VARIANT	(WMOD_MIX_WITH_FREQ)
#include "bsewaveosc-aux.c"
#undef  WMOD_MIX_VARIANT
#undef  WMOD_MIX_VARIANT_NAME

#define WMOD_MIX_VARIANT_NAME	wmod_mix_freq_sync
#define WMOD_MIX_VARIANT	(WMOD_MIX_WITH_SYNC | WMOD_MIX_WITH_FREQ)
#include "bsewaveosc-aux.c"
#undef  WMOD_MIX_VARIANT
#undef  WMOD_MIX_VARIANT_NAME


static void
wmod_process (GslModule *module,
	      guint      n_values)
{
  WaveOscModule *wmod = module->user_data;

  if (module->istreams[BSE_WAVE_OSC_ICHANNEL_FREQ].connected)
    {
      if (module->istreams[BSE_WAVE_OSC_ICHANNEL_SYNC].connected)
	wmod_mix_freq_sync (module, n_values);
      else /* nosync */
	wmod_mix_freq_nosync (module, n_values);
    }
  else /* nofreq */
    {
      if (module->istreams[BSE_WAVE_OSC_ICHANNEL_SYNC].connected)
	wmod_mix_nofreq_sync (module, n_values);
      else /* nosync */
	wmod_mix_nofreq_nosync (module, n_values);
    }

  if (wmod->y[0] != 0.0 &&
      !(fabs (wmod->y[0]) > GSL_SIGNAL_EPSILON && fabs (wmod->y[0]) < GSL_SIGNAL_KAPPA))
    {
      guint i;

      g_printerr ("clearing filter state at:\n");
      for (i = 0; i < ORDER; i++)
	{
	  g_printerr ("%u) %+.38f\n", i, wmod->y[i]);
	  if (GSL_DOUBLE_IS_INF (wmod->y[0]) || fabs (wmod->y[0]) > GSL_SIGNAL_KAPPA)
	    wmod->y[i] = GSL_DOUBLE_SIGN (wmod->y[0]) ? -1.0 : 1.0;
	  else
	    wmod->y[i] = 0.0;
	}
    }
  g_assert (!GSL_DOUBLE_IS_NANINF (wmod->y[0]));
  g_assert (!GSL_DOUBLE_IS_SUBNORMAL (wmod->y[0]));
}

static void
bse_wave_osc_context_create (BseSource *source,
			     guint      context_handle,
			     GslTrans  *trans)
{
  static const GslClass wmod_class = {
    BSE_WAVE_OSC_N_ICHANNELS,	/* n_istreams */
    0,                          /* n_jstreams */
    BSE_WAVE_OSC_N_OCHANNELS,	/* n_ostreams */
    wmod_process,		/* process */
    wmod_free,			/* free */
    GSL_COST_NORMAL,		/* cost */
  };
  BseWaveOsc *wosc = BSE_WAVE_OSC (source);
  WaveOscModule *wmod = g_new0 (WaveOscModule, 1);
  GslModule *module;

  if (!wosc->vars.index && wosc->wave)	/* BseWave is prepared now */
    wosc->vars.index = bse_wave_get_index_for_modules (wosc->wave);

  wmod->vars = wosc->vars;
  wmod->last_sync_level = 0;
  wmod->last_freq_level = 0;
  wmod->wchunk = NULL;
  wmod->play_freq = 0;
  wmod->block.play_dir = wmod->vars.play_dir;
  wmod->block.offset = wmod->vars.start_offset;
  wmod_set_freq (wmod, wmod->play_freq, TRUE);

  module = gsl_module_new (&wmod_class, wmod);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_wave_osc_reset (BseSource *source)
{
  BseWaveOsc *wosc = BSE_WAVE_OSC (source);

  wosc->vars.index = NULL;

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}


/* --- Export to BSE --- */
#include "./icons/waveosc.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_wave_osc, "BseWaveOsc", "BseSource",
    "BseWaveOsc is a waveeric oscillator source",
    &type_info_wave_osc,
    "/Source/Oscillators/Wave Oscillator",
    { WAVE_OSC_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      WAVE_OSC_IMAGE_WIDTH, WAVE_OSC_IMAGE_HEIGHT,
      WAVE_OSC_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
