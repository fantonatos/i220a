#include "hamming.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

/**
  All bitIndex'es are numbered starting at the LSB which is given index 1

  ** denotes exponentiation; note that 2**n == (1 << n)
*/

void print_word(HammingWord word)
{
  for(int index = 1; index <= sizeof(word)*8; index++)
  {
    printf("%u", get_bit(word, index));
  }
  printf("(%llu)\n", word);
}


/** Return bit at bitIndex from word. */
unsigned
get_bit(HammingWord word, int bitIndex)
{
  assert(bitIndex > 0);
  return (word >> bitIndex - 1) & 1ull;
}

/** Return word with bit at bitIndex in word set to bitValue. */
HammingWord
set_bit(HammingWord word, int bitIndex, unsigned bitValue)
{
  assert(bitIndex > 0);
  assert(bitValue == 0 || bitValue == 1);
  
  word ^= (-(HammingWord)bitValue ^ word) & (1ULL << bitIndex-1);
  return word;
}

/** Given a Hamming code with nParityBits, return 2**nParityBits - 1,
 *  i.e. the max # of bits in an encoded word (# data bits + # parity
 *  bits).
 */
unsigned
get_n_encoded_bits(unsigned nParityBits)
{
  return 1 << (nParityBits - 1);
}

/** Return non-zero if bitIndex indexes a bit which will be used for a
 *  Hamming parity bit; i.e. the bit representation of bitIndex
 *  contains only a single 1.
 * 
 *  If true, returns the parity bit's index within it's own byte. (1, 2, or 4).
 */
int
is_parity_position(int bitIndex)
{
  assert(bitIndex > 0);
  return (bitIndex & (bitIndex-1)) == 0;
}

/**
 * Returns non-zero if the specified bitIndex, 
 * should be computed for the specified parityIndex
 */
int
parity_includes_index(int parityIndex, int bitIndex)
{
  assert(is_parity_position(parityIndex));
  return parityIndex & bitIndex;
}

/** Return the parity over the data bits in word specified by the
 *  parity bit bitIndex.  The word contains a total of nBits bits.
 *  Equivalently, return parity over all data bits whose bit-index has
 *  a 1 in the same position as in bitIndex.
 */
int
compute_parity(HammingWord word, int bitIndex, unsigned nBits)
{
  assert(bitIndex > 0);
  assert(is_parity_position(bitIndex));
 
  /*
     (Step 1) Produce list of indicies which share a 1 in the binary place.
     (Step 2) Compute them all with XOR (^)

     If a bit's index is & with the index of a parity, the result is parity.
     Otherwise you will get 0.
     
     If 16 & 18 == 16, 18 will be used when computing 16.
  */
  int parityValue = 0;
  for (int dataBit = 1; dataBit <= nBits; dataBit++)
  {
    if (parity_includes_index(bitIndex, dataBit) > 0 && bitIndex != dataBit)
    {
      parityValue ^= get_bit(word, dataBit);
    }
  }

  return parityValue;  
}

/** Encode data using nParityBits Hamming code parity bits.
 *  Assumes data is within range of values which can be encoded using
 *  nParityBits.
 */
HammingWord
hamming_encode(HammingWord data, unsigned nParityBits)
{
  HammingWord word = 0ull;

/*  (Step 1) Restructures the bits to leave room for parities
    
    is_parity_position(wordIndex) && (wordIndex < (1 << nParityBits))
    true if index is parity within the user specified range.
*/

  for (int dataIndex = 1, wordIndex = 1; wordIndex <= sizeof(HammingWord)*8; is_parity_position(wordIndex) && (wordIndex < (1 << nParityBits))? (++wordIndex | dataIndex) : (++wordIndex | ++dataIndex))
    if (!(is_parity_position(wordIndex) && (wordIndex < (1 << nParityBits))))
      word = set_bit(word, wordIndex, get_bit(data, dataIndex));

  // (Step 2) Calculate parities and fill their positions with the proper values
  for (int parityIndex = 1, counter = 1; parityIndex < (1 << nParityBits); parityIndex = (1 << counter++))
    word = set_bit(word, parityIndex, compute_parity(word, parityIndex, sizeof(word) * 8));

  return word;
}

/** Decode encoded using nParityBits Hamming code parity bits.
 *  Set *hasError if an error was corrected.
 *  Assumes that data is within range of values which can be decoded
 *  using nParityBits.
 */
HammingWord
hamming_decode(HammingWord encoded, unsigned nParityBits, int *hasError)
{
  HammingWord tmp = encoded;
  int nBits = sizeof(HammingWord) * 8;

  // (Step 2) Compute each parity, and compare it to the parity we have.
  // Determine which parities are wrong.
  int wrongParities = 0;
  for (unsigned parityIndex = 1, counter = 1; parityIndex < (1 << nParityBits); parityIndex = (1 << counter++))
  {
    int ourResult = compute_parity(encoded, parityIndex, sizeof(encoded)*8);
    int encodedParity = get_bit(encoded, parityIndex);

    // If parities are not equal
    if (ourResult != encodedParity)
    {
      *hasError = 1;
      wrongParities += parityIndex;
    }
  }

  // The sum, "wrong parities" is the index of the bit that was corrupted, so we will flip that bit.
  if (wrongParities > 0) 
  {
    tmp ^= 1ul << wrongParities-1;
    encoded = tmp;
  }
  

  // (Step 1) Grab each non-parity bit, and paste it into decoded
  for (int dataIndex = 1, encodedIndex = 1; encodedIndex <= nBits; encodedIndex++)
  {
    if ((encodedIndex & (encodedIndex-1)) == 0 && encodedIndex < (1 << nParityBits)) continue;
    tmp = set_bit(tmp, dataIndex++, get_bit(encoded, encodedIndex));
  }


  

  return tmp;
}
