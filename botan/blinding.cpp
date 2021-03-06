/*************************************************
* Blinder Source File                            *
* (C) 1999-2007 Jack Lloyd                       *
*************************************************/

#include <botan/blinding.h>
#include <botan/numthry.h>

namespace Botan {

/*************************************************
* Blinder Constructor                            *
*************************************************/
Blinder::Blinder(const BigInt& e, const BigInt& d, const BigInt& n)
   {
   if(e < 1 || d < 1 || n < 1)
      throw Invalid_Argument("Blinder: Arguments too small");

   reducer = Modular_Reducer(n);
   this->e = e;
   this->d = d;
   }

/*************************************************
* Blind a number                                 *
*************************************************/
BigInt Blinder::blind(const BigInt& i) const
   {
   if(!reducer.initialized())
      return i;

   e = reducer.square(e);
   d = reducer.square(d);
   return reducer.multiply(i, e);
   }

/*************************************************
* Unblind a number                               *
*************************************************/
BigInt Blinder::unblind(const BigInt& i) const
   {
   if(!reducer.initialized())
      return i;
   return reducer.multiply(i, d);
   }

}
