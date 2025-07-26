#!/usr/bin/env python3
"""
Export Nomic SentenceTransformer (transformer only) to ONNX for C++ inference.
This script exports the base transformer model and provides pooling config for C++ implementation.
"""
import os
import torch
import json
from sentence_transformers import SentenceTransformer
from transformers import AutoTokenizer

MODEL_NAME = "nomic-ai/nomic-embed-text-v2-moe"
CACHE_DIR = "models/nomic-ai--nomic-embed-text-v2-moe"
ONNX_PATH = "models/nomic-embed-text-v2-moe.onnx"
TOKENIZER_INFO_PATH = "models/tokenizer_info.json"

print(f"🔍 Model: {MODEL_NAME}")
print(f"📁 Cache directory: {CACHE_DIR}")
print(f"💾 ONNX output: {ONNX_PATH}")

# Load SentenceTransformer model
print("🚀 Loading SentenceTransformer model...")
model = SentenceTransformer(MODEL_NAME, cache_folder=CACHE_DIR, trust_remote_code=True)
print("✅ Model loaded successfully!")

# Get the underlying transformer model
print("🔧 Extracting transformer model...")
transformer_model = model._first_module().auto_model
transformer_model.eval()
print(f"📊 Transformer model type: {type(transformer_model).__name__}")

# Force model to CPU
transformer_model = transformer_model.cpu()

# Load tokenizer
print("🔤 Loading tokenizer...")
tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME, cache_dir=CACHE_DIR, trust_remote_code=True)
print("✅ Tokenizer loaded!")

# Get pooling configuration
print("🔍 Extracting pooling configuration...")
pooling_config = None
try:
    # Try to get pooling config from the model
    if hasattr(model._first_module(), 'pooling'):
        pooling_module = model._first_module().pooling
        pooling_config = {
            "type": type(pooling_module).__name__,
            "word_embedding_dimension": getattr(pooling_module, 'word_embedding_dimension', None),
            "pooling_cls_token": getattr(pooling_module, 'pooling_cls_token', False),
            "pooling_mean_token": getattr(pooling_module, 'pooling_mean_token', True),
            "pooling_max_token": getattr(pooling_module, 'pooling_max_token', False),
            "pooling_mean_sqrt_len_tok": getattr(pooling_module, 'pooling_mean_sqrt_len_tok', False),
        }
        print(f"✅ Pooling config extracted: {pooling_config}")
    else:
        print("⚠️  Could not extract pooling config from model")
except Exception as e:
    print(f"⚠️  Error extracting pooling config: {e}")

# Prepare dummy input for ONNX export
print("📝 Preparing dummy input...")
dummy_text = "This is a test sentence for ONNX export."
dummy_input = tokenizer(
    dummy_text, 
    return_tensors="pt", 
    max_length=512, 
    padding="max_length", 
    truncation=True
)
print(f"📊 Input shape: {dummy_input['input_ids'].shape}")

# Move dummy input to CPU
for k in dummy_input:
    dummy_input[k] = dummy_input[k].cpu()

# Export to ONNX
print(f"💾 Exporting to ONNX: {ONNX_PATH}")
try:
    torch.onnx.export(
        transformer_model,
        (dummy_input['input_ids'], dummy_input['attention_mask']),
        ONNX_PATH,
        input_names=['input_ids', 'attention_mask'],
        output_names=['last_hidden_state'],
        dynamic_axes={
            'input_ids': {0: 'batch_size', 1: 'sequence'},
            'attention_mask': {0: 'batch_size', 1: 'sequence'},
            'last_hidden_state': {0: 'batch_size', 1: 'sequence'}
        },
        opset_version=17,
        do_constant_folding=True,
        export_params=True
    )
    print("✅ ONNX export complete!")
except Exception as e:
    print(f"❌ ONNX export failed: {e}")
    raise

# Save tokenizer info for C++
print(f"💾 Saving tokenizer info: {TOKENIZER_INFO_PATH}")
tokenizer_info = {
    "model_name": MODEL_NAME,
    "vocab_size": tokenizer.vocab_size,
    "max_length": 512,
    "pad_token_id": tokenizer.pad_token_id,
    "unk_token_id": tokenizer.unk_token_id,
    "cls_token_id": tokenizer.cls_token_id,
    "sep_token_id": tokenizer.sep_token_id,
    "pooling_config": pooling_config,
    "embedding_dimension": model.get_sentence_embedding_dimension()
}

with open(TOKENIZER_INFO_PATH, 'w') as f:
    json.dump(tokenizer_info, f, indent=2)
print("✅ Tokenizer info saved!")

# Test the ONNX model
print("🧪 Testing ONNX model...")
try:
    import onnxruntime as ort
    
    # Create ONNX Runtime session
    session = ort.InferenceSession(ONNX_PATH)
    
    # Test with dummy input
    ort_inputs = {
        'input_ids': dummy_input['input_ids'].numpy(),
        'attention_mask': dummy_input['attention_mask'].numpy()
    }
    
    outputs = session.run(None, ort_inputs)
    print(f"✅ ONNX test successful! Output shape: {outputs[0].shape}")
    
except ImportError:
    print("⚠️  onnxruntime not installed, skipping ONNX test")
except Exception as e:
    print(f"⚠️  ONNX test failed: {e}")

print("\n🎉 Export completed successfully!")
print(f"📁 ONNX model: {ONNX_PATH}")
print(f"📁 Tokenizer info: {TOKENIZER_INFO_PATH}")
print("\n📋 Next steps:")
print("1. Implement pooling logic in C++ based on the pooling config")
print("2. Build C++ project with ONNX Runtime")
print("3. Test end-to-end embedding pipeline") 