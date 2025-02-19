#! /usr/bin/env zsh
# SBATCH --job-name=task2
# SBATCH --output=task2_output_%j.txt
# SBATCH --error=task2_error_%j.txt
# SBATCH --time=00:15:00          
# SBATCH --ntasks=1               
# SBATCH --gpus-per-task=1
# SBATCH --partition=instruction

python my_parser.py ebay_data/items-*.json
