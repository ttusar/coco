#!/bin/sh
#$ -j y
#$ -pe smp 2
#$ -cwd
#$ -l h_rt=48:00:00
#$ -l h_vmem=30G
#$ -m bea


module load R


cd ~/gbea/code-postprocessing/gbea
Rscript compute_ela.R rw-gan-mario-lhs.RData rw-gan-mario-lhs-ela.RData 2>&1 > output.txt

