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

# Define batch sizes to test
batch_sizes = [4, 8, 32]

base_texts = [
    "BGE M3 is an embedding model supporting dense retrieval, lexical matching and multi-vector interaction.",
    "The quick brown fox jumps over the lazy dog.",
    "ONNX Runtime enables high-performance inference across platforms.",
    "Testing with a batch of sentences for embedding generation.",
    "Short.",
    "Another example sentence for embedding.",
    "Yet another test input for the batch.",
    "Final example for this batch test."
]

for batch_size in batch_sizes:
    print(f'\n===== Testing batch size: {batch_size} =====')
    # Repeat or slice base_texts to fill the batch
    texts = [base_texts[i % len(base_texts)] + f" #{i+1}" for i in range(batch_size)]

    # Run tokenizer on batch
    print('Running ONNX tokenizer on batch...')
    tokenizer_outputs = tokenizer_session.run(None, {"inputs": np.array(texts)})
    tokens, _, token_indices = tokenizer_outputs

    # Prepare model inputs for batch
    input_ids_batch = []
    attention_mask_batch = []
    for i in range(len(texts)):
        t_indices = token_indices[i]
        t_tokens = tokens[i]
        if np.isscalar(t_indices):
            t_indices = [t_indices]
        if np.isscalar(t_tokens):
            t_tokens = [t_tokens]
        token_pairs = list(zip(t_indices, t_tokens))
        token_pairs.sort()
        ordered_tokens = [pair[1] for pair in token_pairs]
        input_ids = np.array(ordered_tokens, dtype=np.int64)
        attention_mask = np.ones_like(input_ids, dtype=np.int64)
        input_ids_batch.append(input_ids)
        attention_mask_batch.append(attention_mask)

    # Pad sequences to the same length for batching
    max_len = max(len(ids) for ids in input_ids_batch)
    input_ids_batch = np.array([np.pad(ids, (0, max_len - len(ids)), constant_values=1) for ids in input_ids_batch], dtype=np.int64)
    attention_mask_batch = np.array([np.pad(mask, (0, max_len - len(mask)), constant_values=0) for mask in attention_mask_batch], dtype=np.int64)

    # Run embedding model on batch
    print('Running ONNX embedding model on batch...')
    outputs = model_session.run(None, {
        "input_ids": input_ids_batch,
        "attention_mask": attention_mask_batch
    })

    # Print output shapes and a sample of the dense embedding for each input
    print('Output shapes:')
    for i, out in enumerate(outputs):
        print(f'Output {i}:', out.shape)

    print('\nDense embedding (first 5 values for each input):')
    for idx, dense in enumerate(outputs[0]):
        print(f'Input {idx}:', dense[:5]) 