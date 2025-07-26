import requests
from huggingface_hub import snapshot_download
import os
import sys

repo_id = "jinaai/jina-embeddings-v3"
local_dir = "./models"

# Only download the specific files we need
allow_patterns = [
    "onnx/model.onnx",  # Main ONNX model
    "onnx/model.onnx_data",  # External weights file required for inference
    "tokenizer.json", 
    "tokenizer_config.json", 
    "vocab.txt",
    "config.json"
]

# Quick check
resp = requests.head(f"https://huggingface.co/{repo_id}")
if resp.status_code != 200:
    print(f"Error: Cannot access repository `{repo_id}` (status {resp.status_code})")
    sys.exit(1)

# Download only needed files
print(f"Downloading minimal files from {repo_id} ...")
snapshot_download(repo_id=repo_id, local_dir=local_dir, local_dir_use_symlinks=False, allow_patterns=allow_patterns)
print(f"Downloaded repository to {local_dir}")

# Check for ONNX model
onnx_path = os.path.join(local_dir, "onnx", "model.onnx")
if os.path.isfile(onnx_path):
    print("ONNX model found:", onnx_path)
else:
    print("Warning: ONNX file not found in expected path.")

# Check for tokenizer files
tokenizer_files = ["tokenizer.json", "vocab.txt", "tokenizer_config.json"]
found_tokenizer = False
for fname in tokenizer_files:
    fpath = os.path.join(local_dir, fname)
    if os.path.isfile(fpath):
        print("Tokenizer file found:", fpath)
        found_tokenizer = True
if not found_tokenizer:
    print("Warning: No standard tokenizer file found in root. Check subfolders or model card for details.") 