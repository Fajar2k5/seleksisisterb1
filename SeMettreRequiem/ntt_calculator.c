#include <stdio.h>

typedef unsigned long long u64;
typedef __uint128_t u128;

u64 bitwise_add(u64 a, u64 b);
u64 bitwise_subtract(u64 a, u64 b);
u64 bitwise_multiply(u64 a, u64 b);
u64 bitwise_divide_and_mod_u64(u64 *q_ptr, u64 dividend, u64 divisor);
u128 bitwise_divide_and_mod_u128(u128 *q_ptr, u128 dividend, u128 divisor);

#define P1 998244353
#define G1 3
#define P2 469762049
#define G2 3
#define P3 167772161
#define G3 3

u64 bitwise_add(u64 a, u64 b) { u64 c; add_loop: if(b==0)goto add_loop_end; c=(a&b)<<1; a^=b; b=c; goto add_loop; add_loop_end: return a; }
u64 bitwise_subtract(u64 a, u64 b) { return bitwise_add(a, bitwise_add(~b, 1)); }

u64 bitwise_multiply(u64 a, u64 b) { u64 r=0; multiply_loop: if(b==0)goto multiply_loop_end; if(b&1)r=bitwise_add(r,a); a<<=1; b>>=1; goto multiply_loop; multiply_loop_end: return r; }

u64 bitwise_divide_and_mod_u64(u64 *q_ptr, u64 dividend, u64 divisor) { u64 q=0,r=0; int i=63; div_loop: if(i<0)goto div_loop_end; r<<=1; r|= (dividend>>i)&1; if(r>=divisor){r=bitwise_subtract(r,divisor); q|=1ULL<<i;} i=bitwise_subtract(i,1); goto div_loop; div_loop_end: if(q_ptr){*q_ptr=q;} return r; }
u128 bitwise_divide_and_mod_u128(u128 *q_ptr, u128 dividend, u128 divisor) { u128 q=0,r=0; int i=127; div_loop: if(i<0)goto div_loop_end; r<<=1; r|= (dividend>>i)&1; if(r>=divisor){r=bitwise_subtract(r,divisor); q|=((u128)1)<<i;} i=bitwise_subtract(i,1); goto div_loop; div_loop_end: if(q_ptr){*q_ptr=q;} return r; }

u64 mod_mul(u64 a, u64 b, u64 mod) { u64 r=0; a=bitwise_divide_and_mod_u64(NULL,a,mod); mod_mul_loop: if(b==0)goto mod_mul_loop_end; if(b&1){r=bitwise_add(r,a); if(r>=mod)r=bitwise_subtract(r,mod);} a<<=1; if(a>=mod)a=bitwise_subtract(a,mod); b>>=1; goto mod_mul_loop; mod_mul_loop_end: return r; }
u64 mod_pow(u64 b, u64 e, u64 m) { u64 r=1; b=bitwise_divide_and_mod_u64(NULL,b,m); mod_pow_loop: if(e==0)goto mod_pow_loop_end; if(e&1)r=mod_mul(r,b,m); b=mod_mul(b,b,m); e>>=1; goto mod_pow_loop; mod_pow_loop_end: return r; }
u64 mod_inverse(u64 n, u64 mod) { return mod_pow(n, bitwise_subtract(mod, 2), mod); }

int string_length(char* s) { int l=0; len_loop: if(*(s+l)==0)goto len_loop_end; l=bitwise_add(l,1); goto len_loop; len_loop_end: return l; }
int string_compare(char* s1, char* s2) { compare_loop: if(!(*s1 && (*s1==*s2)))goto compare_loop_end; s1=(char*)bitwise_add((u64)s1,1); s2=(char*)bitwise_add((u64)s2,1); goto compare_loop; compare_loop_end: return *(unsigned char*)s1-*(unsigned char*)s2; }

#define HYBRID_THRESHOLD 20000

#define SIMPLE_NTT_MAX_SIZE 65536
u64 simple_a[SIMPLE_NTT_MAX_SIZE], simple_b[SIMPLE_NTT_MAX_SIZE];

#define CHUNK_SIZE 5
#define BASE 100000
#define COMPLEX_NTT_SIZE 524288
u64 p1_a[COMPLEX_NTT_SIZE], p1_b[COMPLEX_NTT_SIZE];
u64 p2_a[COMPLEX_NTT_SIZE], p2_b[COMPLEX_NTT_SIZE];
u64 p3_a[COMPLEX_NTT_SIZE], p3_b[COMPLEX_NTT_SIZE];
u128 final_coeffs[COMPLEX_NTT_SIZE];

char s1_in[1000005], s2_in[1000005];
int result_digits[2000005];


void ntt_transform(u64 a[], int n, int invert, u64 prime, u64 root) {
    int i=1, j=0;
rev_loop: if(i>=n)goto rev_loop_end; int bit=n>>1; rev_inner_loop: if(!((j&bit)>0))goto rev_inner_loop_end; j^=bit; bit>>=1; goto rev_inner_loop; rev_inner_loop_end: j^=bit;
    if(i<j){ u64 t=a[i]; a[i]=a[j]; a[j]=t; } i=bitwise_add(i,1); goto rev_loop;
rev_loop_end:;
    int len=2;
len_loop: if(len>n)goto len_loop_end;
    u64 exponent; u64 p_minus_1 = bitwise_subtract(prime,1);
    bitwise_divide_and_mod_u64(&exponent, p_minus_1, len);
    u64 wlen=mod_pow(root, exponent, prime);
    if(invert)wlen=mod_inverse(wlen, prime);
    i=0;
outer_butterfly_loop: if(i>=n)goto outer_butterfly_loop_end;
    u64 w=1; j=0;
inner_butterfly_loop: if(j>=(len>>1))goto inner_butterfly_loop_end;
    u64 u_idx=bitwise_add(i,j); u64 v_idx=bitwise_add(u_idx, len>>1);
    u64 u=a[u_idx]; u64 v=mod_mul(a[v_idx],w,prime);
    u64 sum=bitwise_add(u,v); if(sum>=prime)sum=bitwise_subtract(sum,prime); a[u_idx]=sum;
    u64 diff=bitwise_add(u, bitwise_subtract(prime,v)); if(diff>=prime)diff=bitwise_subtract(diff,prime); a[v_idx]=diff;
    w=mod_mul(w,wlen,prime); j=bitwise_add(j,1); goto inner_butterfly_loop;
inner_butterfly_loop_end:
    i=bitwise_add(i,len); goto outer_butterfly_loop;
outer_butterfly_loop_end:
    len<<=1; goto len_loop;
len_loop_end:;
    if(invert){ u64 n_inv=mod_inverse(n,prime); i=0;
inv_loop: if(i>=n)goto inv_loop_end; a[i]=mod_mul(a[i],n_inv,prime); i=bitwise_add(i,1); goto inv_loop;
inv_loop_end:; }
}

void run_simple_multiplication(char* s1, int len1, char* s2, int len2) {
    int n=1; size_loop: if(n>=bitwise_add(len1,len2))goto size_loop_end; n<<=1; goto size_loop; size_loop_end:;
    int i=0; init_loop: if(i>=n)goto init_loop_end; simple_a[i]=0; simple_b[i]=0; i++; goto init_loop; init_loop_end:;
    i=0; str_a_loop: if(i>=len1)goto str_a_loop_end; simple_a[i]=s1[bitwise_subtract(len1,bitwise_add(i,1))]-'0'; i++; goto str_a_loop; str_a_loop_end:;
    i=0; str_b_loop: if(i>=len2)goto str_b_loop_end; simple_b[i]=s2[bitwise_subtract(len2,bitwise_add(i,1))]-'0'; i++; goto str_b_loop; str_b_loop_end:;
    ntt_transform(simple_a, n, 0, P1, G1); ntt_transform(simple_b, n, 0, P1, G1);
    i=0; pointwise_loop: if(i>=n)goto pointwise_loop_end; simple_a[i]=mod_mul(simple_a[i],simple_b[i],P1); i++; goto pointwise_loop; pointwise_loop_end:;
    ntt_transform(simple_a, n, 1, P1, G1);
    u64 carry=0; i=0; carry_loop: if(i>=n && carry==0)goto carry_loop_end; u64 cur=(i<n?simple_a[i]:0); cur=bitwise_add(cur,carry);
    result_digits[i]=bitwise_divide_and_mod_u64(&carry,cur,10); i=bitwise_add(i,1); goto carry_loop; carry_loop_end:;
    int total_digits=i; int start_idx=bitwise_subtract(total_digits,1);
find_msd_loop: if(start_idx<=0||result_digits[start_idx]!=0)goto find_msd_loop_end; start_idx=bitwise_subtract(start_idx,1); goto find_msd_loop; find_msd_loop_end:;
    i=start_idx; print_loop: if(i<0)goto print_loop_end; printf("%d", result_digits[i]); i=bitwise_subtract(i,1); goto print_loop; print_loop_end: printf("\n");
    fflush(stdout);
}

u64 string_to_u64(char* s, int len) { u64 res=0; int i=0; s_to_u64_loop: if(i>=len)return res; res=bitwise_multiply(res,10); res=bitwise_add(res, bitwise_subtract(s[i],'0')); i=bitwise_add(i,1); goto s_to_u64_loop; }
int fill_coeffs_chunked(u64 arr[], char* s, int slen) { int nc=0; int cpos=slen; fill_loop: if(cpos<=0)goto fill_loop_end; int cstart = cpos > CHUNK_SIZE ? bitwise_subtract(cpos,CHUNK_SIZE) : 0; int clen=bitwise_subtract(cpos,cstart); arr[nc]=string_to_u64(s+cstart,clen); nc=bitwise_add(nc,1); cpos=bitwise_subtract(cpos,CHUNK_SIZE); goto fill_loop; fill_loop_end: return nc; }

void run_complex_multiplication(char* s1, int len1, char* s2, int len2) {
    int n_chunks1 = fill_coeffs_chunked(p1_a, s1, len1); int n_chunks2 = fill_coeffs_chunked(p1_b, s2, len2);
    int i=0; copy_loop: if(i>=COMPLEX_NTT_SIZE)goto copy_loop_end; p2_a[i]=p3_a[i]=p1_a[i]; p2_b[i]=p3_b[i]=p1_b[i]; i++; goto copy_loop; copy_loop_end:;
    ntt_transform(p1_a, COMPLEX_NTT_SIZE, 0, P1, G1); ntt_transform(p1_b, COMPLEX_NTT_SIZE, 0, P1, G1);
    ntt_transform(p2_a, COMPLEX_NTT_SIZE, 0, P2, G2); ntt_transform(p2_b, COMPLEX_NTT_SIZE, 0, P2, G2);
    ntt_transform(p3_a, COMPLEX_NTT_SIZE, 0, P3, G3); ntt_transform(p3_b, COMPLEX_NTT_SIZE, 0, P3, G3);
    i=0; pointwise_loop_c: if(i>=COMPLEX_NTT_SIZE)goto pointwise_loop_c_end; p1_a[i]=mod_mul(p1_a[i],p1_b[i],P1); p2_a[i]=mod_mul(p2_a[i],p2_b[i],P2); p3_a[i]=mod_mul(p3_a[i],p3_b[i],P3); i++; goto pointwise_loop_c; pointwise_loop_c_end:;
    ntt_transform(p1_a, COMPLEX_NTT_SIZE, 1, P1, G1); ntt_transform(p2_a, COMPLEX_NTT_SIZE, 1, P2, G2); ntt_transform(p3_a, COMPLEX_NTT_SIZE, 1, P3, G3);
    u64 inv_p1_p2=mod_inverse(P1,P2); u64 inv_p1p2_p3=mod_mul(mod_inverse(P1,P3),mod_inverse(P2,P3),P3); u128 M1=(u128)P1; u128 M12=(u128)P1*P2;
    i=0; crt_loop: if(i>=COMPLEX_NTT_SIZE)goto crt_loop_end; u64 r1=p1_a[i], r2=p2_a[i], r3=p3_a[i];
    u64 k1=mod_mul(bitwise_add(r2,bitwise_subtract(P2, bitwise_divide_and_mod_u64(NULL,r1,P2))),inv_p1_p2,P2);
    u64 t2_term_a = bitwise_add(r3, bitwise_subtract(P3, bitwise_divide_and_mod_u64(NULL,r1,P3)));
    u64 t2_term_b = mod_mul(k1, bitwise_divide_and_mod_u64(NULL,P1,P3), P3);
    u64 t2 = bitwise_add(t2_term_a, bitwise_subtract(P3, t2_term_b));
    u64 k2=mod_mul(t2,inv_p1p2_p3,P3);
    final_coeffs[i] = bitwise_add(bitwise_add((u128)r1, bitwise_multiply((u128)k1, M1)), bitwise_multiply((u128)k2, M12));
    i++; goto crt_loop; crt_loop_end:;
    u128 carry=0; int max_chunks=bitwise_add(n_chunks1,n_chunks2);
    i=0; final_carry_loop: if(i>=bitwise_add(max_chunks,2)&&carry==0)goto final_carry_loop_end;
    u128 cur=final_coeffs[i]; cur=bitwise_add(cur,carry);
    u128 chunk_val=bitwise_divide_and_mod_u128(&carry,cur,BASE);
    int j=0; u64 temp_chunk=(u64)chunk_val;
    digit_save_loop: if(j>=CHUNK_SIZE)goto digit_save_loop_end; u64 digit_val;
    result_digits[bitwise_add(bitwise_multiply(i,CHUNK_SIZE),j)]=bitwise_divide_and_mod_u64(&temp_chunk,temp_chunk,10);
    j++; goto digit_save_loop; digit_save_loop_end:;
    i++; goto final_carry_loop; final_carry_loop_end:;
    int total_digits=bitwise_multiply(i,CHUNK_SIZE);
    int start_idx=bitwise_subtract(total_digits,1);
find_msd_loop_c: if(start_idx<=0||result_digits[start_idx]!=0)goto find_msd_loop_c_end; start_idx--; goto find_msd_loop_c; find_msd_loop_c_end:;
    i=start_idx; print_loop_c: if(i<0)goto print_loop_c_end; printf("%d",result_digits[i]); i--; goto print_loop_c; print_loop_c_end: printf("\n");
    fflush(stdout);
}

int main() {
    printf("Kalkulator Perkalian, 'exit' untuk keluar'\n");
    // fflush(stdout);
master_loop:
    printf("\n> ");
    // fflush(stdout);
    if (scanf("%s", s1_in) == EOF) goto master_loop_end;
    if (string_compare(s1_in, "exit") == 0) goto master_loop_end;
    (void)scanf("%s", s2_in);
    int len1 = string_length(s1_in); int len2 = string_length(s2_in);
    int max_len = len1 > len2 ? len1 : len2;
    if (max_len < HYBRID_THRESHOLD) {
        run_simple_multiplication(s1_in, len1, s2_in, len2);
    } else {
        run_complex_multiplication(s1_in, len1, s2_in, len2);
    }
    goto master_loop;
master_loop_end:
    return 0;
}
