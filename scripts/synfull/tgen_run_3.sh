#!/bin/bash

./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_2000000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 2000000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_2000000_packets_x10_buf_size_50_seed_2 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 2000000 10 2
./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_2000000_packets_x10_buf_size_50_seed_3 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 2000000 10 3
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_2000000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 2000000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_2000000_packets_x10_buf_size_50_seed_2 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 2000000 10 2
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_2000000_packets_x10_buf_size_50_seed_3 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 2000000 10 3
