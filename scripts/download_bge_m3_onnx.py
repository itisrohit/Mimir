from huggingface_hub import snapshot_download
import os

repo_id = "aapot/bge-m3-onnx"
local_dir = "./models/bge-m3-onnx"

print(f"🔽 Downloading full repo from `{repo_id}` ...")
snapshot_download(
    repo_id=repo_id,
    local_dir=local_dir,
    local_dir_use_symlinks=False
)
print(f"✅ Full model downloaded to: {local_dir}")

# Optional: verify key files exist
required_files = [
    "model.onnx",
    "model.onnx.data",
    "tokenizer.json",
    "tokenizer_config.json",
    "config.json"
]

print("\n🔍 Verifying key files...")
for fname in required_files:
    path = os.path.join(local_dir, fname)
    if os.path.exists(path):
        print(f"✔ Found: {fname}")
    else:
        print(f"⚠ Missing: {fname}") 