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

install:
	cd $(BINDIR); make install
	cp -r include/ /usr/local/include/

clean:
	# cd src/bdwgc; make clean
	rm -rf $(BINDIR)
