// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "CGALHybridPolyhedron.h"

#include "cgalutils.h"
#include "hash.h"
#include <CGAL/Surface_mesh.h>
#include <CGAL/boost/graph/helpers.h>

typedef CGALHybridPolyhedron::immediate_data_t immediate_data_t;
typedef CGALHybridPolyhedron::future_data_t future_data_t;

typedef CGALHybridPolyhedron::hybrid_data_t hybrid_data_t;
typedef CGALHybridPolyhedron::nef_polyhedron_t nef_polyhedron_t;
typedef CGALHybridPolyhedron::mesh_t mesh_t;

/**
 * Will force lazy coordinates to be exact to avoid subsequent performance issues
 * (only if the kernel is lazy), and will also collect the mesh's garbage if applicable.
 */
void cleanupMesh(CGALHybridPolyhedron::mesh_t& mesh, bool is_corefinement_result)
{
  mesh.collect_garbage();
#if FAST_CSG_KERNEL_IS_LAZY
  // If exact corefinement callbacks are enabled, no need to make numbers exact here again.
  auto make_exact = 
    Feature::ExperimentalFastCsgExactCorefinementCallback.is_enabled()
      ? !is_corefinement_result
      : Feature::ExperimentalFastCsgExact.is_enabled();

  if (make_exact) {
    for (auto v : mesh.vertices()) {
      auto &pt = mesh.point(v);
      CGAL::exact(pt.x());
      CGAL::exact(pt.y());
      CGAL::exact(pt.z());
    }
  }
#endif // FAST_CSG_KERNEL_IS_LAZY
}

CGALHybridPolyhedron::CGALHybridPolyhedron(const shared_ptr<nef_polyhedron_t>& nef)
{
  assert(nef);
  data = nef;
  estimated_facet_count = -1;
  estimated_vertex_count = -1;
}

CGALHybridPolyhedron::CGALHybridPolyhedron(const shared_ptr<mesh_t>& mesh)
{
  assert(mesh);
  data = mesh;
  estimated_facet_count = -1;
  estimated_vertex_count = -1;
}

CGALHybridPolyhedron::CGALHybridPolyhedron(const CGALHybridPolyhedron& other)
{
  *this = other;
}

immediate_data_t copyImmediateData(const immediate_data_t& data)
{
  if (auto pNef = boost::get<shared_ptr<nef_polyhedron_t>>(&data)) {
    return make_shared<nef_polyhedron_t>(*pNef->get());
  } else if (auto pMesh = boost::get<shared_ptr<mesh_t>>(&data)) {
    return make_shared<mesh_t>(*pMesh->get());
  } else {
    assert(!"Bad hybrid polyhedron state");
    return immediate_data_t(); 
  }
}

CGALHybridPolyhedron& CGALHybridPolyhedron::operator=(const CGALHybridPolyhedron& other)
{
  auto otherData = other.data;
  if (hasImmediateData(otherData)) {
    data = copyImmediateData(getImmediateData(otherData));
    estimated_facet_count = other.estimated_facet_count;
    estimated_vertex_count = other.estimated_vertex_count;
  } else {
    data = std::shared_future<immediate_data_t>(std::async(std::launch::async, [otherData]() {
      return copyImmediateData(getImmediateData(otherData));
    }));
    estimated_facet_count = -1;
    estimated_vertex_count = -1;
  }
  return *this;
}

CGALHybridPolyhedron::CGALHybridPolyhedron()
{
  data = make_shared<CGALHybridPolyhedron::mesh_t>();
  estimated_facet_count = 0;
  estimated_vertex_count = 0;
}

shared_ptr<CGALHybridPolyhedron::nef_polyhedron_t> CGALHybridPolyhedron::getNefPolyhedron(const immediate_data_t &data)
{
  auto pp = boost::get<shared_ptr<nef_polyhedron_t>>(&data);
  return pp ? *pp : nullptr;
}

shared_ptr<CGALHybridPolyhedron::mesh_t> CGALHybridPolyhedron::getMesh(const immediate_data_t &data)
{
  auto pp = boost::get<shared_ptr<mesh_t>>(&data);
  return pp ? *pp : nullptr;
}

bool CGALHybridPolyhedron::hasImmediateData(const hybrid_data_t &data)
{
  return boost::get<shared_ptr<mesh_t>>(&data) ||
    boost::get<shared_ptr<nef_polyhedron_t>>(&data);;
}

CGALHybridPolyhedron::immediate_data_t CGALHybridPolyhedron::getImmediateData(const hybrid_data_t &data)
{
  if (auto pMesh = boost::get<shared_ptr<mesh_t>>(&data)) {
    return *pMesh;
  }
  if (auto pNef = boost::get<shared_ptr<nef_polyhedron_t>>(&data)) {
    return *pNef;
  }
  if (auto pFuture = boost::get<future_data_t>(&data)) {
    // Blocking call:
    return pFuture->get();
  }
  assert(!"Bad hybrid polyhedron state");
  return immediate_data_t();
}

bool CGALHybridPolyhedron::isEmpty() const
{
  return numFacets() == 0;
}

size_t CGALHybridPolyhedron::numFacets(const immediate_data_t &data)
{
  if (auto nef = getNefPolyhedron(data)) {
    return nef->number_of_facets();
  } else if (auto mesh = getMesh(data)) {
    return mesh->number_of_faces();
  }
  assert(!"Bad hybrid polyhedron state");
  return 0;
}

size_t CGALHybridPolyhedron::numFacets() const
{
  if (Feature::ExperimentalFastCsgParallelUnsafe.is_enabled() && !hasImmediateData(data)) {
    return estimated_facet_count;
  }
  return numFacets(getImmediateData(data));
}

size_t CGALHybridPolyhedron::numVertices(const immediate_data_t &data)
{
  if (auto nef = getNefPolyhedron(data)) {
    return nef->number_of_vertices();
  } else if (auto mesh = getMesh(data)) {
    return mesh->number_of_vertices();
  }
  assert(!"Bad hybrid polyhedron state");
  return 0;
}

size_t CGALHybridPolyhedron::numVertices() const
{
  if (Feature::ExperimentalFastCsgParallelUnsafe.is_enabled() && !hasImmediateData(data)) {
    return estimated_vertex_count;
  }
  return numVertices(getImmediateData(data));
}

bool CGALHybridPolyhedron::isManifold() const
{
  return isManifold(getImmediateData(data));
}

bool CGALHybridPolyhedron::isManifold(const immediate_data_t &data)
{
  if (auto mesh = getMesh(data)) {
    // Note: haven't tried mesh->is_valid() but it could be too expensive.
    return CGAL::is_closed(*mesh);
  } else if (auto nef = getNefPolyhedron(data)) {
    return nef->is_simple();
  }
  assert(!"Bad hybrid polyhedron state");
  return false;
}

shared_ptr<const PolySet> CGALHybridPolyhedron::toPolySet() const
{
  auto immediateData = getImmediateData(data);
  if (auto mesh = getMesh(immediateData)) {
    auto ps = make_shared<PolySet>(3, /* convex */ unknown);
    auto err = CGALUtils::createPolySetFromMesh(*mesh, *ps);
    assert(!err);
    return ps;
  } else if (auto nef = getNefPolyhedron(immediateData)) {
    auto ps = make_shared<PolySet>(3, /* convex */ unknown);
    auto err = CGALUtils::createPolySetFromNefPolyhedron3(*nef, *ps);
    assert(!err);
    return ps;
  } else {
    assert(!"Bad hybrid polyhedron state");
    return nullptr;
  }
}

void CGALHybridPolyhedron::clear()
{
  data = make_shared<mesh_t>();
  estimated_facet_count = -1;
  estimated_vertex_count = -1;
}

// We don't parallelize mesh<->nefs conversions yet, and doing so without ditching nefs that can be reused will need some refactoring.
void CGALHybridPolyhedron::runOperation(
    CGALHybridPolyhedron& other,
    const std::function<void(immediate_data_t &, immediate_data_t &)> immediateOp) {
  auto lhs = data;
  auto rhs = other.data;
  if (Feature::ExperimentalFastCsgParallelUnsafe.is_enabled()) {
    data = std::shared_future<immediate_data_t>(std::async(std::launch::async, [=]() {
      // Start with two blocking calls (execution graph dependencies):
      auto lhsImmediate = getImmediateData(lhs);
      auto rhsImmediate = getImmediateData(rhs);

      // TODO(ochafik): Acquire operation token here from semaphore to limit number of active operation threads.
      immediateOp(lhsImmediate, rhsImmediate);
      return lhsImmediate;
    }));
    estimated_facet_count = ((numFacets() + other.numFacets()) * 120) / 100;
    estimated_vertex_count = ((numVertices() + other.numVertices()) * 120) / 100;
  } else {
    auto lhsImmediate = getImmediateData(lhs);
    auto rhsImmediate = getImmediateData(rhs);
    immediateOp(lhsImmediate, rhsImmediate);

    data = lhsImmediate;
    estimated_facet_count = -1;
    estimated_vertex_count = -1;
  }
}

void CGALHybridPolyhedron::operator+=(CGALHybridPolyhedron& other)
{
  runOperation(other, [](immediate_data_t &lhs, immediate_data_t &rhs) {
    if (meshBinOp("corefinement mesh union", lhs, rhs,
                    [](mesh_t& lhs, mesh_t& rhs, mesh_t& out) {
        return CGALUtils::corefineAndComputeUnion(lhs, rhs, out);
      })) return;

    nefPolyBinOp("nef union", lhs, rhs,
                [](nef_polyhedron_t& destinationNef, nef_polyhedron_t& otherNef) {
      CGALUtils::inPlaceNefUnion(destinationNef, otherNef);
    });
  });
}

void CGALHybridPolyhedron::operator*=(CGALHybridPolyhedron& other)
{
  runOperation(other, [](immediate_data_t &lhs, immediate_data_t &rhs) {
    if (meshBinOp("corefinement mesh intersection", lhs, rhs,
                    [](mesh_t& lhs, mesh_t& rhs, mesh_t& out) {
        return CGALUtils::corefineAndComputeIntersection(lhs, rhs, out);
      })) return;

    nefPolyBinOp("nef intersection", lhs, rhs,
                [](nef_polyhedron_t& destinationNef, nef_polyhedron_t& otherNef) {
      CGALUtils::inPlaceNefIntersection(destinationNef, otherNef);
    });
  });
}

void CGALHybridPolyhedron::operator-=(CGALHybridPolyhedron& other)
{
  runOperation(other, [](immediate_data_t &lhs, immediate_data_t &rhs) {
    if (meshBinOp("corefinement mesh difference", lhs, rhs,
                    [](mesh_t& lhs, mesh_t& rhs, mesh_t& out) {
        return CGALUtils::corefineAndComputeDifference(lhs, rhs, out);
      })) return;

    nefPolyBinOp("nef difference", lhs, rhs,
                [](nef_polyhedron_t& destinationNef, nef_polyhedron_t& otherNef) {
      CGALUtils::inPlaceNefDifference(destinationNef, otherNef);
    });
  });
}

void CGALHybridPolyhedron::minkowski(CGALHybridPolyhedron& other)
{
  runOperation(other, [](immediate_data_t &lhs, immediate_data_t &rhs) {
    nefPolyBinOp("minkowski", lhs, rhs,
                [&](nef_polyhedron_t& destinationNef, nef_polyhedron_t& otherNef) {
      CGALUtils::inPlaceNefMinkowski(destinationNef, otherNef);
    });
  });
}

void CGALHybridPolyhedron::transform(const Transform3d& mat)
{
  auto run = [mat](const immediate_data_t &data) -> immediate_data_t {
    if (mat.matrix().determinant() == 0) {
      LOG(message_group::Warning, Location::NONE, "", "Scaling a 3D object with 0 - removing object");
      return make_shared<mesh_t>();
    } else {
      auto t = CGALUtils::createAffineTransformFromMatrix<CGAL_HybridKernel3>(mat);

      if (auto mesh = getMesh(data)) {
        CGALUtils::transform(*mesh, mat);
        cleanupMesh(*mesh, /* is_corefinement_result */ false);
      } else if (auto nef = getNefPolyhedron(data)) {
        CGALUtils::transform(*nef, mat);
      } else {
        assert(!"Bad hybrid polyhedron state");
      }
      return data;
    }
  };

  if (hasImmediateData(data)) {
    data = run(getImmediateData(data));
  } else {
    auto previousData = data;
    data = std::shared_future<immediate_data_t>(std::async(std::launch::async, [run, previousData]() {
      return run(getImmediateData(previousData));
    }));
  }
}

void CGALHybridPolyhedron::resize(
  const Vector3d& newsize, const Eigen::Matrix<bool, 3, 1>& autosize)
{
  if (this->isEmpty()) return;

  transform(
    CGALUtils::computeResizeTransform(getExactBoundingBox(), getDimension(), newsize, autosize));
}

CGALHybridPolyhedron::bbox_t CGALHybridPolyhedron::getExactBoundingBox() const
{
  // TODO(ochafik): Make this async / non blocking.

  bbox_t result(0, 0, 0, 0, 0, 0);
  std::vector<point_t> points;
  // TODO(ochafik): Optimize this!
  foreachVertexUntilTrue(data, [&](const auto& pt) {
    points.push_back(pt);
    return false;
  });
  if (points.size()) CGAL::bounding_box(points.begin(), points.end());
  return result;
}

std::string CGALHybridPolyhedron::dump() const
{
  assert(!"TODO: implement CGALHybridPolyhedron::dump!");
  return "?";
  // return OpenSCAD::dump_svg(toPolySet());
}

size_t CGALHybridPolyhedron::memsize() const
{
  size_t total = sizeof(CGALHybridPolyhedron);
  if (hasImmediateData(data)) {
    auto immediateData = getImmediateData(data);
    if (auto mesh = getMesh(immediateData)) {
      total += numFacets() * 3 * sizeof(size_t);
      total += numVertices() * sizeof(point_t);
    } else if (auto nef = getNefPolyhedron(immediateData)) {
      total += nef->bytes();
    }
  } else {
    // TODO: Add Nef inflation factor for more pessimistic estimate
    total += estimated_facet_count * 3 * sizeof(size_t);
    total += estimated_vertex_count * 3 * sizeof(point_t);
  }
  return total;
}

void CGALHybridPolyhedron::foreachVertexUntilTrue(const std::function<bool(const point_t& pt)>& f) const
{
  foreachVertexUntilTrue(data, f);
}

void CGALHybridPolyhedron::foreachVertexUntilTrue(
  const hybrid_data_t &data, const std::function<bool(const point_t& pt)>& f)
{
  auto immediateData = getImmediateData(data);
  if (auto mesh = getMesh(immediateData)) {
    for (auto v : mesh->vertices()) {
      if (f(mesh->point(v))) return;
    }
  } else if (auto nef = getNefPolyhedron(immediateData)) {
    nef_polyhedron_t::Vertex_const_iterator vi;
    CGAL_forall_vertices(vi, *nef)
    {
      if (f(vi->point())) return;
    }
  } else {
    assert(!"Bad hybrid polyhedron state");
  }
}

void CGALHybridPolyhedron::nefPolyBinOp(
  const std::string& opName, immediate_data_t &lhs, const immediate_data_t &rhs,
  const std::function<void(nef_polyhedron_t& destinationNef, nef_polyhedron_t& otherNef)>
  & operation)
{
  LOG(message_group::None, Location::NONE, "", "[fast-csg] %1$s (%2$lu vs. %3$lu facets)",
      opName.c_str(), numFacets(lhs), numFacets(rhs));

  auto lhsNef = convertToNef(lhs);
  auto rhsNef = convertToNef(rhs);
  operation(*lhsNef, *rhsNef);
  lhs = lhsNef;
}

bool CGALHybridPolyhedron::meshBinOp(
  const std::string& opName, immediate_data_t &lhs, const immediate_data_t &rhs,
  const std::function<bool(mesh_t& lhs, mesh_t& rhs, mesh_t& out)>& operation)
{
  if (!Feature::ExperimentalFastCsgTrustCorefinement.is_enabled() &&
        (sharesAnyVertices(lhs, rhs) || !isManifold(lhs) || !isManifold(rhs))) {
    return false;
  }

  LOG(message_group::None, Location::NONE, "", "[fast-csg] %1$s (%2$lu vs. %3$lu facets)",
      opName.c_str(), numFacets(lhs), numFacets(rhs));

  auto success = false;
  try {
    auto lhsMesh = convertToMesh(lhs);
    auto rhsMesh = convertToMesh(rhs);

    if (Feature::ExperimentalFastCsgDebugCorefinement.is_enabled()) {
      static std::map<std::string, size_t> opCount;
      auto opNumber = opCount[opName]++;

      std::ostringstream lhsOut, rhsOut;
      lhsOut << opName << " " << opNumber << " lhs.off";
      rhsOut << opName << " " << opNumber << " rhs.off";
      lhsDebugDumpFile = lhsOut.str();
      rhsDebugDumpFile = rhsOut.str();
      
      std::ofstream(lhsDebugDumpFile) << *lhsMesh;
      std::ofstream(rhsDebugDumpFile) << *rhsMesh;
    }

    if ((success = operation(*lhsMesh, *rhsMesh, *lhsMesh))) {
      cleanupMesh(*lhsMesh, /* is_corefinement_result */ true);
      // cleanupMesh(rhsMesh, /* is_corefinement_result */ true);
      lhs = lhsMesh;

      if (Feature::ExperimentalFastCsgDebugCorefinement.is_enabled()) {
        remove(lhsDebugDumpFile.c_str());
        remove(rhsDebugDumpFile.c_str());
      }
    } else {
      LOG(message_group::Warning, Location::NONE, "", "[fast-csg] Corefinement %1$s failed",
          opName.c_str());
    }
  } catch (const std::exception& e) {
    // This can be a CGAL::Failure_exception, a CGAL::Intersection_of_constraints_exception or who
    // knows what else...
    success = false;
    LOG(message_group::Warning, Location::NONE, "",
        "[fast-csg] Corefinement %1$s failed with an error: %2$s", opName.c_str(), e.what());
  }

  return success;
}

shared_ptr<CGALHybridPolyhedron::nef_polyhedron_t> CGALHybridPolyhedron::convertToNef(const immediate_data_t &data)
{
  if (auto mesh = getMesh(data)) {
    return make_shared<nef_polyhedron_t>(*mesh);
  } else if (auto nef = getNefPolyhedron(data)) {
    return nef;
  } else {
    throw "Bad data state";
  }
}

shared_ptr<CGALHybridPolyhedron::mesh_t> CGALHybridPolyhedron::convertToMesh(const immediate_data_t &data)
{
  if (auto mesh = getMesh(data)) {
    return mesh;
  } else if (auto nef = getNefPolyhedron(data)) {
    auto mesh = make_shared<mesh_t>();
    CGALUtils::convertNefPolyhedronToTriangleMesh(*nef, *mesh);
    cleanupMesh(*mesh, /* is_corefinement_result */ false);
    return mesh;
  } else {
    throw "Bad data state";
  }
}

bool CGALHybridPolyhedron::sharesAnyVertices(const immediate_data_t lhs, const immediate_data_t rhs)
{
  if (numVertices(rhs) < numVertices(lhs)) {
    // The other has less vertices to index!
    return sharesAnyVertices(rhs, lhs);
  }

  std::unordered_set<point_t> vertices;
  foreachVertexUntilTrue(lhs, [&](const auto& p) {
    vertices.insert(p);
    return false;
  });

  auto foundCollision = false;
  foreachVertexUntilTrue(rhs,
    [&](const auto& p) {
    return foundCollision = vertices.find(p) != vertices.end();
  });

  return foundCollision;
}
