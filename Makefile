encode: encode.c filter_list.c filter_list.h
	clang -Wall -O3 -o encode encode.c filter_list.c murmur3.c -lm 

query: query.c filter_list.c filter_list.h
	clang -Wall -O3 -o query query.c filter_list.c murmur3.c -lm 