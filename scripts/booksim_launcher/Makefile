DIR=experiments/2020/ISPASS/2020_01_07
RES_DIR=results/2020/ISPASS/2020_01_07/
TRITON_RES_DIR=booksim-unican/scripts/booksim_launcher/results/

exp_launch:
	for file in ${DIR}/*.json; do \
		echo "Makefile: launching file $${file}"; \
		python launcher.py $${file}; \
	done

add_simtime:
	python3 utils/stats_time_inc.py ${RES_DIR}/*/*.csv

down_triton:
	rsync -avz iperez@triton.atc.unican.es:${TRITON_RES_DIR} results
