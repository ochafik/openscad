// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
//
// Based on my related "fuzzy number" attempt posted here: https://github.com/CGAL/cgal/issues/5490
// 
#pragma once

#include <cmath>
#include <csignal>
#include <iostream>
#include <type_traits>
#include <unordered_map>

template <class FT>
class SingletonNumber;

template <class FT>
class SingletonCache {
  typedef size_t NumberId;
  typedef std::pair<NumberId, NumberId> NumberPair;

  struct NumberPairHash
  {
      std::size_t operator() (const NumberPair &pair) const {
          return std::hash<NumberId>()(pair.first) ^ std::hash<NumberId>()(pair.second);
      }
  };

  typedef std::unordered_map<NumberId, NumberId> UnaryNumericOpCache;
  typedef std::unordered_map<NumberPair, NumberId, NumberPairHash> BinaryNumericOpCache;
  typedef std::unordered_map<NumberPair, bool, NumberPairHash> BinaryBooleanOpCache;

  friend class SingletonNumber<FT>;

  std::vector<FT> values_;
  std::vector<double> doubleValues_;
  std::unordered_multimap<double, NumberId> idsByDoubleValue_;

#ifdef DEBUG
  std::unordered_map<double, size_t> debugDoubleResolutions_;
#endif

public:
  SingletonCache() : values_(2), doubleValues_(2) {
    // id=0 has value zero built by FT()
    idsByDoubleValue_.insert(std::make_pair(0.0, 0));
    idsByDoubleValue_.insert(std::make_pair(0.0, 1));
    assert(values_[0] == FT(0.0));
    assert(doubleValues_[0] == 0.0);
    values_[1] = FT(1.0);
    doubleValues_[1] = 1.0;
  }


  // SingletonCache<Gmpq>::makeFractionOpsCache takes 2 ids of Gmpz as key and gives a Gmpq value valid in SingletonCache<Gmpq>
  BinaryNumericOpCache makeFractionOpsCache;

  UnaryNumericOpCache unaryMinusOpsCache;
  UnaryNumericOpCache sqrtOpsCache;

  BinaryNumericOpCache plusOpsCache;
  BinaryNumericOpCache minusOpsCache;
  BinaryNumericOpCache timesOpsCache;
  BinaryNumericOpCache divideOpsCache;
  BinaryNumericOpCache minOpsCache;
  BinaryNumericOpCache maxOpsCache;

  BinaryBooleanOpCache equalOpsCache;
  BinaryBooleanOpCache lessOpsCache;
  BinaryBooleanOpCache greaterOpsCache;

  void printStats() {
    std::cout << "values_: " << values_.size() << "\n";
    std::cout << "doubleValues_: " << doubleValues_.size() << "\n";
    std::cout << "idsByDoubleValue_: " << idsByDoubleValue_.size() << "\n";
    std::cout << "makeFractionOpsCache: " << makeFractionOpsCache.size() << "\n";
    std::cout << "unaryMinusOpsCache: " << unaryMinusOpsCache.size() << "\n";
    std::cout << "sqrtOpsCache: " << sqrtOpsCache.size() << "\n";
    std::cout << "plusOpsCache: " << plusOpsCache.size() << "\n";
    std::cout << "minusOpsCache: " << minusOpsCache.size() << "\n";
    std::cout << "timesOpsCache: " << timesOpsCache.size() << "\n";
    std::cout << "divideOpsCache: " << divideOpsCache.size() << "\n";
    std::cout << "minOpsCache: " << minOpsCache.size() << "\n";
    std::cout << "maxOpsCache: " << maxOpsCache.size() << "\n";
    std::cout << "equalOpsCache: " << equalOpsCache.size() << "\n";
    std::cout << "lessOpsCache: " << lessOpsCache.size() << "\n";
    std::cout << "greaterOpsCache: " << greaterOpsCache.size() << "\n";

#ifdef DEBUG
    std::vector<std::pair<double, size_t>> pairs(debugDoubleResolutions_.begin(), debugDoubleResolutions_.end());

    struct {
      bool operator()(const std::pair<double, size_t>& a, const std::pair<double, size_t>& b) const {
        if (a.second == b.second) {
          return a.first < b.first;
        }
        return a.second > b.second;
      }
    } order;
    std::sort(pairs.begin(), pairs.end(), order);
    
    std::cerr << "Common resolved doubles (" << pairs.size() << "):\n";
    for (auto i = 0; i < pairs.size() && i < 20; i++) {
      std::cerr << "  " << pairs[i].first << ": \tresolved " << pairs[i].second << " times.\n";
    }
#endif
  }

  NumberId getId(const FT& value) {
    auto doubleValue = CGAL::to_double(value);
#ifdef DEBUG
    debugDoubleResolutions_[doubleValue]++;
#endif
    if (doubleValue == 0 && value == 0) {
      // Fast track, no hashtable lookups.
      return 0;
    }

#if 1
    auto it = idsByDoubleValue_.find(doubleValue);
    while (it != idsByDoubleValue_.end()) {
      if (it->first != doubleValue) {
        break;
        // throw 0;
      }

      // Multiple numbers can have the same double approximation but be actually different in exact precision.
      // Exact (potentially very expensive) equality here.
      auto existingId = it->second;
      auto &existingValue = getValue(existingId);
      if (existingValue == value) {
#ifdef DEBUG
        if (CGAL::to_double(existingValue) != doubleValue) {
          throw 0;
        }
#endif
        return existingId;
      }
      ++it;
    }
#endif

    auto newId = values_.size();
    idsByDoubleValue_.insert(it, std::make_pair(doubleValue, newId));
    values_.push_back(value);
    doubleValues_.push_back(doubleValue);
    return newId;
  }

  const FT& getValue(NumberId id) {
    return values_[id];
  }
  double getDoubleValue(NumberId id) {
    return doubleValues_[id];
  }

};

// template <typename FT>
// SingletonCache<FT>& getCache();

template <class FT>
class SingletonNumber {
  typedef size_t NumberId;
  typedef SingletonNumber<FT> Type;

  // static SingletonCache<FT> cache;

  NumberId id_;
  
  // THe bool is just here to avoid any implicit usage & conflict with other ctors.
  SingletonNumber(NumberId id, bool) : id_(id) {}
  
public:
  static SingletonCache<FT> cache;

  template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, bool> = true>
  SingletonNumber(const T& value) : id_(value == 0 ? 0 : value == 1 ? 1 : cache.getId(FT(value))) {} // static_cast<double>(value)?

  SingletonNumber(double value) : id_(value == 0 ? 0 : value == 1 ? 1 : cache.getId(FT(value))) {}
  SingletonNumber(const FT& value) : id_(cache.getId(value)) {}
  SingletonNumber(const Type& other) : id_(other.id_) {}
  SingletonNumber() : id_(0) {} //cache.getId(FT(0.0))) {}

  const FT& underlyingValue() const {
    return cache.getValue(id_);
  }
  explicit operator double() const {
    return cache.getDoubleValue(id_);
  }
  
  bool isZero() const {
    return id_ == 0;
  }
  
  bool isOne() const {
    return id_ == 1;
  }

  Type &operator=(const Type& other) {
    id_ = other.id_;
    return *this;
  }

#define SINGLETON_NUMBER_OP_TEMPLATE(T) \
  template <typename T, std::enable_if_t<(std::is_arithmetic<T>::value || std::is_assignable<FT, T>::value), bool> = true>

  template <class Operation, class CacheType>
  static auto getCachedUnaryOp(NumberId operandId, CacheType &cache, Operation op) {
    auto it = cache.find(operandId);
    if (it != cache.end()) {
      return it->second;
    }
    
    auto result = op(operandId);
    cache.insert(it, std::make_pair(operandId, result));
    return result;
  }

  template <class Operation, class CacheType>
  static auto getCachedBinaryOp(NumberId lhsId, NumberId rhsId, CacheType &cache, Operation op) {
    auto key = std::make_pair(lhsId, rhsId);
    auto it = cache.find(key);
    if (it != cache.end()) {
      return it->second;
    }
    
    auto result = op(lhsId, rhsId);
    cache.insert(it, std::make_pair(key, result));
    return result;
  }

  template <class RawOperation>
  struct UnaryNumericOp {
    NumberId operator()(NumberId operand) {
      RawOperation op;
      return cache.getId(op(cache.getValue(operand)));
    }
  };

  template <class RawOperation>
  struct BinaryNumericOp {
    NumberId operator()(NumberId lhs, NumberId rhs) {
      RawOperation op;
      return cache.getId(op(cache.getValue(lhs), cache.getValue(rhs)));
    }
  };

  template <class RawOperation>
  struct BinaryBooleanOp {
    bool operator()(NumberId lhs, NumberId rhs) {
      RawOperation op;
      return op(cache.getValue(lhs), cache.getValue(rhs));
    }
  };

  Type operator-() const {
    struct Op {
      FT operator()(const FT &operand) {
        return -operand;
      }
    };
    return Type(getCachedUnaryOp(id_, cache.unaryMinusOpsCache, UnaryNumericOp<Op>()), false);
  }

  Type sqrt() const {
    struct Op {
      FT operator()(const FT &operand) {
        return std::sqrt(operand);
      }
    };
    return Type(getCachedUnaryOp(id_, cache.sqrtOpsCache, UnaryNumericOp<Op>()), false);
  }

  bool operator==(const Type& other) const {
    if (id_ == other.id_) {
      return true;
    }
    if (isZero() != other.isZero()) {
      return false;
    }
    if (isOne() != other.isOne()) {
      return false;
    }
    struct Op {
      bool operator()(const FT &lhs, const FT& rhs) {
        return lhs == rhs;
      }
    };
    return getCachedBinaryOp(id_, other.id_, cache.equalOpsCache, BinaryBooleanOp<Op>());
  }
  SINGLETON_NUMBER_OP_TEMPLATE(T) bool operator==(const T& x) const {
    // Fast track to avoid resolving x's id.
    if (x == 0) {
      return isZero();
    } else if (isZero()) {
      return false;
    }
    if (x == 1) {
      return isOne();
    } else if (isOne()) {
      return false;
    }
    return *this == Type(x);
  }

  bool operator<(const Type& other) const {
    if (id_ == other.id_) {
      return false;
    }
    struct Op {
      bool operator()(const FT &lhs, const FT& rhs) {
        return lhs < rhs;
      }
    };
    return getCachedBinaryOp(id_, other.id_, cache.lessOpsCache, BinaryBooleanOp<Op>());
  }
  SINGLETON_NUMBER_OP_TEMPLATE(T) bool operator<(const T& x) const {
    // Fast track to avoid resolving x's id.
    if (x == 0 && isZero()) {
      return false;
    }
    // if (x == 1 && isOne()) {
    //   return false;
    // }
    return *this < Type(x);
  }

  // a >= b if !(a < b)
  bool operator>=(const Type& other) const {
    return !(*this < other);
  }
  SINGLETON_NUMBER_OP_TEMPLATE(T) bool operator>=(const T& x) const {
    return !(*this < x);
  }

  bool operator>(const Type& other) const {
    if (id_ == other.id_) {
      return false;
    }
    struct Op {
      bool operator()(const FT &lhs, const FT& rhs) {
        return lhs > rhs;
      }
    };
    return getCachedBinaryOp(id_, other.id_, cache.greaterOpsCache, BinaryBooleanOp<Op>());
  }
  SINGLETON_NUMBER_OP_TEMPLATE(T) bool operator>(const T& x) const {
    // Fast track to avoid resolving x's id.
    if (x == 0 && isZero()) {
      return false;
    }
    // if (x == 1 && isOne()) {
    //   return false;
    // }
    return *this > Type(x);
  }

  // a <= b if !(a > b)
  bool operator<=(const Type& other) const {
    return !(*this > other);
  }
  SINGLETON_NUMBER_OP_TEMPLATE(T) bool operator<=(const T& x) const {
    return !(*this > x);
  }

  bool operator!=(const Type& other) const {
    return !(*this == other);
  }
  SINGLETON_NUMBER_OP_TEMPLATE(T) bool operator!=(const T& x) const {
    // Fast track to avoid resolving x's id.
    if (x == 0) {
      return !isZero();
    } else if (isZero()) {
      return false;
    }
    if (x == 1) {
      return !isOne();
    } else if (isOne()) {
      return false;
    }
    return !(*this == Type(x));
  }

  Type min(const Type& other) const {
    struct Op {
      FT operator()(const FT &lhs, const FT& rhs) {
        return std::min(lhs, rhs);
      }
    };
    return Type(getCachedBinaryOp(id_, other.id_, cache.minOpsCache, BinaryNumericOp<Op>()), false);
  }

  Type max(const Type& other) const {
    struct Op {
      FT operator()(const FT &lhs, const FT& rhs) {
        return std::max(lhs, rhs);
      }
    };
    return Type(getCachedBinaryOp(id_, other.id_, cache.maxOpsCache, BinaryNumericOp<Op>()), false);
  }

  Type operator+(const Type& other) const {
    if (isZero()) {
      return other;
    }
    if (other.isZero()) {
      return *this;
    }
    struct Op {
      FT operator()(const FT &lhs, const FT& rhs) {
        return lhs + rhs;
      }
    };
    return Type(getCachedBinaryOp(id_, other.id_, cache.plusOpsCache, BinaryNumericOp<Op>()), false);
  }
  SINGLETON_NUMBER_OP_TEMPLATE(T) SingletonNumber<FT> operator+(const T& x) const {
    if (x == 0) {
      return *this;
    }
    return *this + Type(x);
  }
  SingletonNumber<FT>& operator+=(const Type& x) {
    return *this = *this + x;
  }
  SINGLETON_NUMBER_OP_TEMPLATE(T) SingletonNumber<FT>& operator+=(const T& x) {
    return *this = *this + x;
  }

  Type operator-(const Type& other) const {
    if (isZero()) {
      return -other;
    }
    if (other.isZero()) {
      return *this;
    }
    struct Op {
      FT operator()(const FT &lhs, const FT& rhs) {
        return lhs - rhs;
      }
    };
    return Type(getCachedBinaryOp(id_, other.id_, cache.minusOpsCache, BinaryNumericOp<Op>()), false);
  }
  SINGLETON_NUMBER_OP_TEMPLATE(T) SingletonNumber<FT> operator-(const T& x) const {
    if (x == 0) {
      return *this;
    }
    return *this - Type(x);
  }
  SingletonNumber<FT>& operator-=(const Type& x) {
    return *this = *this - x;
  }
  SINGLETON_NUMBER_OP_TEMPLATE(T) SingletonNumber<FT>& operator-=(const T& x) {
    return *this = *this - x;
  }

  Type operator*(const Type& other) const {
    if (isZero()) {
      return *this;
    }
    if (other.isZero()) {
      return other;
    }
    if (isOne()) {
      return other;
    }
    if (other.isOne()) {
      return *this;
    }
    struct Op {
      FT operator()(const FT &lhs, const FT& rhs) {
        return lhs * rhs;
      }
    };
    return Type(getCachedBinaryOp(id_, other.id_, cache.timesOpsCache, BinaryNumericOp<Op>()), false);
  }
  SINGLETON_NUMBER_OP_TEMPLATE(T) SingletonNumber<FT> operator*(const T& x) const {
    if (x == 0) {
      return Type(0);
    }
    return *this * Type(x);
  }
  SingletonNumber<FT>& operator*=(const Type& x) {
    return *this = *this * x;
  }
  SINGLETON_NUMBER_OP_TEMPLATE(T) SingletonNumber<FT>& operator*=(const T& x) {
    return *this = *this * x;
  }

  Type operator/(const Type& other) const {
    if (isZero()) {
      return *this;
    }
    if (other.isZero()) {
      raise(SIGFPE); // Throw a floating point exception.
      throw 0; // We won't reach this point.
    }
    if (other.isOne()) {
      return *this;
    }
    struct Op {
      FT operator()(const FT &lhs, const FT& rhs) {
        return lhs / rhs;
      }
    };
    return Type(getCachedBinaryOp(id_, other.id_, cache.divideOpsCache, BinaryNumericOp<Op>()), false);
  }
  SINGLETON_NUMBER_OP_TEMPLATE(T) SingletonNumber<FT> operator/(const T& x) const {
    if (x == 0) {
      return Type(0);
    }
    return *this / Type(x);
  }
  SingletonNumber<FT>& operator/=(const Type& x) {
    return *this = *this / x;
  }
  SINGLETON_NUMBER_OP_TEMPLATE(T) SingletonNumber<FT>& operator/=(const T& x) {
    return *this = *this / x;
  }

};

#define SINGLETON_NUMBER_BINARY_NUMERIC_OP_FN(functionName, op) \
  template <class T, class FT, std::enable_if_t<std::is_arithmetic<T>::value, bool> = true> \
  SingletonNumber<FT> functionName(const T& lhs, const SingletonNumber<FT>& rhs) { \
    return SingletonNumber<FT>(lhs) op rhs; \
  }

#define SINGLETON_NUMBER_BINARY_BOOL_OP_FN(functionName, op) \
  template <class T, class FT, std::enable_if_t<std::is_arithmetic<T>::value, bool> = true> \
  bool functionName(const T& lhs, const SingletonNumber<FT>& rhs) { \
    return SingletonNumber<FT>(lhs) op rhs; \
  }

SINGLETON_NUMBER_BINARY_NUMERIC_OP_FN(operator+, +)
SINGLETON_NUMBER_BINARY_NUMERIC_OP_FN(operator-, -)
SINGLETON_NUMBER_BINARY_NUMERIC_OP_FN(operator*, *)
SINGLETON_NUMBER_BINARY_NUMERIC_OP_FN(operator/, /)

SINGLETON_NUMBER_BINARY_BOOL_OP_FN(operator<, <)
SINGLETON_NUMBER_BINARY_BOOL_OP_FN(operator>, >)
SINGLETON_NUMBER_BINARY_BOOL_OP_FN(operator<=, <=)
SINGLETON_NUMBER_BINARY_BOOL_OP_FN(operator>=, >=)

template<typename FT>
SingletonCache<FT> SingletonNumber<FT>::cache; // TODO define somewhere else?

template <class FT>
std::ostream& operator<<(std::ostream& out, const SingletonNumber<FT>& n) {
  out << n.underlyingValue();
  return out;
}
template <class FT>
std::istream& operator>>(std::istream& in, SingletonNumber<FT>& n) {
  in >> n.underlyingValue();
  return in;
}

namespace std {
  template <class FT>
  SingletonNumber<FT> sqrt(const SingletonNumber<FT>& x) {
    return x.sqrt();
  }
}

namespace CGAL {
  template <class FT>
  inline SingletonNumber<FT> min BOOST_PREVENT_MACRO_SUBSTITUTION(const SingletonNumber<FT>& x,const SingletonNumber<FT>& y){
    return x.min(y);
  }
  template <class FT>
  inline SingletonNumber<FT> max BOOST_PREVENT_MACRO_SUBSTITUTION(const SingletonNumber<FT>& x,const SingletonNumber<FT>& y){
    return x.max(y);
  }

  CGAL_DEFINE_COERCION_TRAITS_FOR_SELF(       SingletonNumber<CGAL::Gmpq>)
  CGAL_DEFINE_COERCION_TRAITS_FROM_TO(short , SingletonNumber<CGAL::Gmpq>)
  CGAL_DEFINE_COERCION_TRAITS_FROM_TO(int   , SingletonNumber<CGAL::Gmpq>)
  CGAL_DEFINE_COERCION_TRAITS_FROM_TO(long  , SingletonNumber<CGAL::Gmpq>)
  CGAL_DEFINE_COERCION_TRAITS_FROM_TO(float , SingletonNumber<CGAL::Gmpq>)
  CGAL_DEFINE_COERCION_TRAITS_FROM_TO(double, SingletonNumber<CGAL::Gmpq>)

  // CGAL_DEFINE_COERCION_TRAITS_FOR_SELF(       SingletonNumber<CGAL::Gmpz>)
  // CGAL_DEFINE_COERCION_TRAITS_FROM_TO(short , SingletonNumber<CGAL::Gmpz>)
  // CGAL_DEFINE_COERCION_TRAITS_FROM_TO(int   , SingletonNumber<CGAL::Gmpz>)
  // CGAL_DEFINE_COERCION_TRAITS_FROM_TO(long  , SingletonNumber<CGAL::Gmpz>)
  // CGAL_DEFINE_COERCION_TRAITS_FROM_TO(float , SingletonNumber<CGAL::Gmpz>)

  template <class FT>
  class Algebraic_structure_traits<SingletonNumber<FT>>
      : public Algebraic_structure_traits_base<SingletonNumber<FT>,Field_with_sqrt_tag>
  {
    typedef Algebraic_structure_traits<FT> Underlying_traits;
  public:
    typedef typename Underlying_traits::Is_exact                Is_exact;
    typedef typename Underlying_traits::Is_numerical_sensitive  Is_numerical_sensitive;
  };

  template <class FT>
  class Real_embeddable_traits<SingletonNumber<FT>>
      : public INTERN_RET::Real_embeddable_traits_base<SingletonNumber<FT>, CGAL::Tag_true>
  {
    typedef Real_embeddable_traits<FT> Underlying_traits;
    typedef SingletonNumber<FT> Type;
  public:
    typedef typename Underlying_traits::Is_real_embeddable  Is_real_embeddable;
    typedef typename Underlying_traits::Boolean             Boolean;
    typedef typename Underlying_traits::Comparison_result   Comparison_result;
    typedef typename Underlying_traits::Sign                Sign;
    typedef typename Underlying_traits::Is_zero             Is_zero;

    struct Is_finite: public CGAL::cpp98::unary_function<Type,Boolean>{
      inline Boolean operator()(const Type &x) const {
        return Underlying_traits::Is_finite()(x.underlyingValue());
      }
    };

    // struct Abs: public CGAL::cpp98::unary_function<Type,Type>{
    //   inline Type operator()(const Type &x) const {
    //     return Underlying_traits::Abs()(x.underlyingValue());
    //   }
    // };

    struct To_interval: public CGAL::cpp98::unary_function<Type,std::pair<double,double> >{
      inline std::pair<double,double> operator()(const Type &x) const {
        typename Underlying_traits::To_interval to_interval;

        return to_interval(x.underlyingValue());
      }
    };
  };

  template <>
  class Fraction_traits< SingletonNumber<CGAL::Gmpq> > {
  public:
      typedef SingletonNumber<CGAL::Gmpq> Type;
      typedef ::CGAL::Tag_true Is_fraction;
      typedef Gmpz Numerator_type;
      typedef Gmpz Denominator_type;
      typedef Algebraic_structure_traits< Gmpz >::Gcd Common_factor;
      class Decompose {
      public:
          typedef SingletonNumber<CGAL::Gmpq> first_argument_type;
          typedef Gmpz& second_argument_type;
          typedef Gmpz& third_argument_type;
          void operator () (const SingletonNumber<CGAL::Gmpq>& rat, Gmpz& num,Gmpz& den) {
            // TODO(ochafik): compose proper singleton num and denom, and their transition to a singleton rational.
              num = rat.underlyingValue().numerator();
              den = rat.underlyingValue().denominator();
          }
      };
      class Compose {
      public:
          typedef Gmpz first_argument_type;
          typedef Gmpz second_argument_type;
          typedef SingletonNumber<CGAL::Gmpq> result_type;
          SingletonNumber<CGAL::Gmpq> operator () (const Gmpz& num,const Gmpz& den) {
            // TODO(ochafik): compose proper singleton num and denom, and their transition to a singleton rational.
              return SingletonNumber<CGAL::Gmpq>(CGAL::Gmpq(num, den));
          }
      };
  };

  class SingletonNumberGMP_arithmetic_kernel : public internal::Arithmetic_kernel_base {
  public:
    typedef Gmpz           Integer;
    typedef SingletonNumber<CGAL::Gmpq>           Rational;
  };

  template <>
  struct Get_arithmetic_kernel<SingletonNumber<Gmpz>> {
    typedef SingletonNumberGMP_arithmetic_kernel Arithmetic_kernel;
  };
  template <>
  struct Get_arithmetic_kernel<SingletonNumber<Gmpq>> {
    typedef SingletonNumberGMP_arithmetic_kernel Arithmetic_kernel;
  };
} //namespace CGAL

namespace Eigen {
  template<class> struct NumTraits;
  template<> struct NumTraits<SingletonNumber<CGAL::Gmpq>>
  {
    typedef SingletonNumber<CGAL::Gmpq> Real;
    typedef SingletonNumber<CGAL::Gmpq> NonInteger;
    typedef SingletonNumber<CGAL::Gmpq> Nested;
    typedef SingletonNumber<CGAL::Gmpq> Literal;

    static inline Real epsilon() { return 0; }
    static inline Real dummy_precision() { return 0; }

    enum {
      IsInteger = 0,
      IsSigned = 1,
      IsComplex = 0,
      RequireInitialization = 1,
      ReadCost = 6,
      AddCost = 150,  // TODO(ochafik): reduce this
      MulCost = 100  // TODO(ochafik): reduce this
    };
  };

  namespace internal {
    template<>
      struct significant_decimals_impl<SingletonNumber<CGAL::Gmpq>>
      {
        static inline int run()
        {
          return 0;
        }
      };
  }
}
