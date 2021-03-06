// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsecxxapi.hh"
#include <bse/bse.hh>
#include <unistd.h>
#include <stdio.h>

static SfiGlueContext *bse_context = NULL;

using namespace Bse;
using namespace std;

void do_sleep (int seconds)
{
  /*
   * sleeps at least the required time, even in the presence of signals
   * for instance due to (gdb)
   */
  while (seconds > 0)
    seconds = sleep (seconds);
}

int
main (int   argc,
      char *argv[])
{
  std::set_terminate (__gnu_cxx::__verbose_terminate_handler);
  Bse::init_async (&argc, argv, "CxxBindingTest");
  bse_context = Bse::init_glue_context (argv[0], []() { g_main_context_wakeup (g_main_context_default()); });
  sfi_glue_context_push (bse_context);
  printout ("type_blurb(BseContainer)=%s\n", type_blurb("BseContainer").c_str());
  const gchar *file_name = "empty.ogg";
  SampleFileInfoHandle info = sample_file_info (file_name);
  if (info.c_ptr())
    {
      printout ("sample_file_info(\"%s\"): file = %s, loader = %s\n",
               file_name, info->file.c_str(), info->loader.c_str());
      printout ("  %d waves contained:\n", info->waves.length());
      for (unsigned int i = 0; i < info->waves.length(); i++)
        printout ("  - %s\n", info->waves[i].c_str());
    }
  else
    {
      printout ("sample_file_info(\"%s\"): failed\n", file_name);
    }

  printout ("error_blurb(Error::DEVICE_ASYNC): %s\n", error_blurb (Error::DEVICE_ASYNC).c_str());

  Server server = 1;    // FIXME: users may not hardcode this

  // printout ("server.get_custom_instrument_dir()=%s\n", server.get_custom_instrument_dir().c_str());

  GConfigHandle prefs = GConfig::from_rec (server.bse_preferences ());
  prefs->plugin_path = "./.libs/testplugin.so";
  SfiRec *rec = GConfig::to_rec (prefs);
  server.set_bse_preferences (rec);
  sfi_rec_unref (rec);
  prefs = GConfig::from_rec (server.bse_preferences());

  printf ("server.bse_preferences().plugin_path=%s\n", prefs->plugin_path.c_str());
  printf ("register core plugins...\n");
  server.register_core_plugins();
  printf ("waiting... (FIXME: need to connect to server signals here)\n");
  do_sleep (2);
  printout ("done.\n");

  /* ... test plugin ... */
  Project test_project = server.use_new_project ("test_project");
  CSynth synth = test_project.create_csynth ("synth");

  printout ("--- creating TestObject ---\n");
  Namespace::TestObject to = synth.create_source("NamespaceTestObject")._proxy(); // FIXME: dynamic_cast me
  if (to)
    printout ("success creating TestObject: %ld\n", to._proxy());
  else
    g_error ("failed.");

  /* --- test procedures --- */
  printout ("--- calling procedure test_exception() ---\n");
  printout ("invoking as: result = test_exception (21, %ld, 42, \"moderately-funky\");\n", to._proxy());
  SfiSeq *pseq = sfi_seq_new ();
  sfi_seq_append_int (pseq, 21);
  sfi_seq_append_proxy (pseq, to._proxy());
  sfi_seq_append_int (pseq, 42);
  sfi_seq_append_choice (pseq, "moderately-funky");
  GValue *rvalue = sfi_glue_call_seq ("namespace-test-exception", pseq);
  sfi_seq_unref (pseq);
  if (!rvalue || !SFI_VALUE_HOLDS_INT (rvalue))
    g_error ("failed (no result).");
  SfiInt result = sfi_value_get_int (rvalue);
  printout ("result=%d\n", result);
  if (result != 21 + 42)
    g_error ("wrong result.");
  printout ("invoking to trigger exception: result = test_exception ();\n");
  pseq = sfi_seq_new ();
  rvalue = sfi_glue_call_seq ("namespace-test-exception", pseq);
  sfi_seq_unref (pseq);
  if (!rvalue || !SFI_VALUE_HOLDS_INT (rvalue))
    g_error ("failed (no result).");
  result = sfi_value_get_int (rvalue);
  printout ("result=%d\n", result);

  /* --- test playback --- */
  file_name = "../test/test-song.bse";
  printout ("--- playing %s... ---\n", file_name);
  Project project = server.use_new_project ("foo");
  project.restore_from_file (file_name);
  project.play();
  sleep (3);
  printout ("done.\n");

  sfi_glue_context_pop ();
}

/* vim:set ts=8 sts=2 sw=2: */
