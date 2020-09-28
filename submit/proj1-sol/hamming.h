#ifndef HAMMING_H_
#define HAMMING_H_

/** A HammingWord contains the encoded data (the original data bits +
 *  the parity bits).
 */
typedef unsigned long long HammingWord;

unsigned get_bit(HammingWord word, int bitIndex);

void print_word(HammingWord word);

HammingWord set_bit(HammingWord word, int bitIndex, unsigned bitValue);

unsigned get_n_encoded_bits(unsigned nParityBits);

int is_parity_position(int bitIndex);

int parity_includes_index(int parityIndex, int bitIndex);

int compute_parity(HammingWord word, int bitIndex, unsigned nBits);

/** Encode data using nParityBits Hamming code parity bits.
 *  Assumes that data is within range of values which can be encoded
 *  using nParityBits.
 */
HammingWord hamming_encode(HammingWord data, unsigned nParityBits);

/** Decode encoded using nParityBits Hamming code parity bits.
 *  Set *hasError to non-zero if an error is detected.
 *  Assumes that data is within range of values which can be decoded
 *  using nParityBits.
 */
HammingWord hamming_decode(HammingWord encoded, unsigned nParityBits,
                           int *hasError);

#endif //ifndef HAMMING_H_