default: superman

superman:
	clang -lyaml -o superman superman.c

install: superman
	rm -f /usr/local/bin/superman
	ln -s `pwd -P`/superman /usr/local/bin/

clean:
	rm superman

.PHONY: test
test: clean superman
	./superman test.yaml
