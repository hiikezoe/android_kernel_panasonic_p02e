/*
 * rijndael.c
 *
 */

#include "linux_precomp.h"

#include "ah.h"
#include "esp.h"
#include "des.h"
#include "rijndael.h"

#include "boxes-fst.dat"


int rijndaelKeySched(word8 k[MAXKC][4], word8 W[MAXROUNDS+1][4][4], int ROUNDS)
{
  /* Calculate the necessary round keys
   * The number of calculations depends on keyBits and blockBits
   */ 
  int j, r, t, rconpointer = 0;
  union {
    word8	x8[MAXKC][4];
    word32	x32[MAXKC];
  } xtk;
#define	tk	xtk.x8
  int KC = ROUNDS - 6;

  for (j = KC-1; j >= 0; j--) {
    *((word32*)tk[j]) = *((word32*)k[j]);
  }
  r = 0;
  t = 0;
  /* copy values into round key array */
  for (j = 0; (j < KC) && (r < ROUNDS + 1); ) {
    for (; (j < KC) && (t < 4); j++, t++) {
      *((word32*)W[r][t]) = *((word32*)tk[j]); /* pgr0000 */
    }
    if (t == 4) {
      r++;
      t = 0;
    }
  }
		
  while (r < ROUNDS + 1) { /* while not enough round key material calculated */
    /* calculate new values */
    tk[0][0] ^= S[tk[KC-1][1]]; /* pgr0000 */
    tk[0][1] ^= S[tk[KC-1][2]];
    tk[0][2] ^= S[tk[KC-1][3]];
    tk[0][3] ^= S[tk[KC-1][0]];
    tk[0][0] ^= rcon[rconpointer++];

    if (KC != 8) {
      for (j = 1; j < KC; j++) {
	*((word32*)tk[j]) ^= *((word32*)tk[j-1]);
      }
    } else {
      for (j = 1; j < KC/2; j++) {
	*((word32*)tk[j]) ^= *((word32*)tk[j-1]);
      }
      tk[KC/2][0] ^= S[tk[KC/2 - 1][0]];
      tk[KC/2][1] ^= S[tk[KC/2 - 1][1]];
      tk[KC/2][2] ^= S[tk[KC/2 - 1][2]];
      tk[KC/2][3] ^= S[tk[KC/2 - 1][3]];
      for (j = KC/2 + 1; j < KC; j++) {
	*((word32*)tk[j]) ^= *((word32*)tk[j-1]);
      }
    }
    /* copy values into round key array */
    for (j = 0; (j < KC) && (r < ROUNDS + 1); ) {
      for (; (j < KC) && (t < 4); j++, t++) {
	*((word32*)W[r][t]) = *((word32*)tk[j]);
      }
      if (t == 4) {
	r++;
	t = 0;
      }
    }
  }		
  return 0;
#undef tk
}


int rijndaelKeyEncToDec(word8 W[MAXROUNDS+1][4][4], int ROUNDS)
{
  int r;
  word8 *w;
  
  for (r = 1; r < ROUNDS; r++) {
    w = W[r][0];
    *((word32*)w) =
      *((const word32*)U1[w[0]])
      ^ *((const word32*)U2[w[1]])
      ^ *((const word32*)U3[w[2]])
      ^ *((const word32*)U4[w[3]]);
    
    w = W[r][1];
    *((word32*)w) =
      *((const word32*)U1[w[0]])
      ^ *((const word32*)U2[w[1]])
      ^ *((const word32*)U3[w[2]])
      ^ *((const word32*)U4[w[3]]);

    w = W[r][2];
    *((word32*)w) =
      *((const word32*)U1[w[0]])
      ^ *((const word32*)U2[w[1]])
      ^ *((const word32*)U3[w[2]])
      ^ *((const word32*)U4[w[3]]);

    w = W[r][3];
    *((word32*)w) =
      *((const word32*)U1[w[0]])
      ^ *((const word32*)U2[w[1]])
      ^ *((const word32*)U3[w[2]])
      ^ *((const word32*)U4[w[3]]);
  }
  return 0;
}	


int rijndael_makeKey(keyInstance *key, BYTE direction, int keyLen, char *keyMaterial) 
{
  word8 k[MAXKC][4];
  int i;
  char *keyMat;
	
  if (key == NULL) {
    return BAD_KEY_INSTANCE;
  }

  if ((direction == DIR_ENCRYPT) || (direction == DIR_DECRYPT)) {
    key->direction = direction;
  } else {
    return BAD_KEY_DIR;
  }

  if ((keyLen == 128) || (keyLen == 192) || (keyLen == 256)) { 
    key->keyLen = keyLen;
  } else {
    return BAD_KEY_MAT;
  }

  if (keyMaterial != NULL) {
    bcopy(keyMaterial, key->keyMaterial, keyLen/8);
  }

  key->ROUNDS = keyLen/32 + 6;

  /* initialize key schedule: */
  keyMat = key->keyMaterial;
  for (i = 0; i < key->keyLen/8; i++) {
    k[i >> 2][i & 3] = (word8)keyMat[i]; 
  }
  rijndaelKeySched(k, key->keySched, key->ROUNDS);
  if (direction == DIR_DECRYPT) {
    rijndaelKeyEncToDec(key->keySched, key->ROUNDS);
  }

  return TRUE;
}

int rijndael_cipherInit(cipherInstance *cipher, BYTE mode, char *IV)
{
  if ((mode == MODE_ECB) || (mode == MODE_CBC) || (mode == MODE_CFB1)) {
    cipher->mode = mode;
  } else {
    return BAD_CIPHER_MODE;
  }
  if (IV != NULL) {
    bcopy(IV, cipher->IV, MAX_IV_SIZE);
  } else {
    bzero(cipher->IV, MAX_IV_SIZE);
  }
  return TRUE;
}

int rijndael_blockDecrypt(cipherInstance *cipher, keyInstance *key,
			  BYTE *input, int inputLen, BYTE *outBuffer) {
  int i, k, numBlocks;
  word8 block[16], iv[4][4];
  
  if (cipher == NULL ||
      key == NULL ||
      (cipher->mode != MODE_CFB1 && key->direction == DIR_ENCRYPT)) {
    return BAD_CIPHER_STATE;
  }
  if (input == NULL || inputLen <= 0) {
    return 0; /* nothing to do */
  }
  
  numBlocks = inputLen/128;
  
  switch (cipher->mode) {
  case MODE_ECB: 
    for (i = numBlocks; i > 0; i--) { 
      rijndael_Decrypt(input, outBuffer, key->keySched, key->ROUNDS);
      input += 16;
      outBuffer += 16;
    }
    break;
		
  case MODE_CBC:
#if 1 /*STRICT_ALIGN */
    bcopy(cipher->IV, iv, 16); 
#else
    *((word32*)iv[0]) = *((word32*)(cipher->IV   ));
    *((word32*)iv[1]) = *((word32*)(cipher->IV+ 4));
    *((word32*)iv[2]) = *((word32*)(cipher->IV+ 8));
    *((word32*)iv[3]) = *((word32*)(cipher->IV+12));
#endif
    for (i = numBlocks; i > 0; i--) {
      rijndael_Decrypt(input, block, key->keySched, key->ROUNDS);
      ((word32*)block)[0] ^= *((word32*)iv[0]);
      ((word32*)block)[1] ^= *((word32*)iv[1]);
      ((word32*)block)[2] ^= *((word32*)iv[2]);
      ((word32*)block)[3] ^= *((word32*)iv[3]);
#if 1 /*STRICT_ALIGN*/
      bcopy(input, iv, 16);
      bcopy(block, outBuffer, 16);
#else
      *((word32*)iv[0]) = ((word32*)input)[0]; ((word32*)outBuffer)[0] = ((word32*)block)[0];
      *((word32*)iv[1]) = ((word32*)input)[1]; ((word32*)outBuffer)[1] = ((word32*)block)[1];
      *((word32*)iv[2]) = ((word32*)input)[2]; ((word32*)outBuffer)[2] = ((word32*)block)[2];
      *((word32*)iv[3]) = ((word32*)input)[3]; ((word32*)outBuffer)[3] = ((word32*)block)[3];
#endif
      input += 16;
      outBuffer += 16;
    }
    break;
    
  case MODE_CFB1:
#if 1 /*STRICT_ALIGN */
    bcopy(cipher->IV, iv, 16); 
#else
    *((word32*)iv[0]) = *((word32*)(cipher->IV));
    *((word32*)iv[1]) = *((word32*)(cipher->IV+ 4));
    *((word32*)iv[2]) = *((word32*)(cipher->IV+ 8));
    *((word32*)iv[3]) = *((word32*)(cipher->IV+12));
#endif
    for (i = numBlocks; i > 0; i--) {
      for (k = 0; k < 128; k++) {
	*((word32*) block    ) = *((word32*)iv[0]);
	*((word32*)(block+ 4)) = *((word32*)iv[1]);
	*((word32*)(block+ 8)) = *((word32*)iv[2]);
	*((word32*)(block+12)) = *((word32*)iv[3]);
	rijndael_Encrypt(block, block, key->keySched, key->ROUNDS);
	iv[0][0] = (iv[0][0] << 1) | (iv[0][1] >> 7);
	iv[0][1] = (iv[0][1] << 1) | (iv[0][2] >> 7);
	iv[0][2] = (iv[0][2] << 1) | (iv[0][3] >> 7);
	iv[0][3] = (iv[0][3] << 1) | (iv[1][0] >> 7);
	iv[1][0] = (iv[1][0] << 1) | (iv[1][1] >> 7);
	iv[1][1] = (iv[1][1] << 1) | (iv[1][2] >> 7);
	iv[1][2] = (iv[1][2] << 1) | (iv[1][3] >> 7);
	iv[1][3] = (iv[1][3] << 1) | (iv[2][0] >> 7);
	iv[2][0] = (iv[2][0] << 1) | (iv[2][1] >> 7);
	iv[2][1] = (iv[2][1] << 1) | (iv[2][2] >> 7);
	iv[2][2] = (iv[2][2] << 1) | (iv[2][3] >> 7);
	iv[2][3] = (iv[2][3] << 1) | (iv[3][0] >> 7);
	iv[3][0] = (iv[3][0] << 1) | (iv[3][1] >> 7);
	iv[3][1] = (iv[3][1] << 1) | (iv[3][2] >> 7);
	iv[3][2] = (iv[3][2] << 1) | (iv[3][3] >> 7);
	iv[3][3] = (iv[3][3] << 1) | ((input[k/8] >> (7-(k&7))) & 1);
	outBuffer[k/8] ^= (block[0] & 0x80) >> (k & 7);
      }
    }
    break;
    
  default:
    return BAD_CIPHER_STATE;
  }
  
  return 128*numBlocks;
}


int rijndael_blockEncrypt(cipherInstance *cipher, keyInstance *key,
			  BYTE *input, int inputLen, BYTE *outBuffer) 
{
  int i, k, numBlocks;
  word8 block[16], iv[4][4];

  if (cipher == NULL ||
      key == NULL ||
      key->direction == DIR_DECRYPT) {
    return BAD_CIPHER_STATE;
  }
  if (input == NULL || inputLen <= 0) {
    return 0; /* nothing to do */
  }

  numBlocks = inputLen/128;
  
  switch (cipher->mode) {
  case MODE_ECB: 
    for (i = numBlocks; i > 0; i--) {
      rijndael_Encrypt(input, outBuffer, key->keySched, key->ROUNDS);
      input += 16;
      outBuffer += 16;
    }
    break;
		
  case MODE_CBC:
#if 1 /*STRICT_ALIGN*/
    bcopy(cipher->IV, block, 16);
    bcopy(input, iv, 16);
    ((word32*)block)[0] ^= ((word32*)iv)[0];
    ((word32*)block)[1] ^= ((word32*)iv)[1];
    ((word32*)block)[2] ^= ((word32*)iv)[2];
    ((word32*)block)[3] ^= ((word32*)iv)[3];
#else
    ((word32*)block)[0] = ((word32*)cipher->IV)[0] ^ ((word32*)input)[0];
    ((word32*)block)[1] = ((word32*)cipher->IV)[1] ^ ((word32*)input)[1];
    ((word32*)block)[2] = ((word32*)cipher->IV)[2] ^ ((word32*)input)[2];
    ((word32*)block)[3] = ((word32*)cipher->IV)[3] ^ ((word32*)input)[3];
#endif
    rijndael_Encrypt(block, outBuffer, key->keySched, key->ROUNDS);
    input += 16;
    for (i = numBlocks - 1; i > 0; i--) {
#if 1 /*STRICT_ALIGN*/
      bcopy(outBuffer, block, 16);
      ((word32*)block)[0] ^= ((word32*)iv)[0];
      ((word32*)block)[1] ^= ((word32*)iv)[1];
      ((word32*)block)[2] ^= ((word32*)iv)[2];
      ((word32*)block)[3] ^= ((word32*)iv)[3];
#else
      ((word32*)block)[0] = ((word32*)outBuffer)[0] ^ ((word32*)input)[0];
      ((word32*)block)[1] = ((word32*)outBuffer)[1] ^ ((word32*)input)[1];
      ((word32*)block)[2] = ((word32*)outBuffer)[2] ^ ((word32*)input)[2];
      ((word32*)block)[3] = ((word32*)outBuffer)[3] ^ ((word32*)input)[3];
#endif
      outBuffer += 16;
      rijndael_Encrypt(block, outBuffer, key->keySched, key->ROUNDS);
      input += 16;
    }
    break;
	
  case MODE_CFB1:
#if 1 /*STRICT_ALIGN*/
    bcopy(cipher->IV, iv, 16); 
#else  /* !STRICT_ALIGN */
    *((word32*)iv[0]) = *((word32*)(cipher->IV   ));
    *((word32*)iv[1]) = *((word32*)(cipher->IV+ 4));
    *((word32*)iv[2]) = *((word32*)(cipher->IV+ 8));
    *((word32*)iv[3]) = *((word32*)(cipher->IV+12));
#endif /* ?STRICT_ALIGN */
    for (i = numBlocks; i > 0; i--) {
      for (k = 0; k < 128; k++) {
	*((word32*) block    ) = *((word32*)iv[0]);
	*((word32*)(block+ 4)) = *((word32*)iv[1]);
	*((word32*)(block+ 8)) = *((word32*)iv[2]);
	*((word32*)(block+12)) = *((word32*)iv[3]);
	rijndael_Encrypt(block, block, key->keySched, key->ROUNDS);
	outBuffer[k/8] ^= (block[0] & 0x80) >> (k & 7);
	iv[0][0] = (iv[0][0] << 1) | (iv[0][1] >> 7);
	iv[0][1] = (iv[0][1] << 1) | (iv[0][2] >> 7);
	iv[0][2] = (iv[0][2] << 1) | (iv[0][3] >> 7);
	iv[0][3] = (iv[0][3] << 1) | (iv[1][0] >> 7);
	iv[1][0] = (iv[1][0] << 1) | (iv[1][1] >> 7);
	iv[1][1] = (iv[1][1] << 1) | (iv[1][2] >> 7);
	iv[1][2] = (iv[1][2] << 1) | (iv[1][3] >> 7);
	iv[1][3] = (iv[1][3] << 1) | (iv[2][0] >> 7);
	iv[2][0] = (iv[2][0] << 1) | (iv[2][1] >> 7);
	iv[2][1] = (iv[2][1] << 1) | (iv[2][2] >> 7);
	iv[2][2] = (iv[2][2] << 1) | (iv[2][3] >> 7);
	iv[2][3] = (iv[2][3] << 1) | (iv[3][0] >> 7);
	iv[3][0] = (iv[3][0] << 1) | (iv[3][1] >> 7);
	iv[3][1] = (iv[3][1] << 1) | (iv[3][2] >> 7);
	iv[3][2] = (iv[3][2] << 1) | (iv[3][3] >> 7);
	iv[3][3] = (iv[3][3] << 1) | ((outBuffer[k/8] >> (7-(k&7))) & 1);
      }
    }
    break;
    
  default:
    return BAD_CIPHER_STATE;
  }
  
  return 128*numBlocks;
}

/**
 * Decrypt a single block.
 */
int rijndael_Decrypt(word8 in[16], word8 out[16], word8 rk[MAXROUNDS+1][4][4], int ROUNDS) {
	int r;
	union {
		word8	x8[16];
		word32	x32[4];
	} xa, xb;
#define	a	xa.x8
#define	b	xb.x8
	union {
		word8	x8[4][4];
		word32	x32[4];
	} xtemp;
#define	temp	xtemp.x8
	
    memcpy(a, in, sizeof a);

    *((word32*)temp[0]) = *((word32*)(a   )) ^ *((word32*)rk[ROUNDS][0]);
    *((word32*)temp[1]) = *((word32*)(a+ 4)) ^ *((word32*)rk[ROUNDS][1]);
    *((word32*)temp[2]) = *((word32*)(a+ 8)) ^ *((word32*)rk[ROUNDS][2]);
    *((word32*)temp[3]) = *((word32*)(a+12)) ^ *((word32*)rk[ROUNDS][3]);

    *((word32*)(b   )) = *((const word32*)T5[temp[0][0]])
           ^ *((const word32*)T6[temp[3][1]])
           ^ *((const word32*)T7[temp[2][2]]) 
           ^ *((const word32*)T8[temp[1][3]]);
	*((word32*)(b+ 4)) = *((const word32*)T5[temp[1][0]])
           ^ *((const word32*)T6[temp[0][1]])
           ^ *((const word32*)T7[temp[3][2]]) 
           ^ *((const word32*)T8[temp[2][3]]);
	*((word32*)(b+ 8)) = *((const word32*)T5[temp[2][0]])
           ^ *((const word32*)T6[temp[1][1]])
           ^ *((const word32*)T7[temp[0][2]]) 
           ^ *((const word32*)T8[temp[3][3]]);
	*((word32*)(b+12)) = *((const word32*)T5[temp[3][0]])
           ^ *((const word32*)T6[temp[2][1]])
           ^ *((const word32*)T7[temp[1][2]]) 
           ^ *((const word32*)T8[temp[0][3]]);
	for (r = ROUNDS-1; r > 1; r--) {
		*((word32*)temp[0]) = *((word32*)(b   )) ^ *((word32*)rk[r][0]);
		*((word32*)temp[1]) = *((word32*)(b+ 4)) ^ *((word32*)rk[r][1]);
		*((word32*)temp[2]) = *((word32*)(b+ 8)) ^ *((word32*)rk[r][2]);
		*((word32*)temp[3]) = *((word32*)(b+12)) ^ *((word32*)rk[r][3]);
		*((word32*)(b   )) = *((const word32*)T5[temp[0][0]])
           ^ *((const word32*)T6[temp[3][1]])
           ^ *((const word32*)T7[temp[2][2]]) 
           ^ *((const word32*)T8[temp[1][3]]);
		*((word32*)(b+ 4)) = *((const word32*)T5[temp[1][0]])
           ^ *((const word32*)T6[temp[0][1]])
           ^ *((const word32*)T7[temp[3][2]]) 
           ^ *((const word32*)T8[temp[2][3]]);
		*((word32*)(b+ 8)) = *((const word32*)T5[temp[2][0]])
           ^ *((const word32*)T6[temp[1][1]])
           ^ *((const word32*)T7[temp[0][2]]) 
           ^ *((const word32*)T8[temp[3][3]]);
		*((word32*)(b+12)) = *((const word32*)T5[temp[3][0]])
           ^ *((const word32*)T6[temp[2][1]])
           ^ *((const word32*)T7[temp[1][2]]) 
           ^ *((const word32*)T8[temp[0][3]]);
	}
	/* last round is special */   
	*((word32*)temp[0]) = *((word32*)(b   )) ^ *((word32*)rk[1][0]);
	*((word32*)temp[1]) = *((word32*)(b+ 4)) ^ *((word32*)rk[1][1]);
	*((word32*)temp[2]) = *((word32*)(b+ 8)) ^ *((word32*)rk[1][2]);
	*((word32*)temp[3]) = *((word32*)(b+12)) ^ *((word32*)rk[1][3]);
	b[ 0] = S5[temp[0][0]];
	b[ 1] = S5[temp[3][1]];
	b[ 2] = S5[temp[2][2]];
	b[ 3] = S5[temp[1][3]];
	b[ 4] = S5[temp[1][0]];
	b[ 5] = S5[temp[0][1]];
	b[ 6] = S5[temp[3][2]];
	b[ 7] = S5[temp[2][3]];
	b[ 8] = S5[temp[2][0]];
	b[ 9] = S5[temp[1][1]];
	b[10] = S5[temp[0][2]];
	b[11] = S5[temp[3][3]];
	b[12] = S5[temp[3][0]];
	b[13] = S5[temp[2][1]];
	b[14] = S5[temp[1][2]];
	b[15] = S5[temp[0][3]];
	*((word32*)(b   )) ^= *((word32*)rk[0][0]);
	*((word32*)(b+ 4)) ^= *((word32*)rk[0][1]);
	*((word32*)(b+ 8)) ^= *((word32*)rk[0][2]);
	*((word32*)(b+12)) ^= *((word32*)rk[0][3]);

	memcpy(out, b, sizeof b /* XXX out */);

	return 0;
#undef a
#undef b
#undef temp
}


/**
 * Encrypt a single block. 
 */
int rijndael_Encrypt(word8 in[16], word8 out[16], word8 rk[MAXROUNDS+1][4][4], int ROUNDS) {
	int r;
	union {
		word8	x8[16];
		word32	x32[4];
	} xa, xb;
#define	a	xa.x8
#define	b	xb.x8
	union {
		word8	x8[4][4];
		word32	x32[4];
	} xtemp;
#define	temp	xtemp.x8

    memcpy(a, in, sizeof a);

    *((word32*)temp[0]) = *((word32*)(a   )) ^ *((word32*)rk[0][0]);
    *((word32*)temp[1]) = *((word32*)(a+ 4)) ^ *((word32*)rk[0][1]);
    *((word32*)temp[2]) = *((word32*)(a+ 8)) ^ *((word32*)rk[0][2]);
    *((word32*)temp[3]) = *((word32*)(a+12)) ^ *((word32*)rk[0][3]);
    *((word32*)(b    )) = *((const word32*)T1[temp[0][0]])
					^ *((const word32*)T2[temp[1][1]])
					^ *((const word32*)T3[temp[2][2]]) 
					^ *((const word32*)T4[temp[3][3]]);
    *((word32*)(b + 4)) = *((const word32*)T1[temp[1][0]])
					^ *((const word32*)T2[temp[2][1]])
					^ *((const word32*)T3[temp[3][2]]) 
					^ *((const word32*)T4[temp[0][3]]);
    *((word32*)(b + 8)) = *((const word32*)T1[temp[2][0]])
					^ *((const word32*)T2[temp[3][1]])
					^ *((const word32*)T3[temp[0][2]]) 
					^ *((const word32*)T4[temp[1][3]]);
    *((word32*)(b +12)) = *((const word32*)T1[temp[3][0]])
					^ *((const word32*)T2[temp[0][1]])
					^ *((const word32*)T3[temp[1][2]]) 
					^ *((const word32*)T4[temp[2][3]]);
	for (r = 1; r < ROUNDS-1; r++) {
		*((word32*)temp[0]) = *((word32*)(b   )) ^ *((word32*)rk[r][0]);
		*((word32*)temp[1]) = *((word32*)(b+ 4)) ^ *((word32*)rk[r][1]);
		*((word32*)temp[2]) = *((word32*)(b+ 8)) ^ *((word32*)rk[r][2]);
		*((word32*)temp[3]) = *((word32*)(b+12)) ^ *((word32*)rk[r][3]);

		*((word32*)(b    )) = *((const word32*)T1[temp[0][0]])
					^ *((const word32*)T2[temp[1][1]])
					^ *((const word32*)T3[temp[2][2]]) 
					^ *((const word32*)T4[temp[3][3]]);
		*((word32*)(b + 4)) = *((const word32*)T1[temp[1][0]])
					^ *((const word32*)T2[temp[2][1]])
					^ *((const word32*)T3[temp[3][2]]) 
					^ *((const word32*)T4[temp[0][3]]);
		*((word32*)(b + 8)) = *((const word32*)T1[temp[2][0]])
					^ *((const word32*)T2[temp[3][1]])
					^ *((const word32*)T3[temp[0][2]]) 
					^ *((const word32*)T4[temp[1][3]]);
		*((word32*)(b +12)) = *((const word32*)T1[temp[3][0]])
					^ *((const word32*)T2[temp[0][1]])
					^ *((const word32*)T3[temp[1][2]]) 
					^ *((const word32*)T4[temp[2][3]]);
	}
	/* last round is special */   
	*((word32*)temp[0]) = *((word32*)(b   )) ^ *((word32*)rk[ROUNDS-1][0]);
	*((word32*)temp[1]) = *((word32*)(b+ 4)) ^ *((word32*)rk[ROUNDS-1][1]);
	*((word32*)temp[2]) = *((word32*)(b+ 8)) ^ *((word32*)rk[ROUNDS-1][2]);
	*((word32*)temp[3]) = *((word32*)(b+12)) ^ *((word32*)rk[ROUNDS-1][3]);
	b[ 0] = T1[temp[0][0]][1];
	b[ 1] = T1[temp[1][1]][1];
	b[ 2] = T1[temp[2][2]][1];
	b[ 3] = T1[temp[3][3]][1];
	b[ 4] = T1[temp[1][0]][1];
	b[ 5] = T1[temp[2][1]][1];
	b[ 6] = T1[temp[3][2]][1];
	b[ 7] = T1[temp[0][3]][1];
	b[ 8] = T1[temp[2][0]][1];
	b[ 9] = T1[temp[3][1]][1];
	b[10] = T1[temp[0][2]][1];
	b[11] = T1[temp[1][3]][1];
	b[12] = T1[temp[3][0]][1];
	b[13] = T1[temp[0][1]][1];
	b[14] = T1[temp[1][2]][1];
	b[15] = T1[temp[2][3]][1];
	*((word32*)(b   )) ^= *((word32*)rk[ROUNDS][0]);
	*((word32*)(b+ 4)) ^= *((word32*)rk[ROUNDS][1]);
	*((word32*)(b+ 8)) ^= *((word32*)rk[ROUNDS][2]);
	*((word32*)(b+12)) ^= *((word32*)rk[ROUNDS][3]);

	memcpy(out, b, sizeof b /* XXX out */);

	return 0;
#undef a
#undef b
#undef temp
}
