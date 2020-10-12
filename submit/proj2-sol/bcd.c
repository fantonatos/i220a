

#include "bcd.h"

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Power
unsigned long long power(unsigned long long base, unsigned int exp) {
    unsigned long long i, result = 1;
    for (i = 0; i < exp; i++)
        result *= base;
    return result;
 }

// Reverse string
char *strrev(char *str)
{
      char *p1, *p2;

      if (! str || ! *str)
            return str;
      for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
      {
            *p1 ^= *p2;
            *p2 ^= *p1;
            *p1 ^= *p2;
      }
      return str;
}

/** Return BCD encoding of binary (which has normal binary representation).
 *
 *  Examples: binary_to_bcd(0xc) => 0x12;
 *            binary_to_bcd(0xff) => 0x255
 *
 *  If error is not NULL, sets *error to OVERFLOW_ERR if binary is too
 *  big for the Bcd type, otherwise *error is unchanged.
 */
Bcd
binary_to_bcd(Binary value, BcdError *error)
{
  //printf("Received Binary Number %llu\t", value);

  Binary tmp = value;
  unsigned long long digits = 0;
  do{
    digits++;
    tmp /= 10;
  }while (tmp);


  if (MAX_BCD_DIGITS < digits)
  {
    *error = OVERFLOW_ERR;
    return 0;
  }

  Bcd result = 0; int shift = 0;
  while (value > 0) {
      result |= (value % 10) << (shift++ << 2);
      value /= 10;
  }

  return result;
}

/** Return binary encoding of BCD value bcd.
 *
 *  Examples: bcd_to_binary(0x12) => 0xc;
 *            bcd_to_binary(0x255) => 0xff
 *
 *  If error is not NULL, sets *error to BAD_VALUE_ERR if bcd contains
 *  a bad BCD digit.
 *  Cannot overflow since Binary can represent larger values than Bcd
 */
Binary
bcd_to_binary(Bcd bcd, BcdError *error)
{
  // Calculate how many digits there are
  Bcd szTemp = bcd; Binary result = 0;
  unsigned long long digits = 1;
  while(szTemp >= 10)
  {
    szTemp /= 16;
    digits++;
  }

  // Cycle through all the nibbles individualy and translate them
  for (unsigned long long index = 0; index < digits; index++)
  {
    // Select the indexth nibble from the BCD.
    Binary nibble = bcd >> (BCD_BITS*index);
    nibble %= 16;

    // Error detection
    if (nibble >= 10) *error = BAD_VALUE_ERR;

    // Add this digit to the result
    result += nibble * power(10, index);
  }

  return *error == 0 ? result : 0; // i numeri sono finito, loro stanno in l'inferno
}

/** Return BCD encoding of decimal number corresponding to string s.
 *  Behavior undefined on overflow or if s contains a non-digit
 *  character.  Sets *p to point to first non-digit char in s.
 *  Rougly equivalent to strtol().
 *
 *  If error is not NULL, sets *error to OVERFLOW_ERR if binary is too
 *  big for the Bcd type, otherwise *error is unchanged.
 */
Bcd
str_to_bcd(const char *s, const char **p, BcdError *error)
{
  Binary val = strtoll(s, (char**)p, 10);
  return binary_to_bcd(val, error);
}

/** Convert bcd to a NUL-terminated string in buf[] without any
 *  non-significant leading zeros.  Never write more than bufSize
 *  characters into buf.  The return value is the number of characters
 *  written (excluding the NUL character used to terminate strings).
 *
 *  If error is not NULL, sets *error to BAD_VALUE_ERR if bcd contains
 *  a BCD digit which is greater than 9, OVERFLOW_ERR if bufSize bytes
 *  is less than BCD_BUF_SIZE, otherwise *error is unchanged.
 */
int
bcd_to_str(Bcd bcd, char buf[], size_t bufSize, BcdError *error)
{
  if (bufSize < BCD_BUF_SIZE) *error=OVERFLOW_ERR;

  sprintf(buf, "%llx", (unsigned long long)bcd);

  Bcd tmp = bcd;

  while (tmp)
  {
    if ((tmp % 16) > 9) *error = BAD_VALUE_ERR;
    tmp /= 16;
  }

  // Convert value to a string, one digit at at time.
  return *error == 0 ? strlen(buf) : 0;
}

/** Return the BCD representation of the sum of BCD int's x and y.
 *
 *  If error is not NULL, sets *error to to BAD_VALUE_ERR is x or y
 *  contains a BCD digit which is greater than 9, OVERFLOW_ERR on
 *  overflow, otherwise *error is unchanged.
 */
Bcd
bcd_add(Bcd x, Bcd y, BcdError *error)
{
  // Convert both x and y to decimal, return the sum
  Binary bin_x = bcd_to_binary(x, error);
  Binary bin_y = bcd_to_binary(y, error);

  return binary_to_bcd(bin_x + bin_y, error);
}

/** Return the BCD representation of the product of BCD int's x and y.
 *
 * If error is not NULL, sets *error to to BAD_VALUE_ERR is x or y
 * contains a BCD digit which is greater than 9, OVERFLOW_ERR on
 * overflow, otherwise *error is unchanged.
 */
Bcd
bcd_multiply(Bcd x, Bcd y, BcdError *error)
{
  // Convert both x and y to decimal, return the product
  Binary bin_x = bcd_to_binary(x, error);
  Binary bin_y = bcd_to_binary(y, error);

  return binary_to_bcd(bin_x * bin_y, error);
}