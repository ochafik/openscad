#include "MatrixObject.h"

// #include "type.hpp"
// #ifdef __GNUG__
#include <cstdlib>
#include <memory>
#include <cxxabi.h>

#include "Value.h"

std::string demangle(const char* name) {

    int status = -4; // some arbitrary value to eliminate the compiler warning

    // enable c++11 by passing the flag -std=c++11 to g++
    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(name, NULL, NULL, &status),
        std::free
    };

    return (status==0) ? res.get() : name ;
}

template <class T>
std::string typeName() {

    return demangle(typeid(T).name());
}

using data_t = MatrixObject::data_t;

/*

template <int Rows, int Cols>
Eigen::Matrix<double, Rows, Cols> makeMatrix(int rows, int cols) {
  Eigen::Matrix<double, Rows, Cols> matrix;
  return std::move(matrix);
}
template <int Cols>
Eigen::Matrix<double, Eigen::Dynamic, Cols> makeMatrix(int rows, int cols) {
  Eigen::Matrix<double, Eigen::Dynamic, Cols> matrix;
  matrix.resize(rows, cols);
  return std::move(matrix);
}
template <int Rows>
Eigen::Matrix<double, Rows, Eigen::Dynamic> makeMatrix(int rows, int cols) {
  Eigen::Matrix<double, Rows, Eigen::Dynamic> matrix;
  matrix.resize(rows, cols);
  return std::move(matrix);
}
template <>
Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> makeMatrix(int rows, int cols) {
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> matrix;
  matrix.resize(rows, cols);
  return std::move(matrix);
}

template <int Rows, int Cols>//, bool Growable>
struct Mat {
  Eigen::Matrix<double, Rows, Cols> matrix;

  Mat(int rows, int cols) {}

  int rows() { return matrix.rows(); }
  int cols() { return matrix.cols(); }
  
  template <int Rows2, int Cols2>
  void copyTo(Mat<Rows, Cols>& out) const {
    out.block(0, 0, out.rows(), out.cols()) = 
  }
  void resize(int rows, int cols) {
    assert(Rows != Eigen::Dynamic);
    assert(Cols != Eigen::Dynamic);
  }
};

template <int Rows, int Cols>
typename std::enable_if_t<Rows == Eigen::Dynamic || Cols == Eigen::Dynamic, void>
resize_dynamic(Eigen::Matrix<double, Rows, Cols>& matrix, int rows, int cols) {
  assert(Rows == Eigen::Dynamic || rows == Rows && rows == matrix.rows());
  assert(Cols == Eigen::Dynamic || cols == Cols && cols == matrix.cols());
  matrix.resize(rows, cols);
}

template <int Cols>
struct Mat<Eigen::Dynamic, Cols> {
  int rows_ = 0;
};

template <int Cols>
void Mat<Eigen::Dynamic, Cols>::rows() const {
  return rows_;
}

template <int Cols>
void Mat<Eigen::Dynamic, Cols>::resize(int rows, int cols) {
  resize_dynamic(matrix, rows, cols);
}

template <int Rows>
void Mat<Rows, Eigen::Dynamic>::resize(int rows, int cols) {
  resize_dynamic(matrix, rows, cols);
}
*/

int grow(int size) {
  return (size * 15) / 10 + 16;
}

struct reserve_visitor : public boost::static_visitor<bool> 
{
  size_t rows;
  reserve_visitor(size_t rows) : rows(rows) {}

  template <class A>
  bool operator()(A& a) const {
    std::cerr << "reserve not doing anything " << typeName<A>().c_str() << "\n";
    return false;
  }

  template <int Cols>
  bool operator()(std::shared_ptr<std::pair<Eigen::Matrix<double, Eigen::Dynamic, Cols>, size_t>>& out) const {
    auto &mat = out->first;
    if (mat.rows() < rows) {
      mat.conservativeResize(rows, Eigen::NoChange);
    }
    return true;
  }
};

bool MatrixObject::reserve(size_t rows) {
  std::cerr << "reserveDoubles(" << rows << ")\n";
  return boost::apply_visitor(reserve_visitor(rows), data);
}

void MatrixObject::reserveDoubles(size_t rows) {
  std::cerr << "reserveDoubles(" << rows << ")\n";
  // auto res = std::make_shared<std::pair<Eigen::Matrix<double, Eigen::Dynamic, 1>, size_t>>();
  // auto &vec = res->first;
  // vec.resize(rows, cols);
  // data = res;
  reserveDoubleArrays(rows, 1);
  // auto res = std::make_shared<std::pair<Eigen::VectorXd, size_t>>();
  // auto &vec = ret->first;
  // vec.resize(rows);
  // data = res;
}

void MatrixObject::reserveDoubleArrays(size_t rows, size_t cols) {
  std::cerr << "reserveDoubleArrays(" << rows << ", " << cols << ")\n";
  auto res = std::make_shared<std::pair<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>, size_t>>();
  auto &vec = res->first;
  vec.resize(rows, cols);
  data = res;
}

template <int InputRows, int InputCols, int OutputRows, int OutputCols>
bool rigidCopy(
    const Eigen::Matrix<double, InputRows, InputCols>& in,
    Eigen::Matrix<double, OutputRows, OutputCols>& out,
    size_t rows, size_t cols) {
  // auto rows = in.rows();
  // auto cols = in.cols();
  assert(rows == OutputRows);
  assert(cols == OutputCols);
  // out.resize(rows, cols);
  out = in.block(0, 0, rows, cols);
  return true;
}

template <int InputRows, int InputCols, /* Eigen::Dynamic, */ int OutputCols>
bool rigidCopy(
    const Eigen::Matrix<double, InputRows, InputCols>& in,
    Eigen::Matrix<double, Eigen::Dynamic, OutputCols>& out,
    size_t rows, size_t cols) {
  // auto rows = in.rows();
  // auto cols = in.cols();
  assert(OutputCols == Eigen::Dynamic || cols == OutputCols);
  out.resize(rows, cols);
  out = in.block(0, 0, rows, cols);
  return true;
}

template <int InputRows, int InputCols, int OutputRows /* , Eigen::Dynamic */>
bool rigidCopy(
    const Eigen::Matrix<double, InputRows, InputCols>& in,
    Eigen::Matrix<double, OutputRows, Eigen::Dynamic>& out,
    size_t rows, size_t cols) {
  // auto rows = in.rows();
  // auto cols = in.cols();
  assert(OutputRows == Eigen::Dynamic || rows == OutputRows);
  out.resize(rows, cols);
  out = in.block(0, 0, rows, cols);
  return true;
}

template <int InputRows, int InputCols /* , Eigen::Dynamic, Eigen::Dynamic */>
bool rigidCopy(
    const Eigen::Matrix<double, InputRows, InputCols>& in,
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>& out,
    size_t rows, size_t cols) {
  // auto rows = in.rows();
  // auto cols = in.cols();
  out.resize(rows, cols);
  out = in.block(0, 0, rows, cols);
  return true;
}

struct seal_visitor : public boost::static_visitor<data_t> 
{
  template <class A>
  data_t operator()(A& a) const {
    std::cerr << "seal not doing anything " << typeName<A>().c_str() << "\n";
    // Pass unchanged
    return a;
  }

  // Turn dynamic vector into exact sized one.
  template <int Cols>//, std::enable_if_t<Cols != Eigen::Dynamic, int> = 0>
  data_t operator()(std::shared_ptr<std::pair<Eigen::Matrix<double, Eigen::Dynamic, Cols>, size_t>>& out) const {
    // assert(Cols != Eigen::Dynamic)

    auto &mat = out->first;
    auto &matRows = out->second;
    auto matCols = mat.cols();
    
    #define RETURN_RIGID_COPY(rows, cols) { \
      auto ret = std::make_shared<Eigen::Matrix<double, rows, cols>>(); \
      if (!rigidCopy(mat, *ret, matRows, matCols)) { \
        return data_t(); \
      } \
      return ret; \
    }

    switch (matRows) {
      case 2:
        switch (matCols) {
          case 2: RETURN_RIGID_COPY(2, 2);
          default: break;
        }
        break;
      case 3:
        switch (matCols) {
          case 3: RETURN_RIGID_COPY(3, 3);
          default: break;
        }
        break;
      case 4:
        switch (matCols) {
          case 3: RETURN_RIGID_COPY(4, 3);
          case 4: RETURN_RIGID_COPY(4, 4);
          default: break;
        }
        break;
      default:
        switch (matCols) {
          case 3: RETURN_RIGID_COPY(Eigen::Dynamic, 3);
          case 4: RETURN_RIGID_COPY(Eigen::Dynamic, 4);
          default: break;
        }
        break;
    }
    RETURN_RIGID_COPY(Eigen::Dynamic, Cols);
  }
};

struct type_to_string_visitor : public boost::static_visitor<std::string> {
  template <class A>
  std::string instanceTypeName(const A& a) const {
    return typeName<A>();
  }
  template <class A>
  std::string operator()(const A& a) const {
    return typeName<A>();
  }
  // Turn dynamic vector into exact sized one.
  template <int Rows, int Cols>
  std::string operator()(const std::shared_ptr<Eigen::Matrix<double, Rows, Cols>>& in) const {
    std::ostringstream out;
    out << instanceTypeName(in).c_str() << "(" << in->rows() << ", " << in->cols() << ")";
    return out.str();
  }
  // Turn dynamic vector into exact sized one.
  template <int Rows, int Cols>
  std::string operator()(const std::shared_ptr<std::pair<Eigen::Matrix<double, Rows, Cols>, size_t>>& in) const {
    std::ostringstream out;
    out << instanceTypeName(in) << "(" << in->second << " (capacity " << in->first.rows() << "), " << in->first.cols() << ")";
    return out.str();
  }
};

std::string type_to_string(const data_t &data) {
  return boost::apply_visitor(type_to_string_visitor(), data);
}

void MatrixObject::seal() const {
  if (!sealed) {
    auto before = type_to_string(data);
    data = boost::apply_visitor(seal_visitor(), data);
    auto after = type_to_string(data);
    std::cerr << "Sealed " << before.c_str() << " -> " << after.c_str() << "\n";
    sealed = true;
  }
}

struct foreach_visitor : public boost::static_visitor<void> 
{
  const std::function<void(Value&&)>& fn;
  foreach_visitor(const std::function<void(Value&&)>& fn) : fn(fn) {}

  void operator()(const boost::blank&) const {}

  template <int Rows, int Cols>
  void foreach(const Eigen::Matrix<double, Rows, Cols>& in, size_t rows) const {
    auto cols = in.cols();
    if (cols == 1) {
      for (size_t i = 0; i < rows; i++) {
        fn(in(i, 0));
      }
    } else {
      for (size_t i = 0; i < rows; i++) {
        VectorType row(nullptr);
        row.reserve(cols);

        for (size_t j = 0; j < cols; j++) {
          row.emplace_back(in(i, j));
        }
        fn(std::move(row));
      }
    }
  }
  // Turn dynamic vector into exact sized one.
  template <int Rows, int Cols>
  void operator()(const std::shared_ptr<Eigen::Matrix<double, Rows, Cols>>& in) const {
    foreach(*in, in->rows());
  }

  // Turn dynamic vector into exact sized one.
  template <int Rows, int Cols>
  void operator()(const std::shared_ptr<std::pair<Eigen::Matrix<double, Rows, Cols>, size_t>>& in) const {
    foreach(in->first, in->second);
  }
};

void MatrixObject::forEachRow(const std::function<void(Value&&)>& fn) const {
  boost::apply_visitor(foreach_visitor(fn), data);
}

struct rows_visitor : public boost::static_visitor<size_t> 
{
  size_t operator()(const boost::blank&) const {
    return 0;
  }

  template <int Rows, int Cols>
  size_t operator()(const std::shared_ptr<Eigen::Matrix<double, Rows, Cols>>& in) const {
    return in->rows();
  }

  // Turn dynamic vector into exact sized one.
  template <int Rows, int Cols>
  size_t operator()(const std::shared_ptr<std::pair<Eigen::Matrix<double, Rows, Cols>, size_t>>& in) const {
    return in->second;
  }
};

size_t MatrixObject::rows() const {
  return boost::apply_visitor(rows_visitor() , data);
}

struct unseal_visitor : public boost::static_visitor<data_t> 
{
  template <class A>
  data_t operator()(A& a) const {
    std::cerr << "seal not doing anything " << typeName<A>().c_str() << "\n";
    // Pass unchanged
    return a;
  }

  // Turn dynamic vector into exact sized one.
  template <int Rows, int Cols, typename std::enable_if<Rows != Eigen::Dynamic, int> = 0>
  data_t operator()(std::shared_ptr<Eigen::Matrix<double, Rows, Cols>>& in) const {
    assert(Rows != Eigen::Dynamic);

    auto ret = std::make_shared<std::pair<Eigen::Matrix<double, Eigen::Dynamic, Cols>, size_t>>();
    auto &vec = ret->first;
    auto &size = ret->second;
    size = in->rows();
    vec.resize(grow(size), in->cols());
    vec = in->block(0, 0, size, in->cols());
    return ret;
  }
};

/*
  pair<VectorXd, size> -> pair<VectorXd, size + 1>
  TODO: grow sealed vectors (e.g. VectorNd -> pair<VectorXd, N + 1>)
  
*/
struct push_back_visitor : public boost::static_visitor<data_t> 
{
  template <class A, class B>
  data_t operator()(A& out, const B& item) const {
    std::cerr << "push_back not supported " << typeName<A>().c_str() << ".push_back(" << typeName<B>().c_str() << "). was seal() called?\n";
    return data_t();
  }

  template <class A>
  data_t operator()(A& out, double) const {
    std::cerr << "push_back not supported " << typeName<A>().c_str() << ".push_back(double). was seal() called?\n";
    return data_t();
  }

  // This needs Matrix<double, Dynamic, *> in the variant

  // // Convert fixed size lhs to growable and retry.
  // template <int Rows, int Cols, class B>
  // data_t operator()(std::shared_ptr<Eigen::Matrix<double, Rows, Cols>>& fixedLhs, const B& rhs) const {
  //   data_t fixedLhsData = fixedLhs;
  //   data_t dynamicLhsData = boost::apply_visitor(unseal_visitor(), fixedLhsData);
  //   if (dynamicLhsData.which() != 0) {
  //     data_t rhsData = rhs;
  //     auto result = boost::apply_visitor(push_back_visitor(), dynamicLhsData, rhsData);
  //     return result;
  //   }
  //   return data_t();
  // }

  // pair<MatrixXNd, rows == 1>.append(double)
  // Grow dynamic MatrixXd (col=1 statically like VectorXd, or dynamically)
  // (w/ explicit size that can be less than the vector's capacity)
  template <int Rows, int Cols, typename std::enable_if_t<(Cols == Eigen::Dynamic) || (Cols == 1), int> = 0>
  data_t operator()(std::shared_ptr<std::pair<Eigen::Matrix<double, Rows, Cols>, size_t>>& out, double value) const {
    auto &vec = out->first;
    auto &size = out->second;
    if (vec.cols() != 1) {
      return data_t();
    }

    if (size >= vec.size()) {
      vec.conservativeResize(grow(size), Eigen::NoChange);
    }
    vec(size++, 0) = value;

    return out;
  }

  // pair<MatrixXNd, rows>.append(pair<VectorXd, vecRows>)
  template <int MCols, int VCols, typename std::enable_if_t<VCols == Eigen::Dynamic || VCols == 1, int> = 0>
  data_t operator()(std::shared_ptr<std::pair<Eigen::Matrix<double, Eigen::Dynamic, MCols>, size_t>>& list,
                    const std::shared_ptr<std::pair<Eigen::Matrix<double, Eigen::Dynamic, VCols>, size_t>>& item) const {
    auto &mat = list->first;
    auto &matRows = list->second;
    auto &vec = item->first;
    auto &vecRows = item->second;
    if (vec.cols() != 1) {
      return data_t();
    }
    if (vecRows != mat.cols()) {
      return data_t();
    }
    if (matRows >= mat.rows()) {
      mat.conservativeResize(grow(matRows), Eigen::NoChange);
    }
    mat.block(matRows++, 0, 1, vecRows) = vec.block(0, 0, vecRows, 1).adjoint();
    return list;
  }   

  // pair<MatrixXNd, rows>.append(VectorNd)
  // template <int Cols>
  // data_t operator()(std::shared_ptr<std::pair<Eigen::Matrix<double, Eigen::Dynamic, Cols>, size_t>>& out,
  //                   const std::shared_ptr<Eigen::Matrix<double, Cols, 1>>& item) const {
  template <int MCols, int VRows, int VCols>//, typename std::enable_if_t<VCols == Eigen::Dynamic || VCols == 1, int> = 0>
  data_t operator()(std::shared_ptr<std::pair<Eigen::Matrix<double, Eigen::Dynamic, MCols>, size_t>>& list,
                    const std::shared_ptr<Eigen::Matrix<double, VRows, VCols>>& item) const {
    auto &mat = list->first;
    auto &matRows = list->second;

    if (item->cols() != 1) {
      return data_t();
    }
    if (mat.cols() != item->rows()) {
      return data_t();
    }
    if (matRows >= mat.rows()) {
      mat.conservativeResize(grow(matRows), Eigen::NoChange);
    }
    mat.block(matRows++, 0, 1, mat.cols()) = item->adjoint();
    return list;
  }
};

template <class Visitor>
boost::optional<MatrixObject> apply_visitor2(const Visitor& visitor, data_t &a, const data_t& b) {
  auto result = boost::apply_visitor(visitor, a, b);
  if (result.which() != 0) {
    return MatrixObject(std::move(result));
  }
  return boost::none;
}

bool MatrixObject::push_back(const MatrixObject& element) {
  assert(!sealed);
  auto result = boost::apply_visitor(push_back_visitor(), data, element.data);
  if (result.which() != 0) {
    data = result;
    return true;
  }
  return false;
}

bool MatrixObject::push_back(double value) {
  assert(!sealed);
  auto result = boost::apply_visitor(std::bind(push_back_visitor(), std::placeholders::_1, value), data);
  if (result.which() != 0) {
    data = result;
    return true;
  }
  std::cerr << type_to_string(data).c_str() << ".push_back(double) failed\n";
  return false;
}

template <class Op>
struct cwise_op_visitor : public boost::static_visitor<data_t>
{
  const Op &op;
  cwise_op_visitor(const Op &op) : op(op) {}

  template <class A, class B>
  data_t operator()(const A& op1, const B& op2) const {
    std::cerr << "cwise op not supported " << typeName<A>().c_str() << " " << typeName<Op>().c_str() << " " << typeName<B>().c_str() << "). was seal() called?\n";
    return data_t();
  }
  
  template <size_t N, size_t M>
  data_t operator()(
      const std::shared_ptr<Eigen::Matrix<double, N, M>>& op1,
      const std::shared_ptr<Eigen::Matrix<double, N, M>>& op2) const {
    if (op1->rows() != op2->rows() ||
        op1->cols() != op2->cols()) {
      return data_t();
    }
    return std::make_shared<Eigen::Matrix<double, N, M>>(op(op1, op2));
  }
};

struct plus_op {
  template <class A, class B>
  data_t operator()(const A&a, const B&b) {
    return a + b;
  }
};

boost::optional<MatrixObject> MatrixObject::operator+(const MatrixObject& element) {
  seal();
  element.seal();
  return apply_visitor2(cwise_op_visitor<plus_op>(plus_op()), data, element.data);
}

struct minus_op {
  template <class A, class B>
  data_t operator()(const A&a, const B&b) {
    return a - b;
  }
};

boost::optional<MatrixObject> MatrixObject::operator-(const MatrixObject& element) {
  seal();
  element.seal();
  return apply_visitor2(cwise_op_visitor<minus_op>(minus_op()), data, element.data);
}

struct divide_op {
  template <class A, class B>
  data_t operator()(const A&a, const B&b) {
    return a / b;
  }
};

boost::optional<MatrixObject> MatrixObject::operator/(const MatrixObject& element) {
  seal();
  element.seal();
  return apply_visitor2(cwise_op_visitor<divide_op>(divide_op()), data, element.data);
}

struct product_visitor : public boost::static_visitor<data_t>
{
  template <class A, class B>
  data_t operator()(const A& out, const B& item) const {
    std::cerr << "product op not supported " << typeName<A>().c_str() << " * " << typeName<B>().c_str() << "). was seal() called?\n";
    return data_t();
  }
  
  // Matrix * Matrix with statically-sized output.
  template <int Rows1, int Cols1, int Rows2, int Cols2,
    typename std::enable_if_t<
      Cols1 != 1 && Rows2 != 1 &&
      // Compatible dimensions only (w/ dynamic dimensions checked at runtime below).
      ((Cols1 == Rows2) || (Cols1 == Eigen::Dynamic) || (Rows2 == Eigen::Dynamic)) &&
      // Only output static dimensions for specific matrix sizes.
      // Otherwise we're covered by the dynamic output overload.
      ((Rows1 == 2 && Cols2 == 2) ||
       (Rows1 == 3 && Cols2 == 3) ||
       (Rows1 == 4 && Cols2 == 4) ||
       (Rows1 == Eigen::Dynamic) //||
      //  (Cols2 == Eigen::Dynamic)
      ),
      int> = 0>
  data_t operator()(
      std::shared_ptr<Eigen::Matrix<double, Rows1, Cols1>> op1,
      std::shared_ptr<Eigen::Matrix<double, Rows2, Cols2>> op2) const {
  // template <int N, int K, int M>
  // data_t operator()(
  //     const std::shared_ptr<Eigen::Matrix<double, N, K>>& op1,
  //     const std::shared_ptr<Eigen::Matrix<double, K, M>>& op2) const {
    
    std::cerr << "product " << type_to_string(op1) << " * " << type_to_string(op2) << "\n";
    if (op1->cols() != op2->rows()) {
      return data_t();
    }
    return std::make_shared<Eigen::Matrix<double, Rows1, Cols2>>(std::move(*op1 * *op2));
  }
  
  // // Matrix * Matrix
  // template <int Rows1, int Cols1, int Rows2, int Cols2,
  //   typename std::enable_if_t<
  //     Cols1 != 1 && Rows2 != 1 &&
  //     ((Cols1 == Rows2) || (Cols1 == Eigen::Dynamic) || (Rows2 == Eigen::Dynamic)),
  //     int> = 0>
  // data_t operator()(
  //     std::shared_ptr<Eigen::Matrix<double, Rows1, Cols1>> op1,
  //     std::shared_ptr<Eigen::Matrix<double, Rows2, Cols2>> op2) const {
  // // template <int N, int K, int M>
  // // data_t operator()(
  // //     const std::shared_ptr<Eigen::Matrix<double, N, K>>& op1,
  // //     const std::shared_ptr<Eigen::Matrix<double, K, M>>& op2) const {
  //   if (op1->cols() != op2->rows()) {
  //     return data_t();
  //   }
  //   return std::make_shared<Eigen::Matrix<double, Rows1, Cols2>>(std::move(*op1 * *op2));
  // }
};

boost::optional<MatrixObject> MatrixObject::operator*(const MatrixObject& element) {
  seal();
  element.seal();
  return apply_visitor2(product_visitor(), data, element.data);
}
