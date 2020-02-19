#!/bin/bash

time ./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_steady_packets_x5_buf_size_50_seed_1 &
./tgen -m ../../synfull-isca/generated-models/barnes.model -c 1000000000 -s 1 -f 5 -r 1
time ./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_steady_packets_x5_buf_size_50_seed_2 &
./tgen -m ../../synfull-isca/generated-models/barnes.model -c 1000000000 -s 1 -f 5 -r 2
time ./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_steady_packets_x5_buf_size_50_seed_3 &
./tgen -m ../../synfull-isca/generated-models/barnes.model -c 1000000000 -s 1 -f 5 -r 3
time ./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_steady_packets_x5_buf_size_50_seed_4 &
./tgen -m ../../synfull-isca/generated-models/barnes.model -c 1000000000 -s 1 -f 5 -r 4
time ./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_steady_packets_x5_buf_size_50_seed_5 &
./tgen -m ../../synfull-isca/generated-models/barnes.model -c 1000000000 -s 1 -f 5 -r 5
time ./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_steady_packets_x5_buf_size_50_seed_1 &
./tgen -m ../../synfull-isca/generated-models/barnes.model -c 1000000000 -s 1 -f 5 -r 1
time ./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_steady_packets_x5_buf_size_50_seed_2 &
./tgen -m ../../synfull-isca/generated-models/barnes.model -c 1000000000 -s 1 -f 5 -r 2
time ./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_steady_packets_x5_buf_size_50_seed_3 &
./tgen -m ../../synfull-isca/generated-models/barnes.model -c 1000000000 -s 1 -f 5 -r 3
time ./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_steady_packets_x5_buf_size_50_seed_4 &
./tgen -m ../../synfull-isca/generated-models/barnes.model -c 1000000000 -s 1 -f 5 -r 4
time ./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_steady_packets_x5_buf_size_50_seed_5 &
./tgen -m ../../synfull-isca/generated-models/barnes.model -c 1000000000 -s 1 -f 5 -r 5
