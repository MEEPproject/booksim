#!/bin/bash

time ./booksim hybrid_buf_size_50.cfg synfull_socket=./socket_x1 > hybrid_raytrace.out_steady_packets_x1_buf_size_50_seed_1 &
./tgen -m ../../synfull-isca/generated-models/raytrace.model -c 1000000000 -s 1 -f 1 -r 1 -t ./socket_x1
time ./booksim hybrid_buf_size_50.cfg synfull_socket=./socket_x1 > hybrid_raytrace.out_steady_packets_x1_buf_size_50_seed_2 &
./tgen -m ../../synfull-isca/generated-models/raytrace.model -c 1000000000 -s 1 -f 1 -r 2 -t ./socket_x1
time ./booksim hybrid_buf_size_50.cfg synfull_socket=./socket_x1 > hybrid_raytrace.out_steady_packets_x1_buf_size_50_seed_3 &
./tgen -m ../../synfull-isca/generated-models/raytrace.model -c 1000000000 -s 1 -f 1 -r 3 -t ./socket_x1
time ./booksim hybrid_buf_size_50.cfg synfull_socket=./socket_x1 > hybrid_raytrace.out_steady_packets_x1_buf_size_50_seed_4 &
./tgen -m ../../synfull-isca/generated-models/raytrace.model -c 1000000000 -s 1 -f 1 -r 4 -t ./socket_x1
time ./booksim hybrid_buf_size_50.cfg synfull_socket=./socket_x1 > hybrid_raytrace.out_steady_packets_x1_buf_size_50_seed_5 &
./tgen -m ../../synfull-isca/generated-models/raytrace.model -c 1000000000 -s 1 -f 1 -r 5 -t ./socket_x1
time ./booksim bypass_arb_buf_size_50.cfg synfull_socket=./socket_x1 > bypass_arb_raytrace.out_steady_packets_x1_buf_size_50_seed_1 &
./tgen -m ../../synfull-isca/generated-models/raytrace.model -c 1000000000 -s 1 -f 1 -r 1 -t ./socket_x1
time ./booksim bypass_arb_buf_size_50.cfg synfull_socket=./socket_x1 > bypass_arb_raytrace.out_steady_packets_x1_buf_size_50_seed_2 &
./tgen -m ../../synfull-isca/generated-models/raytrace.model -c 1000000000 -s 1 -f 1 -r 2 -t ./socket_x1
time ./booksim bypass_arb_buf_size_50.cfg synfull_socket=./socket_x1 > bypass_arb_raytrace.out_steady_packets_x1_buf_size_50_seed_3 &
./tgen -m ../../synfull-isca/generated-models/raytrace.model -c 1000000000 -s 1 -f 1 -r 3 -t ./socket_x1
time ./booksim bypass_arb_buf_size_50.cfg synfull_socket=./socket_x1 > bypass_arb_raytrace.out_steady_packets_x1_buf_size_50_seed_4 &
./tgen -m ../../synfull-isca/generated-models/raytrace.model -c 1000000000 -s 1 -f 1 -r 4 -t ./socket_x1
time ./booksim bypass_arb_buf_size_50.cfg synfull_socket=./socket_x1 > bypass_arb_raytrace.out_steady_packets_x1_buf_size_50_seed_5 &
./tgen -m ../../synfull-isca/generated-models/raytrace.model -c 1000000000 -s 1 -f 1 -r 5 -t ./socket_x1
