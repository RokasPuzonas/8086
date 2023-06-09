CFLAGS=-g -Wall

.PHONY := cli web clean serve-web

cli: src/cli.c
	mkdir -p build
	gcc -o build/cli.exe src/cli.c $(CFLAGS)

web: src/web.c
	mkdir -p build/web
	emcc -o build/web/sim8086.js src/web.c --no-entry -sEXPORTED_RUNTIME_METHODS=cwrap,AsciiToString -sEXPORTED_FUNCTIONS=_free,_malloc $(CFLAGS)
	cp -r src/web/* build/web

serve-web: web
	cd build/web && python -m http.server

watch-web:
	live-server build/web --wait=250

clean:
	rm -r build
