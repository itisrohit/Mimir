import onnxruntime as ort
from transformers import AutoTokenizer
import numpy as np
import os

model_dir = "./models/bge-m3-onnx"
onnx_path = os.path.join(model_dir, "model.onnx")

# Load tokenizer
tokenizer = AutoTokenizer.from_pretrained(model_dir)

# Load ONNX model
ort_session = ort.InferenceSession(onnx_path)

# Prepare input
text = "BGE M3 is an embedding model supporting dense retrieval, lexical matching and multi-vector interaction."
inputs = tokenizer(text, padding="longest", return_tensors="np")
inputs_onnx = {k: v for k, v in inputs.items()}

# Run inference
outputs = ort_session.run(None, inputs_onnx)

# Print output shapes
print("Dense embedding shape:", outputs[0].shape)
print("Sparse embedding shape:", outputs[1].shape)
print("ColBERT embedding shape:", outputs[2].shape)

# Optionally, print a snippet of the dense embedding
print("Dense embedding (first 5 values):", outputs[0][0][:5]) 