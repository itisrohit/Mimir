#!/usr/bin/env python3
"""
Download Nomic model weights using SentenceTransformer with trust_remote_code=True.
This ensures all necessary files including custom code and pooling configs are downloaded.
"""
from sentence_transformers import SentenceTransformer
import os
import sys

MODEL_NAME = "nomic-ai/nomic-embed-text-v2-moe"
CACHE_DIR = "models/nomic-ai--nomic-embed-text-v2-moe"

print(f"🔍 Downloading SentenceTransformer model: {MODEL_NAME}")
print(f"📁 Cache directory: {CACHE_DIR}")

try:
    # Download using SentenceTransformer with trust_remote_code=True
    print("🚀 Loading model with trust_remote_code=True...")
    model = SentenceTransformer(MODEL_NAME, cache_folder=CACHE_DIR, trust_remote_code=True)
    print("✅ Model loaded successfully!")
    
    # Show model info
    print(f"📊 Model dimension: {model.get_sentence_embedding_dimension()}")
    print(f"🔧 Model device: {model.device}")
    
    # Show where the weights are
    print("\n🔍 Searching for model weight files...")
    found_weights = []
    
    # Search in our cache directory
    for root, dirs, files in os.walk(CACHE_DIR):
        for f in files:
            if f.endswith(".bin") or f.endswith(".safetensors"):
                full_path = os.path.join(root, f)
                found_weights.append(full_path)
                print(f"✅ Found weights: {full_path}")
    
    if not found_weights:
        print("⚠️  No .bin or .safetensors files found in cache directory")
        print("🔍 Checking HuggingFace default cache...")
        
        # Check default HuggingFace cache locations
        possible_cache_dirs = [
            os.path.expanduser("~/.cache/huggingface/hub"),
            os.path.expanduser("~/Library/Caches/huggingface/hub"),  # macOS
            "/tmp/huggingface/hub"
        ]
        
        for cache_dir in possible_cache_dirs:
            if os.path.exists(cache_dir):
                print(f"📁 Checking cache: {cache_dir}")
                for root, dirs, files in os.walk(cache_dir):
                    for f in files:
                        if f.endswith(".bin") or f.endswith(".safetensors"):
                            if "nomic" in root.lower():
                                full_path = os.path.join(root, f)
                                found_weights.append(full_path)
                                print(f"✅ Found weights in cache: {full_path}")
    
    if found_weights:
        print(f"\n🎉 Success! Found {len(found_weights)} weight file(s)")
        print("📋 Weight files:")
        for weight_file in found_weights:
            print(f"   - {weight_file}")
            
        # Copy the first weight file to our models directory if not already there
        if not any(CACHE_DIR in wf for wf in found_weights):
            print(f"\n📋 Copying weight file to {CACHE_DIR}...")
            import shutil
            os.makedirs(CACHE_DIR, exist_ok=True)
            source_file = found_weights[0]
            dest_file = os.path.join(CACHE_DIR, os.path.basename(source_file))
            shutil.copy2(source_file, dest_file)
            print(f"✅ Copied: {dest_file}")
            
    else:
        print("\n❌ No weight files found. This might indicate an issue with the model download.")
        print("💡 Try running the script again or check your internet connection.")
        print("🔍 Let's check what files are actually in the cache directory:")
        
        if os.path.exists(CACHE_DIR):
            print(f"\n📁 Files in {CACHE_DIR}:")
            for root, dirs, files in os.walk(CACHE_DIR):
                level = root.replace(CACHE_DIR, '').count(os.sep)
                indent = ' ' * 2 * level
                print(f"{indent}{os.path.basename(root)}/")
                subindent = ' ' * 2 * (level + 1)
                for f in files:
                    print(f"{subindent}{f}")
        
        sys.exit(1)
        
except Exception as e:
    print(f"❌ Error downloading model: {e}")
    print("💡 This might be due to:")
    print("   - Network connectivity issues")
    print("   - Insufficient disk space")
    print("   - Model repository access issues")
    sys.exit(1)

print("\n🎯 Next step: Run scripts/export_nomic_to_onnx.py to convert to ONNX format") 