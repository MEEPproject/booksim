#!/bin/bash

./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_blackscholes.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_blackscholes.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_bodytrack.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_bodytrack.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_cholesky.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_cholesky.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_facesim.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_facesim.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_fft.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fft.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_fluidanimate.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fluidanimate.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_lu_cb.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_cb.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_lu_ncb.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_ncb.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_radiosity.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radiosity.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_radix.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radix.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_raytrace.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_raytrace.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_swaptions.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_swaptions.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_volrend.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_volrend.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_water_nsquared.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_nsquared.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_water_spatial.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 10 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_spatial.out_500000_packets_x10_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 10 1
./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 10 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 10 10
./booksim hybrid_buf_size_50.cfg > hybrid_blackscholes.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 10 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_blackscholes.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 10 10
./booksim hybrid_buf_size_50.cfg > hybrid_bodytrack.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 10 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_bodytrack.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 10 10
./booksim hybrid_buf_size_50.cfg > hybrid_cholesky.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 10 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_cholesky.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 10 10
./booksim hybrid_buf_size_50.cfg > hybrid_facesim.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 10 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_facesim.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 10 10
./booksim hybrid_buf_size_50.cfg > hybrid_fft.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 10 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fft.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 10 10
./booksim hybrid_buf_size_50.cfg > hybrid_fluidanimate.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 10 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fluidanimate.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 10 10
./booksim hybrid_buf_size_50.cfg > hybrid_lu_cb.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 10 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_cb.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 10 10
./booksim hybrid_buf_size_50.cfg > hybrid_lu_ncb.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 10 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_ncb.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 10 10
./booksim hybrid_buf_size_50.cfg > hybrid_radiosity.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 10 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radiosity.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 10 10
./booksim hybrid_buf_size_50.cfg > hybrid_radix.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 10 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radix.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 10 10
./booksim hybrid_buf_size_50.cfg > hybrid_raytrace.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 10 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_raytrace.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 10 10
./booksim hybrid_buf_size_50.cfg > hybrid_swaptions.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 10 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_swaptions.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 10 10
./booksim hybrid_buf_size_50.cfg > hybrid_volrend.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 10 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_volrend.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 10 10
./booksim hybrid_buf_size_50.cfg > hybrid_water_nsquared.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 10 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_nsquared.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 10 10
./booksim hybrid_buf_size_50.cfg > hybrid_water_spatial.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 10 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_spatial.out_500000_packets_x10_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 10 10
./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 10 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 10 20
./booksim hybrid_buf_size_50.cfg > hybrid_blackscholes.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 10 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_blackscholes.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 10 20
./booksim hybrid_buf_size_50.cfg > hybrid_bodytrack.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 10 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_bodytrack.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 10 20
./booksim hybrid_buf_size_50.cfg > hybrid_cholesky.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 10 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_cholesky.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 10 20
./booksim hybrid_buf_size_50.cfg > hybrid_facesim.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 10 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_facesim.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 10 20
./booksim hybrid_buf_size_50.cfg > hybrid_fft.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 10 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fft.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 10 20
./booksim hybrid_buf_size_50.cfg > hybrid_fluidanimate.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 10 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fluidanimate.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 10 20
./booksim hybrid_buf_size_50.cfg > hybrid_lu_cb.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 10 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_cb.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 10 20
./booksim hybrid_buf_size_50.cfg > hybrid_lu_ncb.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 10 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_ncb.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 10 20
./booksim hybrid_buf_size_50.cfg > hybrid_radiosity.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 10 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radiosity.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 10 20
./booksim hybrid_buf_size_50.cfg > hybrid_radix.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 10 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radix.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 10 20
./booksim hybrid_buf_size_50.cfg > hybrid_raytrace.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 10 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_raytrace.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 10 20
./booksim hybrid_buf_size_50.cfg > hybrid_swaptions.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 10 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_swaptions.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 10 20
./booksim hybrid_buf_size_50.cfg > hybrid_volrend.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 10 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_volrend.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 10 20
./booksim hybrid_buf_size_50.cfg > hybrid_water_nsquared.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 10 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_nsquared.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 10 20
./booksim hybrid_buf_size_50.cfg > hybrid_water_spatial.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 10 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_spatial.out_500000_packets_x10_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 10 20
#./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 10 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 10 40
#./booksim hybrid_buf_size_50.cfg > hybrid_blackscholes.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 10 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_blackscholes.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 10 40
#./booksim hybrid_buf_size_50.cfg > hybrid_bodytrack.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 10 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_bodytrack.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 10 40
#./booksim hybrid_buf_size_50.cfg > hybrid_cholesky.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 10 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_cholesky.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 10 40
#./booksim hybrid_buf_size_50.cfg > hybrid_facesim.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 10 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_facesim.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 10 40
#./booksim hybrid_buf_size_50.cfg > hybrid_fft.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 10 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fft.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 10 40
#./booksim hybrid_buf_size_50.cfg > hybrid_fluidanimate.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 10 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fluidanimate.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 10 40
#./booksim hybrid_buf_size_50.cfg > hybrid_lu_cb.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 10 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_cb.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 10 40
#./booksim hybrid_buf_size_50.cfg > hybrid_lu_ncb.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 10 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_ncb.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 10 40
#./booksim hybrid_buf_size_50.cfg > hybrid_radiosity.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 10 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radiosity.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 10 40
#./booksim hybrid_buf_size_50.cfg > hybrid_radix.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 10 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radix.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 10 40
#./booksim hybrid_buf_size_50.cfg > hybrid_raytrace.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 10 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_raytrace.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 10 40
#./booksim hybrid_buf_size_50.cfg > hybrid_swaptions.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 10 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_swaptions.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 10 40
#./booksim hybrid_buf_size_50.cfg > hybrid_volrend.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 10 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_volrend.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 10 40
#./booksim hybrid_buf_size_50.cfg > hybrid_water_nsquared.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 10 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_nsquared.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 10 40
#./booksim hybrid_buf_size_50.cfg > hybrid_water_spatial.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 10 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_spatial.out_500000_packets_x10_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 10 40
#./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 10 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 10 80
#./booksim hybrid_buf_size_50.cfg > hybrid_blackscholes.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 10 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_blackscholes.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 10 80
#./booksim hybrid_buf_size_50.cfg > hybrid_bodytrack.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 10 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_bodytrack.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 10 80
#./booksim hybrid_buf_size_50.cfg > hybrid_cholesky.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 10 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_cholesky.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 10 80
#./booksim hybrid_buf_size_50.cfg > hybrid_facesim.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 10 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_facesim.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 10 80
#./booksim hybrid_buf_size_50.cfg > hybrid_fft.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 10 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fft.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 10 80
#./booksim hybrid_buf_size_50.cfg > hybrid_fluidanimate.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 10 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fluidanimate.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 10 80
#./booksim hybrid_buf_size_50.cfg > hybrid_lu_cb.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 10 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_cb.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 10 80
#./booksim hybrid_buf_size_50.cfg > hybrid_lu_ncb.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 10 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_ncb.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 10 80
#./booksim hybrid_buf_size_50.cfg > hybrid_radiosity.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 10 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radiosity.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 10 80
#./booksim hybrid_buf_size_50.cfg > hybrid_radix.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 10 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radix.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 10 80
#./booksim hybrid_buf_size_50.cfg > hybrid_raytrace.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 10 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_raytrace.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 10 80
#./booksim hybrid_buf_size_50.cfg > hybrid_swaptions.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 10 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_swaptions.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 10 80
#./booksim hybrid_buf_size_50.cfg > hybrid_volrend.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 10 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_volrend.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 10 80
#./booksim hybrid_buf_size_50.cfg > hybrid_water_nsquared.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 10 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_nsquared.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 10 80
#./booksim hybrid_buf_size_50.cfg > hybrid_water_spatial.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 10 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_spatial.out_500000_packets_x10_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 10 80
./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 5 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 5 1
./booksim hybrid_buf_size_50.cfg > hybrid_blackscholes.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 5 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_blackscholes.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 5 1
./booksim hybrid_buf_size_50.cfg > hybrid_bodytrack.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 5 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_bodytrack.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 5 1
./booksim hybrid_buf_size_50.cfg > hybrid_cholesky.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 5 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_cholesky.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 5 1
./booksim hybrid_buf_size_50.cfg > hybrid_facesim.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 5 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_facesim.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 5 1
./booksim hybrid_buf_size_50.cfg > hybrid_fft.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 5 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fft.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 5 1
./booksim hybrid_buf_size_50.cfg > hybrid_fluidanimate.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 5 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fluidanimate.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 5 1
./booksim hybrid_buf_size_50.cfg > hybrid_lu_cb.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 5 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_cb.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 5 1
./booksim hybrid_buf_size_50.cfg > hybrid_lu_ncb.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 5 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_ncb.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 5 1
./booksim hybrid_buf_size_50.cfg > hybrid_radiosity.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 5 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radiosity.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 5 1
./booksim hybrid_buf_size_50.cfg > hybrid_radix.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 5 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radix.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 5 1
./booksim hybrid_buf_size_50.cfg > hybrid_raytrace.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 5 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_raytrace.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 5 1
./booksim hybrid_buf_size_50.cfg > hybrid_swaptions.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 5 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_swaptions.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 5 1
./booksim hybrid_buf_size_50.cfg > hybrid_volrend.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 5 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_volrend.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 5 1
./booksim hybrid_buf_size_50.cfg > hybrid_water_nsquared.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 5 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_nsquared.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 5 1
./booksim hybrid_buf_size_50.cfg > hybrid_water_spatial.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 5 1
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_spatial.out_500000_packets_x5_buf_size_50_seed_1 &
./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 5 1
./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 5 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 5 10
./booksim hybrid_buf_size_50.cfg > hybrid_blackscholes.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 5 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_blackscholes.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 5 10
./booksim hybrid_buf_size_50.cfg > hybrid_bodytrack.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 5 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_bodytrack.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 5 10
./booksim hybrid_buf_size_50.cfg > hybrid_cholesky.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 5 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_cholesky.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 5 10
./booksim hybrid_buf_size_50.cfg > hybrid_facesim.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 5 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_facesim.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 5 10
./booksim hybrid_buf_size_50.cfg > hybrid_fft.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 5 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fft.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 5 10
./booksim hybrid_buf_size_50.cfg > hybrid_fluidanimate.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 5 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fluidanimate.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 5 10
./booksim hybrid_buf_size_50.cfg > hybrid_lu_cb.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 5 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_cb.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 5 10
./booksim hybrid_buf_size_50.cfg > hybrid_lu_ncb.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 5 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_ncb.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 5 10
./booksim hybrid_buf_size_50.cfg > hybrid_radiosity.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 5 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radiosity.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 5 10
./booksim hybrid_buf_size_50.cfg > hybrid_radix.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 5 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radix.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 5 10
./booksim hybrid_buf_size_50.cfg > hybrid_raytrace.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 5 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_raytrace.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 5 10
./booksim hybrid_buf_size_50.cfg > hybrid_swaptions.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 5 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_swaptions.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 5 10
./booksim hybrid_buf_size_50.cfg > hybrid_volrend.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 5 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_volrend.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 5 10
./booksim hybrid_buf_size_50.cfg > hybrid_water_nsquared.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 5 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_nsquared.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 5 10
./booksim hybrid_buf_size_50.cfg > hybrid_water_spatial.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 5 10
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_spatial.out_500000_packets_x5_buf_size_50_seed_10 &
./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 5 10
./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 5 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 5 20
./booksim hybrid_buf_size_50.cfg > hybrid_blackscholes.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 5 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_blackscholes.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 5 20
./booksim hybrid_buf_size_50.cfg > hybrid_bodytrack.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 5 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_bodytrack.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 5 20
./booksim hybrid_buf_size_50.cfg > hybrid_cholesky.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 5 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_cholesky.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 5 20
./booksim hybrid_buf_size_50.cfg > hybrid_facesim.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 5 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_facesim.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 5 20
./booksim hybrid_buf_size_50.cfg > hybrid_fft.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 5 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fft.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 5 20
./booksim hybrid_buf_size_50.cfg > hybrid_fluidanimate.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 5 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fluidanimate.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 5 20
./booksim hybrid_buf_size_50.cfg > hybrid_lu_cb.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 5 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_cb.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 5 20
./booksim hybrid_buf_size_50.cfg > hybrid_lu_ncb.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 5 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_ncb.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 5 20
./booksim hybrid_buf_size_50.cfg > hybrid_radiosity.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 5 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radiosity.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 5 20
./booksim hybrid_buf_size_50.cfg > hybrid_radix.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 5 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radix.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 5 20
./booksim hybrid_buf_size_50.cfg > hybrid_raytrace.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 5 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_raytrace.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 5 20
./booksim hybrid_buf_size_50.cfg > hybrid_swaptions.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 5 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_swaptions.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 5 20
./booksim hybrid_buf_size_50.cfg > hybrid_volrend.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 5 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_volrend.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 5 20
./booksim hybrid_buf_size_50.cfg > hybrid_water_nsquared.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 5 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_nsquared.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 5 20
./booksim hybrid_buf_size_50.cfg > hybrid_water_spatial.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 5 20
./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_spatial.out_500000_packets_x5_buf_size_50_seed_20 &
./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 5 20
#./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 5 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 5 40
#./booksim hybrid_buf_size_50.cfg > hybrid_blackscholes.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 5 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_blackscholes.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 5 40
#./booksim hybrid_buf_size_50.cfg > hybrid_bodytrack.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 5 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_bodytrack.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 5 40
#./booksim hybrid_buf_size_50.cfg > hybrid_cholesky.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 5 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_cholesky.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 5 40
#./booksim hybrid_buf_size_50.cfg > hybrid_facesim.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 5 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_facesim.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 5 40
#./booksim hybrid_buf_size_50.cfg > hybrid_fft.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 5 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fft.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 5 40
#./booksim hybrid_buf_size_50.cfg > hybrid_fluidanimate.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 5 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fluidanimate.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 5 40
#./booksim hybrid_buf_size_50.cfg > hybrid_lu_cb.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 5 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_cb.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 5 40
#./booksim hybrid_buf_size_50.cfg > hybrid_lu_ncb.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 5 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_ncb.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 5 40
#./booksim hybrid_buf_size_50.cfg > hybrid_radiosity.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 5 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radiosity.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 5 40
#./booksim hybrid_buf_size_50.cfg > hybrid_radix.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 5 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radix.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 5 40
#./booksim hybrid_buf_size_50.cfg > hybrid_raytrace.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 5 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_raytrace.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 5 40
#./booksim hybrid_buf_size_50.cfg > hybrid_swaptions.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 5 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_swaptions.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 5 40
#./booksim hybrid_buf_size_50.cfg > hybrid_volrend.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 5 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_volrend.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 5 40
#./booksim hybrid_buf_size_50.cfg > hybrid_water_nsquared.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 5 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_nsquared.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 5 40
#./booksim hybrid_buf_size_50.cfg > hybrid_water_spatial.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 5 40
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_spatial.out_500000_packets_x5_buf_size_50_seed_40 &
#./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 5 40
#./booksim hybrid_buf_size_50.cfg > hybrid_barnes.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 5 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_barnes.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/barnes.model 1000000000 1 500000 5 80
#./booksim hybrid_buf_size_50.cfg > hybrid_blackscholes.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 5 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_blackscholes.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/blackscholes.model 1000000000 1 500000 5 80
#./booksim hybrid_buf_size_50.cfg > hybrid_bodytrack.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 5 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_bodytrack.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/bodytrack.model 1000000000 1 500000 5 80
#./booksim hybrid_buf_size_50.cfg > hybrid_cholesky.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 5 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_cholesky.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/cholesky.model 1000000000 1 500000 5 80
#./booksim hybrid_buf_size_50.cfg > hybrid_facesim.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 5 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_facesim.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/facesim.model 1000000000 1 500000 5 80
#./booksim hybrid_buf_size_50.cfg > hybrid_fft.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 5 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fft.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/fft.model 1000000000 1 500000 5 80
#./booksim hybrid_buf_size_50.cfg > hybrid_fluidanimate.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 5 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_fluidanimate.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/fluidanimate.model 1000000000 1 500000 5 80
#./booksim hybrid_buf_size_50.cfg > hybrid_lu_cb.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 5 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_cb.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/lu_cb.model 1000000000 1 500000 5 80
#./booksim hybrid_buf_size_50.cfg > hybrid_lu_ncb.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 5 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_lu_ncb.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/lu_ncb.model 1000000000 1 500000 5 80
#./booksim hybrid_buf_size_50.cfg > hybrid_radiosity.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 5 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radiosity.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/radiosity.model 1000000000 1 500000 5 80
#./booksim hybrid_buf_size_50.cfg > hybrid_radix.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 5 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_radix.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/radix.model 1000000000 1 500000 5 80
#./booksim hybrid_buf_size_50.cfg > hybrid_raytrace.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 5 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_raytrace.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/raytrace.model 1000000000 1 500000 5 80
#./booksim hybrid_buf_size_50.cfg > hybrid_swaptions.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 5 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_swaptions.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/swaptions.model 1000000000 1 500000 5 80
#./booksim hybrid_buf_size_50.cfg > hybrid_volrend.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 5 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_volrend.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/volrend.model 1000000000 1 500000 5 80
#./booksim hybrid_buf_size_50.cfg > hybrid_water_nsquared.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 5 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_nsquared.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/water_nsquared.model 1000000000 1 500000 5 80
#./booksim hybrid_buf_size_50.cfg > hybrid_water_spatial.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 5 80
#./booksim bypass_arb_buf_size_50.cfg > bypass_arb_water_spatial.out_500000_packets_x5_buf_size_50_seed_80 &
#./tgen ../../synfull-isca/generated-models/water_spatial.model 1000000000 1 500000 5 80
