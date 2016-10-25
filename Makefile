test_bloom_list: test_bloom_list.c bloom_list.c bloom_list.h
	clang -Wall -g -o test_bloom_list test_bloom_list.c bloom_list.c murmur3.c -lm 

encode: encode.c bloom_list.c bloom_list.h
	clang -Wall -O3 -o encode encode.c bloom_list.c murmur3.c -lm 

query: query.c bloom_list.c bloom_list.h
	clang -Wall -O3 -o query query.c bloom_list.c murmur3.c -lm 