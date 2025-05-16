import os
import random
import math

WORDS_FILE = "words.csv"
OUTPUT_DIR = "text_data_mixed"
TOTAL_SIZE_GB = 20
WORDS_PER_LINE = 10
AVG_WORD_LENGTH = 6
TARGET_FILE_SIZE_BYTES = 1 * 1024**3  # 1GB
NUM_FILES = TOTAL_SIZE_GB

SHARED_PERCENTAGE = 0.6  # 60% del vocabulario será compartido


def load_words(path):
    with open(path, "r", encoding="utf-8") as f:
        return [line.strip() for line in f if line.strip()]


def split_words(words, shared_ratio, num_files):
    total_words = len(words)
    num_shared = int(total_words * shared_ratio)
    shared_words = random.sample(words, num_shared)

    remaining_words = list(set(words) - set(shared_words))
    chunk_size = math.ceil(len(remaining_words) / num_files)
    exclusive_chunks = [
        remaining_words[i : i + chunk_size]
        for i in range(0, len(remaining_words), chunk_size)
    ]

    return shared_words, exclusive_chunks


def generate_file(file_path, shared_words, exclusive_words, target_size_bytes):
    with open(file_path, "w", encoding="utf-8") as f:
        total_written = 0
        local_vocab = shared_words + exclusive_words
        while total_written < target_size_bytes:
            random.shuffle(local_vocab)
            for i in range(0, len(local_vocab), WORDS_PER_LINE):
                line = " ".join(local_vocab[i : i + WORDS_PER_LINE]) + "\n"
                f.write(line)
                total_written += len(line)
                if total_written >= target_size_bytes:
                    break


def main():
    words = load_words(WORDS_FILE)
    if len(words) < NUM_FILES:
        print("❌ Error: Hay menos palabras que archivos.")
        return

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    random.shuffle(words)

    shared_words, exclusive_chunks = split_words(words, SHARED_PERCENTAGE, NUM_FILES)

    print(f"Generando {NUM_FILES} archivos (~1GB cada uno)...")
    print(f" → Palabras compartidas entre archivos: {len(shared_words)}")

    for i in range(NUM_FILES):
        exclusive_words = exclusive_chunks[i] if i < len(exclusive_chunks) else []
        filename = os.path.join(OUTPUT_DIR, f"file_{i + 1}.txt")
        print(
            f" → {filename} con {len(exclusive_words)} exclusivas + {len(shared_words)} compartidas"
        )
        generate_file(filename, shared_words, exclusive_words, TARGET_FILE_SIZE_BYTES)

    print("✔ Archivos mixtos generados correctamente.")


if __name__ == "__main__":
    main()
