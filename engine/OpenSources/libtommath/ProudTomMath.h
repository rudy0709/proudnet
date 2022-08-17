﻿#define mp_int	pn_mp_int
#define ltm_prime_callback pn_ltm_prime_callback
#define mp_error_to_string pn_mp_error_to_string
#define mp_init pn_mp_init
#define mp_clear pn_mp_clear
#define mp_init_multi pn_mp_init_multi
#define mp_clear_multi pn_mp_clear_multi
#define mp_exch pn_mp_exch
#define mp_shrink pn_mp_shrink
#define mp_grow pn_mp_grow
#define mp_init_size pn_mp_init_size
#define mp_zero pn_mp_zero
#define mp_set pn_mp_set
#define mp_set_int pn_mp_set_int
#define mp_get_int pn_mp_get_int
#define mp_init_set  pn_mp_init_set 
#define mp_init_set_int  pn_mp_init_set_int 
#define mp_copy pn_mp_copy
#define mp_init_copy pn_mp_init_copy
#define mp_clamp pn_mp_clamp
#define mp_rshd pn_mp_rshd
#define mp_lshd pn_mp_lshd
#define mp_div_2d pn_mp_div_2d
#define mp_div_2 pn_mp_div_2
#define mp_mul_2d pn_mp_mul_2d
#define mp_mul_2 pn_mp_mul_2
#define mp_mod_2d pn_mp_mod_2d
#define mp_2expt pn_mp_2expt
#define mp_cnt_lsb pn_mp_cnt_lsb
#define mp_rand pn_mp_rand
#define mp_xor pn_mp_xor
#define mp_or pn_mp_or
#define mp_and pn_mp_and
#define mp_neg pn_mp_neg
#define mp_abs pn_mp_abs
#define mp_cmp pn_mp_cmp
#define mp_cmp_mag pn_mp_cmp_mag
#define mp_add pn_mp_add
#define mp_sub pn_mp_sub
#define mp_mul pn_mp_mul
#define mp_sqr pn_mp_sqr
#define mp_div pn_mp_div
#define mp_mod pn_mp_mod
#define mp_cmp_d pn_mp_cmp_d
#define mp_add_d pn_mp_add_d
#define mp_sub_d pn_mp_sub_d
#define mp_mul_d pn_mp_mul_d
#define mp_div_d pn_mp_div_d
#define mp_div_3 pn_mp_div_3
#define mp_expt_d pn_mp_expt_d
#define mp_mod_d pn_mp_mod_d
#define mp_addmod pn_mp_addmod
#define mp_submod pn_mp_submod
#define mp_mulmod pn_mp_mulmod
#define mp_sqrmod pn_mp_sqrmod
#define mp_invmod pn_mp_invmod
#define mp_gcd pn_mp_gcd
#define mp_exteuclid pn_mp_exteuclid
#define mp_lcm pn_mp_lcm
#define mp_n_root pn_mp_n_root
#define mp_sqrt pn_mp_sqrt
#define mp_is_square pn_mp_is_square
#define mp_jacobi pn_mp_jacobi
#define mp_reduce_setup pn_mp_reduce_setup
#define mp_reduce pn_mp_reduce
#define mp_montgomery_setup pn_mp_montgomery_setup
#define mp_montgomery_calc_normalization pn_mp_montgomery_calc_normalization
#define mp_montgomery_reduce pn_mp_montgomery_reduce
#define mp_dr_is_modulus pn_mp_dr_is_modulus
#define mp_dr_setup pn_mp_dr_setup
#define mp_dr_reduce pn_mp_dr_reduce
#define mp_reduce_is_2k pn_mp_reduce_is_2k
#define mp_reduce_2k_setup pn_mp_reduce_2k_setup
#define mp_reduce_2k pn_mp_reduce_2k
#define mp_reduce_is_2k_l pn_mp_reduce_is_2k_l
#define mp_reduce_2k_setup_l pn_mp_reduce_2k_setup_l
#define mp_reduce_2k_l pn_mp_reduce_2k_l
#define mp_exptmod pn_mp_exptmod
#define mp_prime_is_divisible pn_mp_prime_is_divisible
#define mp_prime_fermat pn_mp_prime_fermat
#define mp_prime_miller_rabin pn_mp_prime_miller_rabin
#define mp_prime_rabin_miller_trials pn_mp_prime_rabin_miller_trials
#define mp_prime_is_prime pn_mp_prime_is_prime
#define mp_prime_next_prime pn_mp_prime_next_prime
#define mp_prime_random_ex pn_mp_prime_random_ex
#define mp_count_bits pn_mp_count_bits
#define mp_unsigned_bin_size pn_mp_unsigned_bin_size
#define mp_read_unsigned_bin pn_mp_read_unsigned_bin
#define mp_to_unsigned_bin pn_mp_to_unsigned_bin
#define mp_to_unsigned_bin_n  pn_mp_to_unsigned_bin_n 
#define mp_signed_bin_size pn_mp_signed_bin_size
#define mp_read_signed_bin pn_mp_read_signed_bin
#define mp_to_signed_bin pn_mp_to_signed_bin
#define mp_to_signed_bin_n  pn_mp_to_signed_bin_n 
#define mp_read_radix pn_mp_read_radix
#define mp_toradix pn_mp_toradix
#define mp_toradix_n pn_mp_toradix_n
#define mp_radix_size pn_mp_radix_size
#define mp_fread pn_mp_fread
#define mp_fwrite pn_mp_fwrite
#define s_mp_add pn_s_mp_add
#define s_mp_sub pn_s_mp_sub
#define fast_s_mp_mul_digs pn_fast_s_mp_mul_digs
#define s_mp_mul_digs pn_s_mp_mul_digs
#define fast_s_mp_mul_high_digs pn_fast_s_mp_mul_high_digs
#define s_mp_mul_high_digs pn_s_mp_mul_high_digs
#define fast_s_mp_sqr pn_fast_s_mp_sqr
#define s_mp_sqr pn_s_mp_sqr
#define mp_karatsuba_mul pn_mp_karatsuba_mul
#define mp_toom_mul pn_mp_toom_mul
#define mp_karatsuba_sqr pn_mp_karatsuba_sqr
#define mp_toom_sqr pn_mp_toom_sqr
#define fast_mp_invmod pn_fast_mp_invmod
#define mp_invmod_slow  pn_mp_invmod_slow 
#define fast_mp_montgomery_reduce pn_fast_mp_montgomery_reduce
#define mp_exptmod_fast pn_mp_exptmod_fast
#define s_mp_exptmod  pn_s_mp_exptmod 
#define bn_reverse pn_bn_reverse
#define mp_s_rmap pn_mp_s_rmap
#define KARATSUBA_MUL_CUTOFF pn_KARATSUBA_MUL_CUTOFF
#define KARATSUBA_SQR_CUTOFF pn_KARATSUBA_SQR_CUTOFF
#define TOOM_MUL_CUTOFF pn_TOOM_MUL_CUTOFF
#define TOOM_SQR_CUTOFF pn_TOOM_SQR_CUTOFF
#define ltm_prime_tab pn_ltm_prime_tab
