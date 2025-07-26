#!/usr/bin/env python3
"""
Compare HuggingFace tokenization with C++ output to identify shape mismatch
"""

from transformers import AutoTokenizer
import json
import sys

def compare_tokenization():
    print("🔍 Comparing HuggingFace vs C++ tokenization...")
    
    # Load the same tokenizer that C++ uses
    tokenizer = AutoTokenizer.from_pretrained("./models/bge-m3-onnx")
    
    # Test text (same as C++ test)
    test_text = "BGE M3 is an embedding model supporting dense retrieval, lexical matching and multi-vector interaction."
    print("\n📊 HuggingFace Tokenization Result:")
    print("=" * 50)
    print(f"\nText: {test_text}")
    # Get tokenization
    tokens = tokenizer(test_text, return_tensors="pt")
    input_ids = tokens["input_ids"][0].tolist()
    attention_mask = tokens["attention_mask"][0].tolist()
    print(f"  Input IDs: {input_ids}")
    print(f"  Attention Mask: {attention_mask}")
    print(f"  Length: {len(input_ids)}")
    # Decode to verify
    decoded = tokenizer.decode(input_ids)
    print(f"  Decoded: {decoded}")
    # Check for special tokens
    cls_token_id = tokenizer.cls_token_id
    sep_token_id = tokenizer.sep_token_id
    pad_token_id = tokenizer.pad_token_id
    print(f"  Special tokens - CLS: {cls_token_id}, SEP: {sep_token_id}, PAD: {pad_token_id}")
    # Check if tokens start/end with special tokens
    if input_ids and input_ids[0] == cls_token_id:
        print(f"  ✅ Starts with CLS token")
    if input_ids and input_ids[-1] == sep_token_id:
        print(f"  ✅ Ends with SEP token")
    print("\n💡 Expected C++ output should match these token IDs exactly!")
    print("   The shape mismatch is likely due to:")
    print("   1. Missing CLS/SEP tokens")
    print("   2. Different padding behavior")
    print("   3. Different tokenization algorithm")

if __name__ == "__main__":
    compare_tokenization() 