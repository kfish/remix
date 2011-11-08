docdir=$(prefix)/share/doc/@PACKAGE@

EXTRA_DIST = Doxyfile.in

noinst_DATA = doxygen-build.stamp

if HAVE_DOXYGEN
doxygen-build.stamp: Doxyfile $(top_srcdir)/include/remix/*.h
	doxygen
	touch doxygen-build.stamp
else
doxygen-build.stamp:
	echo "*** Warning: Doxygen not found; documentation will not be built."
	touch doxygen-build.stamp
endif

dist_docdir = $(distdir)/libremix

dist-hook:
	if test -d libremix; then \
	  mkdir $(dist_docdir); \
	  for dir in libremix/*; do \
	    b=`basename $$dir`; \
	    if test $$b != "CVS"; then \
	      if test -d $$dir; then \
	        mkdir $(dist_docdir)/$$b; \
	        for f in $$dir/*; do \
	          cp -p $$f $(dist_docdir)/$$b; \
	        done \
	      fi \
	    fi \
	  done \
	fi


install-data-local: doxygen-build.stamp
	$(mkinstalldirs) $(DESTDIR)$(docdir)
	if test -d libremix; then \
	  for dir in libremix/*; do \
	    if test -d $$dir; then \
	      b=`basename $$dir`; \
	      $(mkinstalldirs) $(DESTDIR)$(docdir)/$$b; \
	      for f in $$dir/*; do \
	        $(INSTALL_DATA) $$f $(DESTDIR)$(docdir)/$$b; \
	      done \
	    fi \
	  done \
	fi

uninstall-local:
	rm -rf $(DESTDIR)$(docdir)

clean-local:
	if test -d libremix; then rm -rf libremix; fi
	if test -f doxygen-build.stamp; then rm -f doxygen-build.stamp; fi

