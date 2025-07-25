#!/bin/bash
set -e

python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
make clean && make
make embedding-server &
sleep 10  # Give server time to start
bash scripts/test_embedding_pipeline.sh
bash scripts/test_ci.sh 