// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include <CGAL/Cartesian_converter.h>
#include <CGAL/gmpxx.h>

namespace CGALUtils {

template <>
double KernelConverter<CGAL::Cartesian<CGAL::Gmpq>, CGAL::Epick>::operator()(
  const CGAL::Gmpq& n) const
{
  return CGAL::to_double(n);
}

template <>
CGAL::Gmpq KernelConverter<CGAL::Epick, CGAL::Cartesian<CGAL::Gmpq>>::operator()(
  const double& n) const
{
  return n;
}

template <>
CGAL::Epeck::FT KernelConverter<CGAL::Epick, CGAL::Epeck>::operator()(
  const double& n) const
{
  return n;
}

template <>
CGAL::Epick::FT KernelConverter<CGAL::Epeck, CGAL::Epick>::operator()(
  const CGAL::Epeck::FT& n) const
{
  return CGAL::to_double(n);
}

template <>
CGAL::Epeck::FT KernelConverter<CGAL::Cartesian<CGAL::Gmpq>, CGAL::Epeck>::operator()(
  const CGAL::Gmpq& n) const
{
  return mpq_class(mpz_class(n.numerator().mpz()), mpz_class(n.denominator().mpz()));
}

template <>
CGAL::Gmpq KernelConverter<CGAL::Epeck, CGAL::Cartesian<CGAL::Gmpq>>::operator()(
  const CGAL::Epeck::FT& n) const
{
  auto& e = n.exact();
  return CGAL::Gmpq(CGAL::Gmpz(e.get_num().get_mpz_t()), CGAL::Gmpz(e.get_den().get_mpz_t()));
}



template <>
CGAL::Epeck::FT KernelConverter<CGAL::Cartesian<SingletonNumber<CGAL::Gmpq>>, CGAL::Epeck>::operator()(
  const SingletonNumber<CGAL::Gmpq>& n) const
{
  auto &nn = n.underlyingValue();
  return mpq_class(mpz_class(nn.numerator().mpz()), mpz_class(nn.denominator().mpz()));
}

template <>
SingletonNumber<CGAL::Gmpq> KernelConverter<CGAL::Epeck, CGAL::Cartesian<SingletonNumber<CGAL::Gmpq>>>::operator()(
  const CGAL::Epeck::FT& n) const
{
  auto& e = n.exact();
  return SingletonNumber<CGAL::Gmpq>(CGAL::Gmpq(CGAL::Gmpz(e.get_num().get_mpz_t()), CGAL::Gmpz(e.get_den().get_mpz_t())));
}

template <>
SingletonNumber<CGAL::Gmpq> KernelConverter<CGAL::Epick, CGAL::Cartesian<SingletonNumber<CGAL::Gmpq>>>::operator()(
  const double& n) const
{
  return SingletonNumber<CGAL::Gmpq>(n);
}

template <>
CGAL::Epick::FT KernelConverter<CGAL::Cartesian<SingletonNumber<CGAL::Gmpq>>, CGAL::Epick>::operator()(
  const CGAL::Cartesian<SingletonNumber<CGAL::Gmpq>>::FT& n) const
{
  return CGAL::to_double(n);
}

} // namespace CGALUtils
