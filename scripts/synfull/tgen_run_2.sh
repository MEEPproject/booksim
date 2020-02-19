#!/bin/bash

./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_1000000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 1000000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_1000000_packets_x10_buf_size_50_seed_2 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 1000000 10 2
./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_1000000_packets_x10_buf_size_50_seed_3 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 1000000 10 3
