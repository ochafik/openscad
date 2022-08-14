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

using data_t = Value::MatrixObject::data_t;

MatrixObject::data_t buildMatrixData(size_t rows, size_t cols, bool dynamic) {
  if (!dynamic) {
    switch (rows) {
      case 2:
        switch (cols) {
          case 1:
            return std::make_shared<Eigen::Matrix<double, 2, 1>>();
          case 2:
            return std::make_shared<Eigen::Matrix<double, 2, 2>>();
          // case 3:
          //   return std::make_shared<Eigen::Matrix<double, 2, 3>>();
          default:
            break;
        }
        break;
      case 3:
        switch (cols) {
          case 1:
            return std::make_shared<Eigen::Matrix<double, 3, 1>>();
          // case 2:
          //   return std::make_shared<Eigen::Matrix<double, 3, 2>>();
          case 3:
            return std::make_shared<Eigen::Matrix<double, 3, 3>>();
          // case 4:
          //   return std::make_shared<Eigen::Matrix<double, 3, 4>>();
          default:
            break;
        }
        break;
      case 4:
        switch (cols) {
          case 1:
            return std::make_shared<Eigen::Matrix<double, 4, 1>>();
          case 3:
            return std::make_shared<Eigen::Matrix<double, 4, 3>>();
          case 4:
            return std::make_shared<Eigen::Matrix<double, 4, 4>>();
          default:
            break;
        }
        break;
      default:
        break;
    }
  }

#define RETURN_SIZED_MATRIX(type) \
  { \
    auto matrix = std::make_shared<type>();  \
    matrix->resize(rows, cols); \
    return matrix; \
  }

  switch (cols) {
    case 1:
      RETURN_SIZED_MATRIX(Eigen::VectorXd);
    case 2:
      RETURN_SIZED_MATRIX(Eigen::MatrixX2d);
    case 3:
      RETURN_SIZED_MATRIX(Eigen::MatrixX3d);
    case 4:
      RETURN_SIZED_MATRIX(Eigen::MatrixX4d);
    default:
      RETURN_SIZED_MATRIX(Eigen::MatrixXd);
  }
}

MatrixObject::MatrixObject(size_t rows, size_t cols, bool dynamic, bool is_vector) : is_vector(is_vector) {
  data = buildMatrixData(rows, cols, dynamic);
  assert(!is_vector || this->cols() == 1);
}

MatrixObject::MatrixObject(data_t &&data, bool is_vector) : data(std::move(data)), is_vector(is_vector) {
  assert(!is_vector || cols() == 1);
}

struct resize_visitor : public boost::static_visitor<bool> 
{
  size_t rows;
  resize_visitor(size_t rows) : rows(rows) {}

  template <class A>
  bool operator()(A& a) const {
    std::cerr << "reserve not doing anything " << typeName<A>().c_str() << "\n";
    return false;
  }

  template <int Cols>
  bool operator()(std::shared_ptr<Eigen::Matrix<double, Eigen::Dynamic, Cols>>& mat) const {
    if (mat->rows() != rows) {
      mat->conservativeResize(rows, Eigen::NoChange);
    }
    return true;
  }
};

bool Value::MatrixObject::resize(size_t rows) {
  std::cerr << "resize(" << rows << ")\n";
  return boost::apply_visitor(resize_visitor(rows), data);
}

template <int InputRows, int InputCols, int OutputRows, int OutputCols>
bool rigidCopy(
    const Eigen::Matrix<double, InputRows, InputCols>& in,
    Eigen::Matrix<double, OutputRows, OutputCols>& out,
    size_t rows, size_t cols) {
  assert(rows == OutputRows);
  assert(cols == OutputCols);
  out = in.block(0, 0, rows, cols);
  return true;
}

template <int InputRows, int InputCols, /* Eigen::Dynamic, */ int OutputCols>
bool rigidCopy(
    const Eigen::Matrix<double, InputRows, InputCols>& in,
    Eigen::Matrix<double, Eigen::Dynamic, OutputCols>& out,
    size_t rows, size_t cols) {
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
  out.resize(rows, cols);
  out = in.block(0, 0, rows, cols);
  return true;
}

struct seal_visitor : public boost::static_visitor<data_t> 
{
  size_t sealed_rows;
  seal_visitor(size_t sealed_rows) : sealed_rows(sealed_rows) {}

  template <class A>
  data_t operator()(A& a) const {
    // std::cerr << "seal not doing anything " << typeName<A>().c_str() << "\n";
    // Pass unchanged
    return a;
  }

  // Turn dynamic vector into exact sized one.
  template <int Cols>//, std::enable_if_t<Cols != Eigen::Dynamic, int> = 0>
  data_t operator()(const std::shared_ptr<Eigen::Matrix<double, Eigen::Dynamic, Cols>>& mat) const {
    auto matCols = mat->cols();
    
    #define RETURN_RIGID_COPY(rows, cols) { \
      auto ret = std::make_shared<Eigen::Matrix<double, rows, cols>>(); \
      if (!rigidCopy(*mat, *ret, sealed_rows, matCols)) { \
        return data_t(); \
      } \
      return ret; \
    }

    switch (sealed_rows) {
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
          case 2: RETURN_RIGID_COPY(Eigen::Dynamic, 2);
          case 3: RETURN_RIGID_COPY(Eigen::Dynamic, 3);
          case 4: RETURN_RIGID_COPY(Eigen::Dynamic, 4);
          default: break;
        }
        break;
    }

    if (sealed_rows != mat->rows()) {
      mat->conservativeResize(sealed_rows, Eigen::NoChange);
    }
    return mat;
    // RETURN_RIGID_COPY(Eigen::Dynamic, Cols);
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
  template <int Rows, int Cols>
  std::string operator()(const std::shared_ptr<Eigen::Matrix<double, Rows, Cols>>& in) const {
    std::ostringstream out;
    out << instanceTypeName(in).c_str() << "(" << in->rows() << ", " << in->cols() << ")";
    return out.str();
  }
};

std::string type_to_string(const data_t &data) {
  return boost::apply_visitor(type_to_string_visitor(), data);
}

void Value::MatrixObject::seal(size_t sealed_rows) {
  data = boost::apply_visitor(seal_visitor(sealed_rows), data);
}

struct to_vector_visitor : public boost::static_visitor<void> 
{
  VectorType &out;
  bool is_vector;
  size_t size;
  to_vector_visitor(VectorType &out, bool is_vector, size_t size) : out(out), is_vector(is_vector), size(size) {}

  template <int Rows, int Cols>//, typename std::enable_if<Cols != 1, int> = 0>
  void operator()(const std::shared_ptr<Eigen::Matrix<double, Rows, Cols>>& in) const {
    auto rows = in->rows();
    auto cols = in->cols();
    assert(size <= rows);

    out.reserve(size);

    if (is_vector) {
      assert(cols == 1);
      for (size_t i = 0; i < size; i++) {
        out.emplace_back((*in)(i, 0));
      }
    } else {
      for (size_t i = 0; i < size; i++) {
        VectorType row(nullptr);
        row.reserve(cols);

        for (size_t j = 0; j < cols; j++) {
          row.emplace_back((*in)(i, j));
        }
        out.emplace_back(std::move(row));
      }
    }
  }
};

VectorType Value::MatrixObject::toVector(EvaluationSession *session, int size) const {
  VectorType out(session);
  boost::apply_visitor(to_vector_visitor(out, is_vector, size < 0 ? rows() : size), data);
  return std::move(out);
}

struct get_visitor : public boost::static_visitor<double> 
{
  size_t row;
  size_t col;
  get_visitor(size_t row, size_t col) : row(row), col(col) {}
  
  template <class T>
  double operator()(const std::shared_ptr<T>& in) const {
    return (*in)(row, col);
  }
};

double Value::MatrixObject::operator()(size_t row, size_t col) const {
  return boost::apply_visitor(get_visitor(row, col), data);
}

struct bracket_visitor : public boost::static_visitor<Value> 
{
  bool is_vector;
  size_t index;
  bracket_visitor(bool is_vector, size_t index) : is_vector(is_vector), index(index) {}

  template <class M>
  Value operator()(std::shared_ptr<M> mat) const {
    if (index >= mat->rows()) {
      return std::move(Value(UndefType()));
    }
    if (mat->cols() == 1) {
      return (*mat)(index, 0);
    } else {
#define RETURN_ROW_AS_COL_VECTOR(type) \
        { \
          auto v = std::make_shared<type>(); \
          v->noalias() = mat->block(index, 0, 1, mat->cols()).transpose(); \
          return MatrixType(MatrixObject(std::move(v), /* is_vector= */ true)); \
        }
        
      switch (mat->cols()) {
        case 2:
          RETURN_ROW_AS_COL_VECTOR(Eigen::Vector2d);
        case 3:
          RETURN_ROW_AS_COL_VECTOR(Eigen::Vector3d);
        case 4:
          RETURN_ROW_AS_COL_VECTOR(Eigen::Vector4d);
        default:
          {
            auto v = std::make_shared<Eigen::MatrixXd>(mat->cols(), 1);
            *v = mat->block(index, 0, 1, mat->cols()).transpose();
            return MatrixType(MatrixObject(std::move(v), /* is_vector= */ true));
          }
      }
    }
  }
};

Value MatrixObject::operator[](size_t index) const
{
  return std::move(boost::apply_visitor(bracket_visitor(is_vector, index), data));
}

struct multibracket_visitor : public boost::static_visitor<Value>
{
  const std::vector<size_t> &indices;
  multibracket_visitor(const std::vector<size_t> &indices) : indices(indices) {}
  
  template <class A>
  Value operator()(const std::shared_ptr<A>& mat) const {
    assert(mat->cols() == 1);

    if (indices.size() == 1) {
      return (*mat)(indices[0], 0);
    }

    VectorBuilder ret;
    ret.reserve(indices.size());
    for (auto index : indices) {
      ret.emplace_back((*mat)(index, 0));
    }
    return std::move(ret.build());
  }
};

Value MatrixObject::operator[](const std::vector<size_t> &indices) const
{
  return std::move(boost::apply_visitor(multibracket_visitor(indices), data));
}

struct set_row_visitor : public boost::static_visitor<bool> 
{
  size_t index;
  bool element_is_vector;
  set_row_visitor(size_t index, bool element_is_vector) : index(index), element_is_vector(element_is_vector) {}
  
  template <class A, class B>
  bool operator()(const A& out, const B& item) const {
    std::cerr << "set matrix row not supported " << typeName<A>().c_str() << " set " << typeName<B>().c_str() << "\n";
    return false;
  }

  template <int Rows1, int Cols1, int Rows2, int Cols2,
    std::enable_if_t<
      (Cols1 == Eigen::Dynamic || Cols1 == Rows2)
      && (Cols2 == Eigen::Dynamic || Cols2 == 1), int> = 0>
  bool operator()(const std::shared_ptr<Eigen::Matrix<double, Rows1, Cols1>> &mat,
                  const std::shared_ptr<Eigen::Matrix<double, Rows2, Cols2>> &element) const {
    if (index >= mat->rows()) {
      return false;
    }
    if (element_is_vector) {
      // Element is a column vector
      if (element->cols() != 1) {
        return false;
      }
      if (mat->cols() != element->rows()) {
        return false;
      }
      mat->block(index, 0, 1, element->rows()) = element->transpose();
    } else {
      if (element->rows() != 1) {
        return false;
      }
      if (mat->cols() != element->cols()) {
        return false;
      }
      mat->block(index, 0, 1, element->cols()) = *element;
    }
    return true;
  }
};

bool Value::MatrixObject::set_row(size_t index, const MatrixObject& element) {
  return boost::apply_visitor(set_row_visitor(index, element.is_vector), data, element.data);
}

struct set_range_visitor : public boost::static_visitor<bool> 
{
  size_t index;
  set_range_visitor(size_t index) : index(index) {}
  
  template <class A, class B>
  bool operator()(const A& out, const B& item) const {
    std::cerr << "set matrix range not supported " << typeName<A>().c_str() << " set " << typeName<B>().c_str();
    return false;
  }

  template <int Rows1, int Cols1, int Rows2, int Cols2,
    std::enable_if_t<
      (Cols1 == Eigen::Dynamic || Cols1 == 1)
      && (Cols2 == Eigen::Dynamic || Cols2 == 1), int> = 0>
  bool operator()(const std::shared_ptr<Eigen::Matrix<double, Rows1, Cols1>> &mat,
                  const std::shared_ptr<Eigen::Matrix<double, Rows2, Cols2>> &element) const {
    if (!(index < mat->rows() && (index + element->rows() <= mat->rows()))) {
      return false;
    }
    // Mat and Element are columns
    if (mat->cols() != 1) {
      return false;
    }
    if (element->cols() != 1) {
      return false;
    }
    mat->block(index, 0, element->rows(), 1) = *element;
    return true;
  }
};

bool Value::MatrixObject::set_range(size_t index, const MatrixObject& element) {
  return boost::apply_visitor(set_range_visitor(index), data, element.data);
}

struct set_scalar_visitor : public boost::static_visitor<bool> 
{
  size_t index;
  double value;
  set_scalar_visitor(size_t index, double value) : index(index), value(value) {}
  
  template <class A>
  bool operator()(const A& out) const {
    std::cerr << "set scalar not supported " << typeName<A>().c_str() ;
    return false;
  }

  template <class M>
  bool operator()(std::shared_ptr<M> mat) const {
    if (mat->cols() != 1) {
      return false;
    }
    if (index >= mat->rows()) {
      return false;
    }
    (*mat)(index) = value;
    return true;
  }
};

bool Value::MatrixObject::set(size_t index, double value) {
  return boost::apply_visitor(set_scalar_visitor(index, value), data);
}

template <class T> struct matrix_traits {};
template <int N, int M> struct matrix_traits<Eigen::Matrix<double, N, M>> {
  typedef std::integral_constant<int, N> rows;
  typedef std::integral_constant<int, M> cols;
  typedef Eigen::Matrix<double, N, M> matrix;
};

template <class Op>
struct cwise_binary_op_visitor : public boost::static_visitor<boost::optional<MatrixObject>>
{
  const Op &op;
  bool is_vector;
  cwise_binary_op_visitor(const Op &op, bool is_vector) : op(op), is_vector(is_vector) {}

  template <class A, class B>
  boost::optional<MatrixObject> operator()(const A& op1, const B& op2) const {
    std::cerr << "cwise op not supported " << typeName<A>().c_str() << " " << typeName<Op>().c_str() << " " << typeName<B>().c_str();
    return boost::none;
  }
  
  template <class A, class B,
    typename std::enable_if_t<
      (matrix_traits<A>::rows::value == matrix_traits<B>::rows::value || matrix_traits<A>::rows::value == Eigen::Dynamic || matrix_traits<B>::rows::value == Eigen::Dynamic) &&
      (matrix_traits<A>::cols::value == matrix_traits<B>::cols::value || matrix_traits<A>::cols::value == Eigen::Dynamic || matrix_traits<B>::cols::value == Eigen::Dynamic), int> = 0>
  boost::optional<MatrixObject> operator()(const std::shared_ptr<A>& op1, const std::shared_ptr<B>& op2) const {
    if (op1->rows() != op2->rows() ||
        op1->cols() != op2->cols()) {
      return boost::none;
    }

    // TODO: lazy matrix for types that make sense.
    auto m = std::make_shared<typename matrix_traits<A>::matrix>(op1->rows(), op1->cols());
    op(*op1, *op2, *m);
    // TODO(ochafik): resize when dynamic
    
    return MatrixObject(std::move(m), is_vector);
  }
};

struct plus_op {
  template <class A, class B, class C>
  void operator()(const A&a, const B&b, C &out) const {
    out.noalias() = a + b;
  }
};

boost::optional<MatrixObject> Value::MatrixObject::operator+(const MatrixObject& other) const {
  return boost::apply_visitor(cwise_binary_op_visitor<plus_op>(plus_op(), is_vector || other.is_vector), data, other.data);
}

struct minus_op {
  template <class A, class B, class C>
  void operator()(const A&a, const B&b, C&out) const {
    out.noalias() = a - b;
  }
};

boost::optional<MatrixObject> Value::MatrixObject::operator-(const MatrixObject& other) const {
  return boost::apply_visitor(cwise_binary_op_visitor<minus_op>(minus_op(), is_vector || other.is_vector), data, other.data);
}

struct unary_minus_visitor : public boost::static_visitor<MatrixObject> 
{
  bool is_vector;
  unary_minus_visitor(bool is_vector) : is_vector(is_vector) {}
  
  template <class A>
  MatrixObject operator()(std::shared_ptr<A> mat) const {
    auto m = std::make_shared<typename matrix_traits<A>::matrix>(mat->rows(), mat->cols());
    *m = -*mat;
    return std::move(MatrixObject(std::move(m), is_vector));
  }
};

MatrixObject Value::MatrixObject::operator-() const {
  return std::move(boost::apply_visitor(unary_minus_visitor(is_vector), data));
}

// struct cross_visitor : public boost::static_visitor<boost::optional<MatrixObject>> 
// {
//   template <class A, class B>
//   boost::optional<MatrixObject> operator()(const A& op1, const B& op2) const {
//     std::cerr << "cross op not supported " << typeName<A>().c_str() << " x " << typeName<B>().c_str();
//     return boost::none;
//   }

//   template <int Rows1, int Cols1, int Rows2, int Cols2,
//     std::enable_if_t<
//       Rows1 == 3 && Rows2 == 3 &&
//       (Cols1 == Eigen::Dynamic || Cols1 == 1) &&
//       (Cols2 == Eigen::Dynamic || Cols2 == 1), int> = 0>
//   boost::optional<MatrixObject> operator()(const std::shared_ptr<Eigen::Matrix<double, Rows1, Cols1>> &op1,
//                   const std::shared_ptr<Eigen::Matrix<double, Rows2, Cols2>> &op2) const {
//     if (op1->rows() != op2->rows()) {
//       return boost::none;
//     }
//     if (op1->cols() != 1 || op2->cols() != 1) {
//       return boost::none;
//     }
  
//     auto r = std::make_shared<Eigen::Matrix<double, Rows1 == Eigen::Dynamic ? Rows2 : Rows1, 1>>(op1->rows());
//     r->noalias() = op1->cross(*op2);

//     return MatrixObject(std::move(r), /* is_vector= */ true);
//   }
// };

boost::optional<MatrixObject> Value::MatrixObject::cross(const MatrixObject &other) const {
  return boost::none;
  // return boost::apply_visitor(cross_visitor(), data, other.data);
}

#define SCALAR_VISITED_OP_IMPL(type, name) \
  struct name ## _visitor : public boost::static_visitor<type> { \
    template <class T> \
    type operator()(const std::shared_ptr<T>& mat) const { \
      return mat->name(); \
    } \
  }; \
  type Value::MatrixObject::name() const { \
    return boost::apply_visitor(name ## _visitor() , data); \
  }

SCALAR_VISITED_OP_IMPL(size_t, rows);
SCALAR_VISITED_OP_IMPL(size_t, cols);
SCALAR_VISITED_OP_IMPL(double, norm);
SCALAR_VISITED_OP_IMPL(double, minCoeff);
SCALAR_VISITED_OP_IMPL(double, maxCoeff);

template <int rows, int cols, class = void>
struct matrix_data_traits;

template <int rows, int cols>
struct matrix_data_traits<rows, cols, std::enable_if_t< std::is_assignable<MatrixObject::data_t, typename std::shared_ptr<Eigen::Matrix<double, rows, cols>>>::value>> {
  typedef typename Eigen::Matrix<double, rows, cols> matrix_type;
};

template <int rows, int cols>
struct matrix_data_traits<rows, cols, std::enable_if_t<!std::is_assignable<MatrixObject::data_t, typename std::shared_ptr<Eigen::Matrix<double, rows, cols>>>::value>> {
  typedef typename Eigen::MatrixXd matrix_type;
};

template <bool lhs_is_vector>
struct product_visitor : public boost::static_visitor<boost::optional<MatrixObject>>
{
  bool rhs_is_vector;
  product_visitor(bool rhs_is_vector) : rhs_is_vector(rhs_is_vector) {}

  template <class A, class B>
  boost::optional<MatrixObject> operator()(const A& out, const B& item) const {
    std::cerr << "product op not supported " << typeName<A>().c_str() << " * " << typeName<B>().c_str();
    return boost::none;
  }
  
  template <class A, class B,
    typename std::enable_if_t<
        (matrix_traits<A>::cols::value == matrix_traits<B>::rows::value ||
        matrix_traits<A>::cols::value == Eigen::Dynamic ||
        matrix_traits<B>::rows::value == Eigen::Dynamic)/* &&
        std::is_assignable<
          MatrixObject::data_t, 
          typename std::shared_ptr<Eigen::Matrix<double, matrix_traits<A>::rows::value, matrix_traits<B>::cols::value>>>::value*/,
      int> = 0>
  boost::optional<MatrixObject> operator()(const std::shared_ptr<A>& op1, const std::shared_ptr<B>& op2) const {
    if (op1->cols() != op2->rows()) {
      return boost::none;
    }

    // TODO: lazy matrix for types that make sense.
    auto m = std::make_shared<typename matrix_data_traits<matrix_traits<A>::rows::value, matrix_traits<B>::cols::value>::matrix_type>(op1->rows(), op2->cols());
    // auto m = std::make_shared<Eigen::Matrix<double, matrix_traits<A>::rows::value, matrix_traits<B>::cols::value>>(op1->rows(), op2->cols());
    m->resize(op1->rows(), op2->cols());
    m->noalias() = *op1 * *op2;

    return MatrixObject(std::move(m), lhs_is_vector || rhs_is_vector);
  }
};

boost::optional<MatrixObject> Value::MatrixObject::operator*(const MatrixObject& other) const {
  return is_vector
    ? boost::apply_visitor(product_visitor</* lhs_is_vector= */ true>(/* rhs_is_vector= */ other.is_vector), data, other.data)
    : boost::apply_visitor(product_visitor</* lhs_is_vector= */ false>(/* rhs_is_vector= */ other.is_vector), data, other.data);
}

template <class Op>
struct cwise_nullary_op_visitor : public boost::static_visitor<MatrixObject>
{
  const Op &op;
  bool is_vector;
  cwise_nullary_op_visitor(const Op &op, bool is_vector) : op(op), is_vector(is_vector) {}

  template <class A>
  MatrixObject operator()(const std::shared_ptr<A>& op1) const {
    auto m = std::make_shared<typename matrix_traits<A>::matrix>(op1->rows(), op1->cols());
    op(*op1, *m);
    // TODO(ochafik): resize when dynamic
    
    return MatrixObject(std::move(m), is_vector);
  }
};

struct multiply_op {
  double factor;
  multiply_op(double factor) : factor(factor) {}  
  
  template <class A, class C>
  void operator()(const A&a, C &out) const {
    out.noalias() = a * factor;
  }
};

struct divide_op {
  double factor;
  divide_op(double factor) : factor(factor) {}  
  
  template <class A, class C>
  void operator()(const A&a, C &out) const {
    out.noalias() = a / factor;
  }
};

MatrixObject Value::MatrixObject::operator*(double scalar) const {
  return boost::apply_visitor(cwise_nullary_op_visitor<multiply_op>(multiply_op(scalar), is_vector) , data);
}

MatrixObject Value::MatrixObject::operator/(double scalar) const {
  return boost::apply_visitor(cwise_nullary_op_visitor<divide_op>(divide_op(scalar), is_vector) , data);
}

struct less_visitor : public boost::static_visitor<bool> {
  template <int Rows1, int Cols1, int Rows2, int Cols2>
  bool operator()(
      const std::shared_ptr<Eigen::Matrix<double, Rows1, Cols1>>& lhs,
      const std::shared_ptr<Eigen::Matrix<double, Rows2, Cols2>>& rhs) const {
    assert(lhs->rows() == rhs->rows());
    assert(rhs->cols() == rhs->cols());

    for (size_t i = 0, n = lhs->rows(); i < n; i++) {
      for (size_t j = 0, n = lhs->cols(); j < n; j++) {
        if (!((*lhs)(i, j) < (*rhs)(i, j))) {
          return false;
        }
      }
    }
    return true;
  }
};

bool Value::MatrixObject::operator<(const MatrixObject& other) const {
  return boost::apply_visitor(less_visitor(), data, other.data);
}

struct more_visitor : public boost::static_visitor<bool> {
  template <int Rows1, int Cols1, int Rows2, int Cols2>
  bool operator()(
      const std::shared_ptr<Eigen::Matrix<double, Rows1, Cols1>>& lhs,
      const std::shared_ptr<Eigen::Matrix<double, Rows2, Cols2>>& rhs) const {
    assert(lhs->rows() == rhs->rows());
    assert(rhs->cols() == rhs->cols());

    for (size_t i = 0, n = lhs->rows(); i < n; i++) {
      for (size_t j = 0, n = lhs->cols(); j < n; j++) {
        if (!((*lhs)(i, j) > (*rhs)(i, j))) {
          return false;
        }
      }
    }
    return true;
  }
};

bool Value::MatrixObject::operator>(const MatrixObject& other) const {
  return boost::apply_visitor(more_visitor(), data, other.data);
}

struct equal_op : public boost::static_visitor<bool> {
  template <class A, class B>
  bool operator()(const A&a, const B&b) const {
    return a == b;
  }

  template <int Rows, int Cols>
  bool operator()(
      std::shared_ptr<Eigen::Matrix<double, Rows, Cols>> op1,
      std::shared_ptr<Eigen::Matrix<double, Rows, Cols>> op2) const {
    return *op1 == *op2;
  }

  template <int Rows1, int Cols1, int Rows2, int Cols2,
    typename std::enable_if_t<Rows1 != Rows2 || Cols1 != Cols2, int> = 0>
  bool operator()(
      std::shared_ptr<Eigen::Matrix<double, Rows1, Cols1>> op1,
      std::shared_ptr<Eigen::Matrix<double, Rows2, Cols2>> op2) const {
    if (op1->rows() != op2->rows() || op1->cols() != op2->cols()) {
      return false;
    }
    
    for (size_t i = 0, n = op1->rows(); i < n; i++) {
      for (size_t j = 0, n = op1->cols(); j < n; j++) {
        if ((*op1)(i, j) != (*op2)(i, j)) {
          return false;
        }
      }
    }
    return true;
  }
};

bool Value::MatrixObject::operator==(const MatrixObject& other) const {
  return boost::apply_visitor(equal_op(), data, other.data);
}
