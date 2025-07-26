import onnxruntime as ort
import numpy as np
from onnxruntime_extensions import get_library_path
import os

MODEL_DIR = os.path.join(os.path.dirname(__file__), '../models/bge-m3-onnx')
TOKENIZER_ONNX_PATH = os.path.join(MODEL_DIR, 'tokenizer.onnx')
MODEL_PATH = os.path.join(MODEL_DIR, 'model.onnx')
MODEL_DATA_PATH = os.path.join(MODEL_DIR, 'model.onnx.data')  # Not used directly, but must be present

# Initialize ONNX Runtime session options
sess_options = ort.SessionOptions()
sess_options.register_custom_ops_library(get_library_path())

# Load ONNX tokenizer
print('Loading ONNX tokenizer...')
tokenizer_session = ort.InferenceSession(TOKENIZER_ONNX_PATH, sess_options=sess_options)

# Load ONNX embedding model
print('Loading ONNX embedding model...')
model_session = ort.InferenceSession(MODEL_PATH)

# Prepare input
text = "BGE M3 is an embedding model supporting dense retrieval, lexical matching and multi-vector interaction."

# Run tokenizer
print('Running ONNX tokenizer...')
tokenizer_outputs = tokenizer_session.run(None, {"inputs": np.array([text])})
tokens, _, token_indices = tokenizer_outputs

# Prepare model inputs (sort tokens by token_indices)
token_pairs = list(zip(token_indices, tokens))
token_pairs.sort()
ordered_tokens = [pair[1] for pair in token_pairs]
input_ids = np.array([ordered_tokens], dtype=np.int64)
attention_mask = np.ones_like(input_ids, dtype=np.int64)

# Run embedding model
print('Running ONNX embedding model...')
outputs = model_session.run(None, {
    "input_ids": input_ids,
    "attention_mask": attention_mask
})

# Print output shapes and a sample of the dense embedding
print('Output shapes:')
for i, out in enumerate(outputs):
    print(f'Output {i}:', out.shape)

print('Dense embedding (first 5 values):', outputs[0][0][:5]) 