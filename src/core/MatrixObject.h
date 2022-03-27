#pragma once

// #define BOOST_MPL_LIMIT_LIST_SIZE 40
// #define BOOST_MPL_LIMIT_VECTOR_SIZE 50

#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <iostream>
#include <memory>
#include <Eigen/Core>

// Workaround for https://bugreports.qt-project.org/browse/QTBUG-22829
#ifndef Q_MOC_RUN
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#endif

#include "Assignment.h"
#include "linalg.h"
#include "memory.h"

class MatrixObject {
  
  // typedef Eigen::internal::scalar_sum_op<double, double> ScalarSum;
  // typedef Eigen::internal::scalar_product_op<double, double> ScalarProduct;
  // typedef Eigen::internal::scalar_difference_op<double, double> ScalarDifference;
  // typedef Eigen::internal::scalar_quotient_op<double, double> ScalarQuotient;
  // typedef Eigen::internal::scalar_opposite_op<double> ScalarOpposite;
  // typedef Eigen::internal::scalar_min_op<double, double> ScalarMin;
  // typedef Eigen::internal::scalar_max_op<double, double> ScalarMax;

 public:

  template <int Rows, int Cols, class T>
  class LazyMatrix;


  typedef boost::variant<
    boost::blank,
    
    std::shared_ptr<Eigen::Vector2d>,  // 2D point
    std::shared_ptr<Eigen::Vector3d>,  // 3D point or row of a 2D transform
    std::shared_ptr<Eigen::Vector4d>,  // Rows of a 3D transform
    std::shared_ptr<Eigen::VectorXd>,  // Generic list of numbers
    
    std::shared_ptr<Eigen::Matrix2d>,  // ...
    std::shared_ptr<Eigen::Matrix3d>,  // 2D transform
    // std::shared_ptr<Eigen::Matrix<double, 3, 2>>, // ...
    // std::shared_ptr<Eigen::Matrix<double, 4, 2>>, // ...
    std::shared_ptr<Eigen::Matrix<double, 4, 3>>, // 3D matrix for vec(pts) * matrix once transposed by BOSL2 apply
    std::shared_ptr<Eigen::Matrix4d>,  // 3D transform
    std::shared_ptr<Eigen::MatrixX2d>, // List of 2D points
    std::shared_ptr<Eigen::MatrixX3d>, // List of 3D points
    std::shared_ptr<Eigen::MatrixX4d>, // List of 3D points (concatenated w/ 1) about to be transformed by BOSL2 apply 
    // std::shared_ptr<Eigen::Matrix<double, 2, Eigen::Dynamic>>, // ...
    // std::shared_ptr<Eigen::Matrix<double, 3, Eigen::Dynamic>>, // ...
    // std::shared_ptr<Eigen::Matrix<double, 4, Eigen::Dynamic>>, // ...
    std::shared_ptr<Eigen::MatrixXd>,  // Generic matrix of numbers

    // These are growable w/ support to reserve capacity
    // std::shared_ptr<std::pair<Eigen::VectorXd, size_t>>,
    std::shared_ptr<std::pair<Eigen::MatrixXd, size_t>>

    // std::shared_ptr<std::pair<Eigen::MatrixX2d, size_t>>, // breaks compilation
    // std::shared_ptr<std::pair<Eigen::MatrixX3d, size_t>>, // breaks compilation
    // std::shared_ptr<std::pair<Eigen::MatrixX4d, size_t>> // breaks compilation

    // std::shared_ptr<LazyMatrix<4, 4,
    //                      Eigen::Product<const Eigen::Matrix4d, const Eigen::Matrix4d>>>,
    // // TODO: add other scalar ops
    // std::shared_ptr<LazyMatrix<4, 4,
    //                      Eigen::CwiseBinaryOp<ScalarSum, const Eigen::Matrix4d, const Eigen::Matrix4d>>>
  > data_t;

  template <int Rows, int Cols, class T>
  class LazyMatrix {
    //}: public std::enable_shared_from_this<LazyMatrix<Rows, Cols, T>> {
    std::function<std::shared_ptr<T>()> getter;
    mutable boost::optional<std::shared_ptr<T>> value;
    mutable boost::optional<data_t> matrix;

   public:
    LazyMatrix(std::function<T()> &&getter) : getter(std::move(getter)) {}

    data_t getMatrix() const {
      if (!matrix) {
        matrix = std::make_shared<std::pair<Eigen::Matrix<double, Rows, Cols>, size_t>>(*getValue(), Rows);
      }
      return matrix;
    }
    std::shared_ptr<T> getValue() const {
      if (matrix) {
        return *matrix;
      }
      if (!value) {
        value = getter();
      }
      return *value;
    }
  };

 private:
  // size_t initial_capacity;
  mutable data_t data;
  mutable bool sealed;

 public:
  MatrixObject() : sealed(false) {}
  // MatrixObject(size_t capacity): initial_capacity(capacity) 
  MatrixObject(data_t &&data) : data(std::move(data)), sealed(false) {}

  operator bool() const {
    return data.which() != 0; // != std::monostate;
  }

  void reserveDoubles(size_t rows);
  void reserveDoubleArrays(size_t rows, size_t columns);
  bool reserve(size_t rows);

  bool push_back(double value);
  bool push_back(const MatrixObject& element);

  void seal() const;
  size_t rows() const;

  boost::optional<MatrixObject> operator+(const MatrixObject& element);
  boost::optional<MatrixObject> operator-(const MatrixObject& element);
  boost::optional<MatrixObject> operator*(const MatrixObject& element);
  boost::optional<MatrixObject> operator/(const MatrixObject& element);

  template <class T>
  static T getValue(const T& value) {
    return value;
  }

  // template <class Fn>
  void forEachRow(const std::function<void(class Value&&)>& fn) const;

  template <int Rows, int Cols, class T>
  static T getValue(const LazyMatrix<Rows, Cols, T>& lazyValue) {
    return lazyValue.get();
  }
};
