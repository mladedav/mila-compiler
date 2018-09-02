CPP=clang++
LN=clang++
CXXFLAGS=-Wall -pedantic -std=c++11 -g

run: parser binary/inc.o
	./compile_test.sh

binary/inc.o : inc.c
	mkdir -p binary
	clang -Wall -pedantic -g -c -o $@ $<

%.o : %.cpp
	$(CPP) $(CXXFLAGS) `llvm-config --cxxflags` -fexceptions -Wno-unknown-warning-option -c -g -o $@ $<

lexan: lexan.o lexan_test.o
	$(LN) $^ -g -o $@

lexan_test: lexan
	./lexan_test.sh

parser: lexan.o ast.o parser.o parser_test.o
	$(LN) `llvm-config --ldflags --system-libs --libs core` -g $^ -o $@

clean:
	rm parser lexan *.o binary/* 2> /dev/null; true
	rmdir binary

const: parser
	./parser < samples/consts.p 
	llc binary/consts -filetype=obj -o binary/consts.o
	gcc binary/consts.o binary/inc.o -o binary/a.out
	binary/a.out 

input: parser
	./parser < samples/inputOutput.p 
	llc binary/inputOutput -filetype=obj -o binary/inputOutput.o
	gcc binary/inputOutput.o binary/inc.o -o binary/a.out
	binary/a.out 

array: parser
	./parser < samples/arrayMax.p 
	llc binary/arrayMax -filetype=obj -o binary/arrayMax.o
	gcc binary/arrayMax.o binary/inc.o -o binary/a.out
	binary/a.out 

lexan.o: lexan.cpp lexan.h
ast.o: ast.cpp ast.h
parser.o: parser.cpp parser.h lexan.cpp lexan.h ast.cpp ast.h
