#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef unsigned __int64 bit64;

bit64 state[5] = {0}, t[5] = {0};
bit64 constants[16] = {
    0xf0,
    0xe1,
    0xd2,
    0xc3,
    0xb4,
    0xa5,
    0x96,
    0x87,
    0x78,
    0x69,
    0x5a,
    0x4b,
    0x3c,
    0x2d,
    0x1e,
    0x0f,
};

bit64 print_state(bit64 state[5])
{
  for (int i = 0; i < 5; i++)
  {
    printf("%016I64x\n", state[i]);
  }
}

bit64 rotate(bit64 x, int l)
{
  bit64 temp;
  temp = (x >> l) ^ (x << (64 - l));
  return temp;
}

void add_constant(bit64 state[5], int i, int a)
{
  state[2] = state[2] ^ constants[12 - a + i];
}
void sbox(bit64 x[5])
{

  x[0] ^= x[4];
  x[4] ^= x[3];
  x[2] ^= x[1];
  t[0] = x[0];
  t[1] = x[1];
  t[2] = x[2];
  t[3] = x[3];
  t[4] = x[4];
  t[0] = ~t[0];
  t[1] = ~t[1];
  t[2] = ~t[2];
  t[3] = ~t[3];
  t[4] = ~t[4];
  t[0] &= x[1];
  t[1] &= x[2];
  t[2] &= x[3];
  t[3] &= x[4];
  t[4] &= x[0];
  x[0] ^= t[1];
  x[1] ^= t[2];
  x[2] ^= t[3];
  x[3] ^= t[4];
  x[4] ^= t[0];
  x[1] ^= x[0];
  x[0] ^= x[4];
  x[3] ^= x[2];
  x[2] = ~x[2];
}
void linear(bit64 state[5])
{
  // Kita akan melakukan operasi rotasi terhadap state dengan tiap
  // 64 bit memiliki rotasi yang berbeda.
  // besar bit rotasi ditentukan pada spec ascon paper.

  bit64 temp0, temp1;
  temp0 = rotate(state[0], 19);
  temp1 = rotate(state[0], 28);
  state[0] ^= temp0 ^ temp1;
  temp0 = rotate(state[1], 61);
  temp1 = rotate(state[1], 39);
  state[1] ^= temp0 ^ temp1;
  temp0 = rotate(state[2], 1);
  temp1 = rotate(state[2], 6);
  state[2] ^= temp0 ^ temp1;
  temp0 = rotate(state[3], 10);
  temp1 = rotate(state[3], 17);
  state[3] ^= temp0 ^ temp1;
  temp0 = rotate(state[4], 7);
  temp1 = rotate(state[4], 41);
  state[4] ^= temp0 ^ temp1;
}

void p(bit64 state[5], int a)
{
  for (int i = 0; i < a; i++)
  {
    add_constant(state, i, a);
    sbox(state);
    linear(state);
  }
}

void initialization(bit64 state[5], bit64 key[2])
{
  p(state, 12);
  state[3] ^= key[0];
  state[4] ^= key[1];
}

void associated_data(bit64 state[5], int length, bit64 associated_data_text[])
{
  for (int i = 0; i < length; i++)
  {
    state[0] = associated_data_text[i] ^ state[0];
    p(state, 6);
  }
  state[5] = state[5] ^ 0x0000000000000001;
}

void finalization(bit64 state[5], bit64 key[2])
{
  state[1] ^= key[0];
  state[2] ^= key[1];
  p(state, 12);
  state[3] ^= key[0];
  state[4] ^= key[1];
}

void encrypt(bit64 state[5], int length, bit64 plaintext[], bit64 ciphertext[])
{
  ciphertext[0] = plaintext[0] ^ state[0];
  for (int i = 1; i < length; i++)
  {
    p(state, 6);
    ciphertext[i] = plaintext[i] ^ state[0];
    state[0] = ciphertext[i];
  }
}

void decrypt(bit64 state[5], int length, bit64 plaintext[], bit64 ciphertext[])
{
  plaintext[0] = ciphertext[0] ^ state[0];
  for (int i = 1; i < length; i++)
  {
    p(state, 6);
    plaintext[i] = ciphertext[i] ^ state[0];
    state[0] = ciphertext[i];
  }
}

bit64 *text_to_hex(char text[], int *hex_text_length)
{
  int text_length = strlen(text);
  // Her 8 byte bir blok olacak şekilde hesaplanır
  // (Every 8 bytes are calculated as a block)
  int blocks = (text_length + 7) / 8;
  *hex_text_length = blocks;

  bit64 *hex_text = (bit64 *)malloc(blocks * sizeof(bit64));

  for (int i = 0; i < blocks; i++)
  {
    bit64 block = 0;
    for (int j = 0; j < 8; j++)
    {
      int index = i * 8 + j;
      if (index < text_length)
      {
        block = (block << 8) | (unsigned char)text[index];
      }
      else
      {
        block = (block << 8); // add 0 for padding
      }
    }
    hex_text[i] = block;
  }

  return hex_text;
}

void print_given_list(bit64 *list, int length)
{
  for (int i = 0; i < length; i++)
  {
    printf("%d) %016I64x\n", i + 1, list[i]);
  }
}

int main()
{
  // initialize nonce, key and IV
  bit64 nonce[2] = {
      0x0000000000000001,
      0x0000000000000002,
  };
  bit64 key[2] = {0};
  bit64 IV = 0x80400c0600000000;
  // Örnek bir metin
  // (You can encrypt the text you want...)
  char text[] = "SAMSUN";
  // hex_text_length, Verilen text'in kaç adet 8 byte'lık bloktan oluştuğunu tutar
  // (hex_text_length, It holds how many 8 byte blocks the given text consists of)
  int hex_text_length = 0;
  // textToHex fonksiyonu text'in ascii tablosundaki ondalık sayıların hex'e karşlığını döner.
  // (The textToHex function returns the hex equivalent of the decimal numbers in the text in ascii table.)
  bit64 *hex_text = text_to_hex(text, &hex_text_length);
  bit64 *ciphertext = (bit64 *)malloc(hex_text_length * sizeof(bit64));
  bit64 associated_data_text[] = {0x787878, 0x878787, 0x09090};

  // encryption
  // initialize state
  state[0] = IV;
  state[1] = key[0];
  state[2] = key[1];
  state[3] = nonce[0];
  state[4] = nonce[1];

  initialization(state, key);
  associated_data(state, 3, associated_data_text);
  printf("State after initialization:\n");
  print_state(state);

  encrypt(state, hex_text_length, hex_text, ciphertext);
  printf("\nCiphertext:\n");
  print_given_list(ciphertext, hex_text_length);

  finalization(state, key);
  printf("\nTag: %016I64x %016I64x\n", state[3], state[4]);

  // Şifre çözme
  bit64 *ciphertext_decrypt = (bit64 *)malloc(hex_text_length * sizeof(bit64));
  for (int i = 0; i < hex_text_length; i++)
  {
    ciphertext_decrypt[i] = ciphertext[i];
  }
  bit64 *plaintext_decrypt = (bit64 *)malloc(hex_text_length * sizeof(bit64));

  // State'i tekrar başlat
  state[0] = IV;
  state[1] = key[0];
  state[2] = key[1];
  state[3] = nonce[0];
  state[4] = nonce[1];

  initialization(state, key);
  associated_data(state, 3, associated_data_text);
  decrypt(state, hex_text_length, plaintext_decrypt, ciphertext_decrypt);
  printf("\nDecrypted plaintext:\n");
  print_given_list(plaintext_decrypt, hex_text_length);

  finalization(state, key);
  printf("\nTag: %016I64x %016I64x\n", state[3], state[4]);

  return 0;
}
