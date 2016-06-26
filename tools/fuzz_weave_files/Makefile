default:build

build-afl:
	#TODO(Vidar): test AFL_USE_ASAN
	afl-gcc -Os main.c -o fuzz_test -lm

build:
	gcc main.c -g -o fuzz_test -lm

sanitize:
	gcc main.c -fsanitize=address -fno-omit-frame-pointer -g -o fuzz_test -lm

run:build
	./fuzz_test test

test:build-afl
	afl-fuzz -i input -o output -Z 0 ./fuzz_test @@
	#afl-fuzz -i input -o output -Z 0 -M fuzzer0 ./fuzz_test @@ &
	#afl-fuzz -i input -o output -Z 1 -S fuzzer1 ./fuzz_test @@ > /dev/null &
	#afl-fuzz -i input -o output -Z 2 -S fuzzer2 ./fuzz_test @@ > /dev/null &
	#afl-fuzz -i input -o output -Z 3 -S fuzzer3 ./fuzz_test @@ > /dev/null &
	#afl-fuzz -i input -o output -Z 4 -S fuzzer4 ./fuzz_test @@ > /dev/null &
	#afl-fuzz -i input -o output -Z 5 -S fuzzer5 ./fuzz_test @@ > /dev/null &
	#afl-fuzz -i input -o output -Z 6 -S fuzzer6 ./fuzz_test @@ > /dev/null
