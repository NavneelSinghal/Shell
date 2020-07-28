all: exe/shell
	@:
exe/shell:
	@gcc -o exe/shell src/2018CS10360_sh.c
run: exe/shell
	@exe/shell
clean:
	@rm -rf exe/shell
