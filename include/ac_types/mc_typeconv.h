//-------------------------------------------------
// Conversion functions from various types to sc_lv
//-------------------------------------------------

#include "mc_container_types.h"
#include <assert.h>

// Check for macro definitions that will conflict with template parameter names in this file
#if defined(Twidth)
#define Twidth 0
#error The macro name 'Twidth' conflicts with a Template Parameter name.
#error The compiler should have produced a warning about redefinition of 'Twidth' giving the location of the previous definition.
#endif
#if defined(Ibits)
#define Ibits 0
#error The macro name 'Ibits' conflicts with a Template Parameter name.
#error The compiler should have produced a warning about redefinition of 'Ibits' giving the location of the previous definition.
#endif
#if defined(Qmode)
#define Qmode 0
#error The macro name 'Qmode' conflicts with a Template Parameter name.
#error The compiler should have produced a warning about redefinition of 'Qmode' giving the location of the previous definition.
#endif
#if defined(Omode)
#define Omode 0
#error The macro name 'Omode' conflicts with a Template Parameter name.
#error The compiler should have produced a warning about redefinition of 'Omode' giving the location of the previous definition.
#endif
#if defined(Nbits)
#define Nbits 0
#error The macro name 'Nbits' conflicts with a Template Parameter name.
#error The compiler should have produced a warning about redefinition of 'Nbits' giving the location of the previous definition.
#endif
#if defined(Tclass)
#define Tclass 0
#error The macro name 'Tclass' conflicts with a Template Parameter name.
#error The compiler should have produced a warning about redefinition of 'Tclass' giving the location of the previous definition.
#endif
#if defined(TclassW)
#define TclassW 0
#error The macro name 'TclassW' conflicts with a Template Parameter name.
#error The compiler should have produced a warning about redefinition of 'TclassW' giving the location of the previous definition.
#endif

#if !defined(MC_TYPECONV_H)
//#define MC_TYPECONV_H

   // built-in types and sc_types:

   template < int Twidth>
   void type_to_vector(const sc_int<Twidth> &in, int length, sc_lv<Twidth>& rvec) {
     // sc_assert(length == Twidth);
     rvec = in;
   }

   template < int Twidth>
   void type_to_vector(const sc_bigint<Twidth> &in, int length, sc_lv<Twidth>& rvec) {
     // sc_assert(length == Twidth);
     rvec = in;
   }
#endif

//This section was further down in the file, however, gcc 4.x is complaining about "ambiguous operator overloads" that 
//seem to be remedied by moving this section here (and adding the type_to_vector template functions just above. PT
#if defined(SC_INCLUDE_FX) && !defined(MC_TYPECONV_H_SC_FIXED)
#define MC_TYPECONV_H_SC_FIXED

   // ---------------------------------------------------------
   // ---------------------------------  SC_FIXED
   // SC_FIXED => SC_LV
   template<int Twidth, int Ibits>
   void type_to_vector(const sc_fixed<Twidth,Ibits> &in, int length, sc_lv<Twidth>& rvec) {
     // sc_assert(length == Twidth);
     // rvec = in.range(Twidth-1, 0); // via assignment from sc_bv_base not provided 
     sc_int<Twidth> tmp;
     tmp = in.range(Twidth-1, 0);
     type_to_vector(tmp, length, rvec);
   }

   template<int Twidth, int Ibits, sc_q_mode Qmode, sc_o_mode Omode, int Nbits>
   void type_to_vector(const sc_fixed<Twidth,Ibits,Qmode,Omode,Nbits> &in, int length, sc_lv<Twidth>& rvec) {
     // sc_assert(length == Twidth);
     // rvec = in.range(Twidth-1, 0); // via assignment from sc_bv_base not provided 
     sc_bigint<Twidth> tmp;
     tmp = in.range(Twidth-1, 0);
     type_to_vector(tmp, length, rvec);
   }

   // SC_LV => SC_FIXED
   template<int Twidth, int Ibits, sc_q_mode Qmode, sc_o_mode Omode, int Nbits>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, sc_fixed<Twidth,Ibits,Qmode,Omode,Nbits> *result) {
     sc_bigint<Twidth> tmp = in;
     result->range(Twidth-1, 0) = tmp; // via sc_bv_base
   }
   // ---------------------------------------------------------

   // ---------------------------------------------------------
   // ---------------------------------  SC_UFIXED
   // SC_UFIXED => SC_LV
   template<int Twidth, int Ibits, sc_q_mode Qmode, sc_o_mode Omode, int Nbits>
   void type_to_vector(const sc_ufixed<Twidth,Ibits,Qmode,Omode,Nbits> &in, int length, sc_lv<Twidth>& rvec) {
     // sc_assert(length == Twidth);
     // rvec = in.range(Twidth-1, 0); // via assignment from sc_bv_base not provided 
     sc_bigint<Twidth> tmp;
     tmp = in.range(Twidth-1, 0);
     type_to_vector(tmp, length, rvec);
   }

   // SC_LV => SC_UFIXED
   template<int Twidth, int Ibits, sc_q_mode Qmode, sc_o_mode Omode, int Nbits>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, sc_ufixed<Twidth,Ibits,Qmode,Omode,Nbits> *result) {
     sc_bigint<Twidth> tmp = in;
     result->range(Twidth-1, 0) = tmp;
   }
   // ---------------------------------------------------------
#endif

#if !defined(MC_TYPECONV_H)
#define MC_TYPECONV_H

   // GENERIC => SC_LV
   template <class T, int Twidth>
   void type_to_vector(const T &in, int length, sc_lv<Twidth>& rvec) {
     // sc_assert(length == Twidth);
     rvec = in;
   }


   // ---------------------------------------------------------
   // ---------------------------------  SC_INT
   // SC_INT => SC_LV
     // (uses GENERIC type_to_vector)

   // SC_LV => SC_INT
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, sc_int<Twidth> *result) {
     *result = in;
   }
   // ---------------------------------------------------------

   // ---------------------------------------------------------
   // ---------------------------------  SC_UINT
   // SC_UINT => SC_LV
     // (uses GENERIC type_to_vector)

   // SC_LV => SC_UINT
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, sc_uint<Twidth> *result) {
     *result = in;
   }
   // ---------------------------------------------------------

   // ---------------------------------------------------------
   // ---------------------------------  SC_BIGINT
   // SC_BIGINT => SC_LV
     // (uses GENERIC type_to_vector)

   // SC_LV => SC_BIGINT
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, sc_bigint<Twidth> *result) {
     *result = in;
   }
   // ---------------------------------------------------------

   // ---------------------------------------------------------
   // ---------------------------------  SC_BIGUINT
   // SC_BIGUINT => SC_LV
     // (uses GENERIC type_to_vector)

   // SC_LV => SC_BIGUINT
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, sc_biguint<Twidth> *result) {
     *result = in;
   }
   // ---------------------------------------------------------


   // ---------------------------------------------------------
   // ---------------------------------  BOOL
   // BOOL => SC_LV 
     // (uses GENERIC type_to_vector)

   // SC_LV => BOOL
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, bool *result) {
     *result = (in[0] == sc_dt::Log_1 ? true : false);
   }
   // ---------------------------------------------------------


   // ---------------------------------------------------------
   // ---------------------------------  ***SPECIAL*** sc_logic to bool, bool to sc_logic
   // SC_Logic => BOOL
   inline
   void vector_to_type(const sc_logic &in, bool issigned, bool *result) {
     *result = (in == sc_dt::Log_1 ? true : false);
   }

   // BOOL => SC_Logic
   inline
   void type_to_vector(const bool &in, int length, sc_logic &rvec) {
     rvec = in;
   }
   // ---------------------------------------------------------

   // ---------------------------------------------------------
   // ---------------------------------  ***SPECIAL*** sc_logic to sc_uint<1>, sc_uint<1> to sc_logic
   // Should not really ever need this, but for completeness...
   // SC_Logic => SC_UINT<1>
   inline
   void vector_to_type(const sc_logic &in, bool issigned, sc_uint<1> *result) {
     *result = (in == sc_dt::Log_1 ? 1 : 0);
   }

   // SC_UINT<1> => SC_Logic
   inline
   void type_to_vector(const sc_uint<1> &in, int length, sc_logic &rvec) {
     rvec = in ? sc_dt::Log_1 : sc_dt::Log_0;
   }

   // ---------------------------------  ***SPECIAL*** sc_lv<1> to sc_logic, sc_logic to sc_lv<1>
   inline
   void vector_to_type(const sc_lv<1> &in, bool issigned, sc_logic *result) {
     *result = in[0];
   }

   inline
   void type_to_vector(const sc_logic &in, int length, sc_lv<1> &rvec) {
     rvec[0] = in;
   }
   // ---------------------------------------------------------

   // ---------------------------------  ***SPECIAL*** sc_logic to sc_lv<1>, sc_lv<1> to sc_logic 
   inline
   void vector_to_type(const sc_logic &in, bool issigned, sc_lv<1> *result) {
     (*result)[0] = in;
   }

   inline
   void type_to_vector(const sc_lv<1> &in, int length, sc_logic &rvec) {
     rvec = in[0];
   }
   // ---------------------------------------------------------

   // ---------------------------------  ***SPECIAL*** sc_logic to sc_logic, sc_logic to sc_logic
   inline
   void vector_to_type(const sc_logic &in, bool issigned, sc_logic *result) {
     *result = in;
   }

   inline
   void type_to_vector(const sc_logic &in, int length, sc_logic &rvec) {
     rvec = in;
   }
   // ---------------------------------------------------------

   // ---------------------------------  ***SPECIAL*** sc_lv<N> to sc_lv<N>, 
   template<int Twidth>
   inline
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, sc_lv<Twidth> *result) {
     *result = in;
   }

   template<int Twidth>
   inline
   void type_to_vector(const sc_lv<Twidth> &in, int length, sc_lv<Twidth> &rvec) {
     rvec = in;
   }
   // ---------------------------------------------------------

   // ---------------------------------  sc_bv<N> to sc_lv<N>, sc_lv<N> to sc_bv<N>
   template<int Twidth>
   inline
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, sc_bv<Twidth> *result) {
     *result = in;
   }

   template<int Twidth>
   inline
   void type_to_vector(const sc_bv<Twidth> &in, int length, sc_lv<Twidth> &rvec) {
     rvec = in;
   }
   // ---------------------------------------------------------

   // ---------------------------------  ***SPECIAL*** sc_lv<1> to sc_bit, sc_bit to sc_lv<1>
   inline
   void vector_to_type(const sc_lv<1> &in, bool issigned, sc_bit *result) {
     *result = in[0];
   }

   inline
   void type_to_vector(const sc_bit &in, int length, sc_lv<1> &rvec) {
     rvec = in;
   }
   // ---------------------------------------------------------

   template<int Twidth, class T>
   void vector_to_type_builtin(const sc_lv<Twidth> &in, bool issigned, T *result) {
     // sc_assert(sizeof(T) <= sizeof(int));
     if (issigned) {
       *result = in.to_int();
     } else { 
       *result = in.to_uint();
     }
   }

   template<int Twidth, class T>
   void vector_to_type_builtin_64(const sc_lv<Twidth> &in, bool issigned, T *result) {
     // sc_assert(sizeof(T) * CHAR_BIT <= 64);
     if (issigned) {
       sc_int<Twidth < 64 ? Twidth : 64> i;
       i = in;
       *result = i.to_int64();
     } else {
       sc_uint<Twidth < 64 ? Twidth : 64> u;
       u = in;
       *result = u.to_uint64();
     }
   }

   // ---------------------------------------------------------
   // --------------------------------- DOUBLE
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, double *result) {
   //#error \'Double\' datatype is unsupported for vector_to_type function
   }
   // ---------------------------------------------------------

   // ---------------------------------------------------------
   // --------------------------------- FLOAT
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, float *result) {
   //#error \'Float\' datatype is unsupported for vector_to_type function
   }
   // ---------------------------------------------------------


   // ---------------------------------------------------------
   // --------------------------------- GENERIC

   template<int Twidth, class T>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, T *result) {
     // sc_assert(sizeof(T) <= sizeof(int));
     assert(Twidth<=sizeof(T)*CHAR_BIT);
     if (sizeof(T) <= sizeof(int)) {
       if (issigned) {
         *result = static_cast<T> (in.to_int());
       } else { 
         *result = static_cast<T> (in.to_uint());
       }
     } else {
       if (issigned) {
         sc_int<Twidth < 64 ? Twidth : 64> i;
         i = in;
         *result = static_cast<T> (i.to_int64());
       } else {
         sc_uint<Twidth < 64 ? Twidth : 64> u;
         u = in;
         *result = static_cast<T> (u.to_uint64());
       }
     }
   }
   // ---------------------------------------------------------

   // ---------------------------------------------------------
   // --------------------------------- CHAR
   // CHAR => SC_LV
     // (uses GENERIC type_to_vector)
   
   // SC_LV => CHAR
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, char *result) {
     vector_to_type_builtin(in, issigned, result);
   }
   // ---------------------------------------------------------


   // ---------------------------------------------------------
   // --------------------------------- UNSIGNED CHAR
   // UNSIGNED CHAR => SC_LV
     // (uses GENERIC type_to_vector)
   
   // SC_LV => UNSIGNED CHAR
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, unsigned char *result) {
     vector_to_type_builtin(in, issigned, result);
   }
   // ---------------------------------------------------------
   
   // ---------------------------------------------------------
   // --------------------------------- SIGNED CHAR
   // SC_LV => SIGNED CHAR
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, signed char *result) {
     vector_to_type_builtin(in, issigned, result);
   }
   // ---------------------------------------------------------

   
   // ---------------------------------------------------------
   // --------------------------------- SHORT
   // SHORT => SC_LV
     // (uses GENERIC type_to_vector)
   
   // SC_LV => SHORT
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, short *result) {
     vector_to_type_builtin(in, issigned, result);
   }
   // ---------------------------------------------------------
   
   // ---------------------------------------------------------
   // --------------------------------- UNSIGNED SHORT
   // UNSIGNED SHORT => SC_LV
     // (uses GENERIC type_to_vector)
   
   // SC_LV => UNSIGNED SHORT
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, unsigned short *result) {
     vector_to_type_builtin(in, issigned, result);
   }
   
   // ---------------------------------------------------------
   // --------------------------------- INT
   // INT => SC_LV
     // (uses GENERIC type_to_vector)
   
   // SC_LV => INT
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, int *result) {
     vector_to_type_builtin(in, issigned, result);
   }
   
   // ---------------------------------------------------------
   // --------------------------------- UNSIGNED INT
   // UNSIGNED INT => SC_LV
     // (uses GENERIC type_to_vector)
   
   // SC_LV => UNSIGNED INT
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, unsigned int *result) {
     vector_to_type_builtin(in, issigned, result);
   }
   
   // ---------------------------------------------------------
   // --------------------------------- LONG
   // LONG => SC_LV
     // (uses GENERIC type_to_vector)
   
   // SC_LV => LONG
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, long *result) {
     vector_to_type_builtin_64(in, issigned, result);
   }
   
   // ---------------------------------------------------------
   // --------------------------------- UNSIGNED LONG
   // UNSIGNED LONG => SC_LV
     // (uses GENERIC type_to_vector)
   
   // SC_LV => UNSIGNED LONG
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, unsigned long *result) {
     vector_to_type_builtin_64(in, issigned, result);
   }
   
   // ---------------------------------------------------------
   // --------------------------------- LONG LONG
   // LONG LONG => SC_LV
     // (uses GENERIC type_to_vector)
   
   // SC_LV => LONG LONG
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, long long *result) {
     vector_to_type_builtin_64(in, issigned, result);
   }
   
   // ---------------------------------------------------------
   // --------------------------------- UNSIGNED LONG LONG
   // UNSIGNED LONG LONG => SC_LV
     // (uses GENERIC type_to_vector)
   
   // SC_LV => UNSIGNED LONG LONG
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, unsigned long long *result) {
     vector_to_type_builtin_64(in, issigned, result);
   }
   
   // ---------------------------------------------------------
   // --------------------------------- CONTAINER
   // CONTAINER mgc_sysc_ver_array1D => SC_LV
   template <class Tclass, int V, int Twidth>
   void type_to_vector(const mgc_sysc_ver_array1D<Tclass,V> &in, int length, sc_lv<Twidth>& rvec)
   {
     // sc_assert(Twidth == length);
     // sc_assert(Twidth % V == 0);
     const int element_length = Twidth / V;
     sc_lv<element_length> el_vec;
     for (int i = 0; i < V; ++i) {
       type_to_vector(in[i], element_length, el_vec);
       rvec.range((i + 1) * element_length - 1, i * element_length) = el_vec;
     }
   }
   
   // SC_LV => CONTAINER mgc_sysc_ver_array1D
   template <int Twidth, class Tclass, int TclassW>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, mgc_sysc_ver_array1D<Tclass,TclassW> *result) 
   {
     // sc_assert(Twidth > 0 && Twidth % TclassW == 0);
     enum { ew = Twidth/TclassW }; 
     for (int i = 0; i < TclassW; ++i) {
       sc_lv<ew> tmp = in.range((i + 1) * ew - 1, i * ew);
       vector_to_type(tmp, issigned, &result->operator[](i));
     } 
   }

#endif


#if defined(__AC_INT_H) && !defined(MC_TYPECONV_H_AC_INT)
#define MC_TYPECONV_H_AC_INT

   #include <ac_sc.h>

   // AC_INT => SC_LV
   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, ac_int<Twidth,true> *result) {
     sc_bigint<Twidth> tmp;
     vector_to_type(in, issigned, &tmp);
     *result = to_ac(tmp);
   }

   template<int Twidth>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, ac_int<Twidth,false> *result) {
     sc_biguint<Twidth> tmp;
     vector_to_type(in, issigned, &tmp);
     *result = to_ac(tmp);
   }

   // SC_LV => AC_INT
   template<int Twidth>
   void type_to_vector(const ac_int<Twidth,true> &in, int length, sc_lv<Twidth>& rvec) {
     sc_bigint<Twidth> tmp;
     tmp = to_sc(in);
     type_to_vector(tmp, length, rvec); 
   }

   template<int Twidth>
   void type_to_vector(const ac_int<Twidth,false> &in, int length, sc_lv<Twidth>& rvec) {
     sc_biguint<Twidth> tmp;
     tmp = to_sc(in);
     type_to_vector(tmp, length, rvec); 
   }

   // SC_LOGIC => AC_INT
   inline
   void vector_to_type(const sc_logic &in, bool issigned, ac_int<1,false> *result) {
     sc_uint<1> tmp;
     vector_to_type(in,false,&tmp);
     *result = tmp.to_uint();
   }

   // AC_INT => SC_LOGIC
   inline
   void type_to_vector(const ac_int<1,false> &in, int length, sc_logic &rvec) {
     sc_uint<1> tmp = in.to_uint();
     type_to_vector(tmp,1,rvec);
   }

#endif

#if defined(__AC_FIXED_H) && !defined(MC_TYPECONV_H_AC_FIXED)
#define MC_TYPECONV_H_AC_FIXED

   #include "ac_sc.h"

   // AC_FIXED => SC_LV
   template<int Twidth, int Ibits, bool Signed, ac_q_mode Qmode, ac_o_mode Omode>
   void type_to_vector(const ac_fixed<Twidth,Ibits,Signed,Qmode,Omode> &in, int length, sc_lv<Twidth>& rvec) {
     ac_int<Twidth,Signed> tmp;
     tmp = in.template slc<Twidth>(0);
     type_to_vector(tmp, length, rvec);
   }

   // SC_LV => AC_FIXED
   template<int Twidth, int Ibits, bool Signed, ac_q_mode Qmode, ac_o_mode Omode>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, ac_fixed<Twidth,Ibits,Signed,Qmode,Omode> *result) {
     ac_int<Twidth,Signed> tmp;
     vector_to_type(in, issigned, &tmp);
     result->set_slc(0, tmp);
   }
#endif

#if defined(__AC_FLOAT_H) && !defined(MC_TYPECONV_H_AC_FLOAT)
#define MC_TYPECONV_H_AC_FLOAT

   #include "ac_sc.h"

   // AC_FLOAT => SC_LV
   template<int Twidth, int MTbits, int MIbits, int Ebits, ac_q_mode Qmode>
   void type_to_vector(const ac_float<MTbits,MIbits,Ebits,Qmode> &in, int length, sc_lv<Twidth>& rvec) {
     ac_int<MTbits+Ebits,false> tmp;
     tmp.set_slc(0, in.mantissa().template slc<MTbits>(0));
     tmp.set_slc(MTbits, in.exp().template slc<Ebits>(0));
     type_to_vector(tmp, length, rvec);
   }

   // SC_LV => AC_FLOAT
   template<int Twidth, int MTbits, int MIbits, int Ebits, ac_q_mode Qmode>
   void vector_to_type(const sc_lv<Twidth> &in, bool issigned, ac_float<MTbits,MIbits,Ebits,Qmode> *result) {
     ac_int<MTbits+Ebits,false> tmp;
     ac_int<Ebits,false> tmp_exp;
     ac_fixed<MTbits,MIbits,false> tmp_mantissa;
     vector_to_type(in, issigned, &tmp);
     tmp_exp.set_slc(0, tmp.template slc<Ebits>(MIbits));
     tmp_mantissa.set_slc(0, tmp.template slc<MIbits>(0));
     result->set_mantissa(tmp_mantissa);
     result->set_exp(tmp_exp);
   }

#endif
