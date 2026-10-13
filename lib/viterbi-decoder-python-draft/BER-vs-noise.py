import viterbi_decoder
import input_sources

from matplotlib import pyplot as plt
import numpy as np

# Num intervals between 1% of noise in graph
RESOLUTION = 5
MAX_NOISE = 20
ITERATIONS = 300

def get_BER_at(noise_level: float = 0.00, iterations: int = ITERATIONS):

    sum_error = 0
    sum_bits = 0

    for i in range(iterations):
        message = input_sources.message(seed=-1, noise=noise_level)
        decoded_message, metric_sum = viterbi_decoder.decode(message.encoded_bits())
        
        error = 0
        for i in range(min(len(decoded_message), len(message._initial_message))):
            error += int(message._initial_message[i] != decoded_message[i])

        sum_error += error
        sum_bits += len(decoded_message)

    print(f"BER at {noise_level * 100}% noise: {sum_error / sum_bits}")

    return sum_error / sum_bits

def plot_BER(data):
    figure = plt.figure()
    ax = figure.add_subplot(1, 1, 1)
    plt.suptitle(rf'Bit Error Rate vs. Noise', fontsize=15)
    ax.set_title(rf'Rate-1/2, K=4, hard decision, {ITERATIONS} trials per point.', fontsize=12)

    ax.set_xlabel(r'Noise Level [%]', fontsize=14)
    ax.set_ylabel(r'Bit Error Rate [%]', fontsize=14)

    ax.plot(np.arange(0, MAX_NOISE * RESOLUTION + 1) / RESOLUTION, data, color='blue', lw=1.5, label='Viterbi Decoded')
    ax.fill_between(np.arange(0, MAX_NOISE * RESOLUTION + 1) / RESOLUTION, 0, data, color='blue', alpha=0.15)

    ax.plot(np.arange(0, MAX_NOISE * RESOLUTION + 1) / RESOLUTION, np.arange(0, MAX_NOISE * RESOLUTION + 1) / RESOLUTION, ls='--', alpha=0.5, lw=0.5, color="k", label="Uncoded")

    plt.legend()
    plt.tight_layout()
    plt.savefig('BER_vs_Noise.png', dpi=150, bbox_inches='tight')
    print("Saved BER_vs_Noise.png")

if __name__ == "__main__":
    ber = []
    for i in range(MAX_NOISE * RESOLUTION + 1):
        ber.append((get_BER_at(i / 100 / RESOLUTION)) * 100)
    plot_BER(ber)