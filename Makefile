.PHONY: clean install gen debug gc

BINDIR = build

default:
	@mkdir -p $(BINDIR)
	@cd $(BINDIR); cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_DIR=${PWD} ../; $(MAKE) -j --no-print-directory

debug:
	@mkdir -p $(BINDIR)
	@cd $(BINDIR); cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_DIR=${PWD} ../; $(MAKE) -j --no-print-directory


src/bdwgc/.libs/libgc.a:
	cd src/bdwgc; ./autogen.sh; ./configure --enable-cplusplus --disable-shared; $(MAKE) -j

gen:
	@python3 tools/scripts/generate_helion_h.py
	@python3 tools/scripts/generate_tokens.py
	@python3 tools/scripts/generate_src_cmakelists.py

install:
	cd $(BINDIR); make install
	mkdir -p /usr/local/lib/cedar
	@rm -rf /usr/local/lib/cedar
	cp -r lib /usr/local/lib/cedar/
	cp -r include/ /usr/local/include/

clean:
	# cd src/bdwgc; make clean
	rm -rf $(BINDIR)
