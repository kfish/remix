docdir=$(prefix)/share/doc/@PACKAGE@

EXTRA_DIST = Doxyfile.in

doc_DATA = doxygen-build.stamp

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
            if test -d $$dir; then \
              b=`basename $$dir`; \
              mkdir $(dist_docdir)/$$b; \
	      for f in $$dir/*; do \
                cp -p $$f $(dist_docdir)/$$b; \
              done \
            fi \
          done \
	fi


install-data-local: doxygen-build.stamp
	$(mkinstalldirs) $(docdir)
	if test -d libremix; then \
	  for dir in libremix/*; do \
	    if test -d $$dir; then \
	      b=`basename $$dir`; \
	      $(mkinstalldirs) $(docdir)/$$b; \
	      for f in $$dir/*; do \
		$(INSTALL_DATA) $$f $(docdir)/$$b; \
              done \
	    fi \
	  done \
	fi

uninstall-local:
	rm -rf $(docdir)

clean-local:
	if test -d libremix; then rm -rf libremix; fi
	if test -f doxygen-build.stamp; then rm -f doxygen-build.stamp; fi

