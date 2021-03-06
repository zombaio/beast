# BEAST & BSE
include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir) -I$(srcdir)
DEFS        += @DEFINE__FILE_DIR__@ -DG_LOG_DOMAIN="\"BEAST-TEST\"" # -DG_DISABLE_CONST_RETURNS
AM_CXXFLAGS += $(SFI_CPPFLAGS)

SFIDL = $(top_builddir)/sfi/sfidl
SFIDL_INC = --nostdinc -I$(top_srcdir) -I$(top_builddir)


# == BSE file check ==
# This test checks that all .bse files contained in the beast tarball
# will load without any warnings or errors being issued. This is done
# by first constructing the tarball file list via git ls-tree and then
# filtering out bse files and finally trying to load these without warnings.
bsefiles.list: $(GITSTAMP)
	$(AM_V_GEN)
	$(Q) git ls-tree --name-only --full-tree -r HEAD > $@.tmp1
	$(Q) grep '\.bse$$' $@.tmp1 > $@.tmp2
	$(Q) mv $@.tmp2 $@ && rm -f $@.tmp1
CLEANFILES += @INGIT@ bsefiles.list
noinst_DATA += bsefiles.list
EXTRA_DIST += bsefiles.list

# == checklist rules ==
# Split the (long) list of files into multiple lists that can be
# checked in parallel. For each file in a list, unless it matches
# the skip-pattern, try to load it and check the output logs.
checklists: bsefiles.list # generate checklist-a .. checklist-e
	$(Q) grep '\.bse$$' bsefiles.list > checklist || \
	{ echo '$@: error: bsefiles.list is empty' >&2; exit 1; }
	$(Q) split -n l/5 -a 1 checklist checklist-
	$(Q) rm -f checklist
SKIP_PATTERN = "tests/latency/midi-latency.bse"
checklist-%-test: checklists
	$(Q) for tfile in `cat $(@F:-test=)` ; do		\
	    if echo "$$tfile" | egrep -q $(SKIP_PATTERN) ; then	\
	      echo "  SKIP     Loading: $$tfile" ;		\
	    else						\
	      $(CHECK_LOAD) "$(abs_top_builddir)/$$tfile" 2>&1 | tee $(@F).log ; \
	      test ! -s $(@F).log || exit 1			\
	      && echo "  OK       Loading: $$tfile" ;		\
	    fi ; done ; rm -f $(@F).log $(@F:-test=)
CLEANFILES += checklists checklist-? checklist-?-test checklist-?-test.log
check-local: checklist-a-test checklist-b-test checklist-c-test checklist-d-test checklist-e-test
plugin_globs = '$(top_builddir)/plugins/.libs/*.so:$(top_builddir)/plugins/freeverb/.libs/*.so'
samplepath   = '$(top_srcdir)/tests/audio:$(top_srcdir)/library/samples'
# Loading tests need all available samples and plugins.
CHECK_LOAD   = $(strip	$(top_builddir)/bse/bsetool				\
			  $(if $(findstring 1, $(V)),, --quiet)			\
			  check-load						\
			  --bse-pcm-driver null=nosleep				\
			  --bse-midi-driver null				\
			  --bse-rcfile "/dev/null"				\
			  --bse-override-plugin-globs '$(plugin_globs)'		\
		          --bse-override-sample-path $(samplepath)		\
)

# == bse-version-check ==
bse-version-check: bsefiles.list
	TESTVERSION="$(BST_MAJOR_VERSION).$(BST_MINOR_VERSION)." ;	\
	for tfile in `cat bsefiles.list` ; do				\
	  file="$(top_srcdir)/$$tfile" ;				\
	  echo "Check project version: $$file" ;			\
	  head -n5 "$$file" |						\
	    grep -Fq "(bse-version \"$$TESTVERSION" ||			\
	      { grep -F bse-version "$$file" ; exit 1 ; }		\
	done
	@MESSAGETEXT="All tested BSE files passed version check"	\
	&& echo $$MESSAGETEXT | sed 's/./=/g' 				\
	&& echo $$MESSAGETEXT		 				\
	&& echo $$MESSAGETEXT | sed 's/./=/g'
.PHONY: bse-version-check
check: # bse-version-check

