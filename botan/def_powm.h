/*************************************************
* Modular Exponentiation Header File             *
* (C) 1999-2007 Jack Lloyd                       *
*************************************************/

#ifndef BOTAN_DEFAULT_MODEXP_H__
#define BOTAN_DEFAULT_MODEXP_H__

#include <botan/pow_mod.h>
#include <botan/reducer.h>
#include <vector>

namespace Botan {

/*************************************************
* Fixed Window Exponentiator                     *
*************************************************/
class BOTAN_DLL Fixed_Window_Exponentiator : public Modular_Exponentiator
   {
   public:
      void set_exponent(const BigInt&);
      void set_base(const BigInt&);
      BigInt execute() const;

      Modular_Exponentiator* copy() const
         { return new Fixed_Window_Exponentiator(*this); }

      Fixed_Window_Exponentiator(const BigInt&, Power_Mod::Usage_Hints);
   private:
      Modular_Reducer reducer;
      BigInt exp;
      u32bit window_bits;
      std::vector<BigInt> g;
      Power_Mod::Usage_Hints hints;
   };

/*************************************************
* Montgomery Exponentiator                       *
*************************************************/
class BOTAN_DLL Montgomery_Exponentiator : public Modular_Exponentiator
   {
   public:
      void set_exponent(const BigInt&);
      void set_base(const BigInt&);
      BigInt execute() const;

      Modular_Exponentiator* copy() const
         { return new Montgomery_Exponentiator(*this); }

      Montgomery_Exponentiator(const BigInt&, Power_Mod::Usage_Hints);
   private:
      BigInt exp, modulus;
      BigInt R2, R_mod;
      std::vector<BigInt> g;
      word mod_prime;
      u32bit mod_words, exp_bits, window_bits;
      Power_Mod::Usage_Hints hints;
   };

}

#endif
