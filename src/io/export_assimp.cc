/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2016 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
../buildOpenSCAD.sh Debug -DCMAKE_MODULE_PATH=$HOME/Downloads && echo 'color("red") cube(); color("blue") translate([1, 0, 0]) sphere();' | ./buildDebug/OpenSCAD.app/Contents/MacOS/OpenSCAD - -o out.3mf --enable=lazy-union --enable=assimp
*/

#include "export.h"
#include "PolySet.h"
#include "PolySetUtils.h"
#include "printutils.h"
#ifdef ENABLE_CGAL
#include "CGALHybridPolyhedron.h"
#include "cgal.h"
#include "cgalutils.h"
#include "CGAL_Nef_polyhedron.h"
#endif
#ifdef ENABLE_MANIFOLD
#include "ManifoldGeometry.h"
#endif

#ifdef ENABLE_ASSIMP

#include <assimp/scene.h>
#include <assimp/Exporter.hpp>

#include <algorithm>

const char *getFormat(FileFormat format) {
  switch (format) {
    case FileFormat::ASCIISTL: return "stl";
    case FileFormat::STL: return "stlb";
    case FileFormat::OBJ: return "obj";

    case FileFormat::COLLADA: return "collada";
    case FileFormat::STP: return "stp";
    case FileFormat::ASCIIPLY: return "ply";
    case FileFormat::PLY: return "plyb";
    case FileFormat::_3DS: return "3ds";
    case FileFormat::_3MF: return "3mf";
    case FileFormat::GLTF2: return "gltf2";
    // case FileFormat::GLTF: return "gltf";
    case FileFormat::GLB2: return "glb2";
    // case FileFormat::GLB: return "glb";
    case FileFormat::X3D: return "x3d";
    case FileFormat::FBX: return "fbx";
    case FileFormat::FBXA: return "fbxa";
    case FileFormat::M3D: return "m3d";
    case FileFormat::M3DA: return "m3da";
    case FileFormat::PBRT: return "pbrt";
    default:
      return NULL;
    // case FileFormat::OFF:
    // case FileFormat::WRL:
    // case FileFormat::AMF:

    // case FileFormat::PDF,
    // case FileFormat::SVG:
    // case FileFormat::DXF:
  }
  // collada
  // stp
  // obj
  // objnomtl
  // stl
  // stlb
  // ply
  // plyb
  // 3ds
  // gltf2
  // glb2
  // gltf
  // glb
  // assbin
  // assxml
  // x3d
  // fbx
  // fbxa
  // m3d
  // m3da
  // 3mf
  // pbrt
}

struct AiSceneBuilder {
  std::vector<aiMaterial*> materials;
  std::vector<aiMesh*> meshes;

  ~AiSceneBuilder() {
    // Only delete the objects if their ownership wasn't moved to the scene.
    for (auto material : materials) {
      delete material;
    }
    for (auto mesh : meshes) {
      delete mesh;
    }
  }

  int addColorMaterial(Color4f color) {
    auto material = new aiMaterial();

    aiColor4D diffuse;
    diffuse.r = color[0];
    diffuse.g = color[1];
    diffuse.b = color[2];
    diffuse.a = color[3];
    material->AddProperty(&diffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
    // material->AddProperty(&diffuse, 1, AI_MATKEY_COLOR_SPECULAR);
    // material->AddProperty(&diffuse, 1, AI_MATKEY_COLOR_AMBIENT);
    materials.push_back(material);
    return materials.size() - 1;
  }

  void addMesh(const PolySet& ps)
  {
    auto mesh = new aiMesh();
    
    if (ps.color.isValid()) {
      mesh->mMaterialIndex = addColorMaterial(ps.color);
    }

    mesh->mNumVertices = ps.vertices.size();
    mesh->mVertices = new aiVector3D[ps.vertices.size()];
    for (int i = 0; i < ps.vertices.size(); i++) {
      mesh->mVertices[i] = aiVector3D(ps.vertices[i][0], ps.vertices[i][1], ps.vertices[i][2]);
    }

    mesh->mNumFaces = ps.indices.size();
    mesh->mFaces = new aiFace[ps.indices.size()];
    for (int i = 0; i < ps.indices.size(); i++) {
      mesh->mFaces[i].mNumIndices = 3;
      mesh->mFaces[i].mIndices = new unsigned int[3];
      mesh->mFaces[i].mIndices[0] = ps.indices[i][0];
      mesh->mFaces[i].mIndices[1] = ps.indices[i][1];
      mesh->mFaces[i].mIndices[2] = ps.indices[i][2];
    }

    meshes.push_back(mesh);
  }

  std::unique_ptr<aiScene> toScene() {
    auto scene = std::make_unique<aiScene>();

    // Copy materials array
    scene->mMaterials = new aiMaterial*[materials.size()];
    std::copy(materials.begin(), materials.end(), scene->mMaterials);
    scene->mNumMaterials = materials.size();

    // Copy meshes array
    scene->mMeshes = new aiMesh*[meshes.size()];
    std::copy(meshes.begin(), meshes.end(), scene->mMeshes);
    scene->mNumMeshes = meshes.size();

    // Put all meshes under the root node.
    scene->mRootNode = new aiNode();

    scene->mRootNode->mMeshes = new unsigned int[meshes.size()];
    for (int i = 0; i < meshes.size(); i++) {
      scene->mRootNode->mMeshes[i] = i;
    }
    scene->mRootNode->mNumMeshes = meshes.size();

    // Reset vectors so we don't attemp to delete them in the destructor.
    materials.clear();
    meshes.clear();

    return scene;
  }
};

/*
 * PolySet must be triangulated.
 */
static bool append_polyset(const PolySet& ps, AiSceneBuilder &builder)
{
  builder.addMesh(ps);
  return true;
}

#ifdef ENABLE_CGAL
static bool append_nef(const CGAL_Nef_polyhedron& root_N, AiSceneBuilder &builder)
{
  if (!root_N.p3) {
    LOG(message_group::Export_Error, "Export failed, empty geometry.");
    return false;
  }

  if (!root_N.p3->is_simple()) {
    LOG(message_group::Export_Warning, "Exported object may not be a valid 2-manifold and may need repair");
  }


  if (const auto ps = CGALUtils::createPolySetFromNefPolyhedron3(*root_N.p3)) {
    return append_polyset(*ps, builder);
  }

  // export_assimp_error("Error converting NEF Polyhedron.", model);
  return false;
}
#endif

static bool append_assimp(const std::shared_ptr<const Geometry>& geom, AiSceneBuilder &builder)
{
  if (const auto geomlist = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto& item : geomlist->getChildren()) {
      if (!append_assimp(item.second, builder)) return false;
    }
#ifdef ENABLE_CGAL
  } else if (const auto N = std::dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
    return append_nef(*N, builder);
  } else if (const auto hybrid = std::dynamic_pointer_cast<const CGALHybridPolyhedron>(geom)) {
    return append_polyset(*hybrid->toPolySet(), builder);
#endif
#ifdef ENABLE_MANIFOLD
  } else if (const auto mani = std::dynamic_pointer_cast<const ManifoldGeometry>(geom)) {
    return append_polyset(*mani->toPolySet(), builder);
#endif
  } else if (const auto ps = std::dynamic_pointer_cast<const PolySet>(geom)) {
    return append_polyset(*PolySetUtils::tessellate_faces(*ps), builder);
  } else if (std::dynamic_pointer_cast<const Polygon2d>(geom)) { // NOLINT(bugprone-branch-clone)
    assert(false && "Unsupported file format");
  } else { // NOLINT(bugprone-branch-clone)
    assert(false && "Not implemented");
  }

  return true;
}

bool export_assimp(const std::shared_ptr<const Geometry>& geom, std::ostream& output, FileFormat format)
{
  const char *formatName = getFormat(format);
  if (!formatName) {
    LOG("Unsupported file format.");
    return false;
  }
  AiSceneBuilder builder;
  if (!append_assimp(geom, builder)) {
    LOG("Assimp export failed.");
    return false;
  }
  auto scene = builder.toScene();
  ::Assimp::Exporter exporter;
  // exporter.Export(scene.get(), formatName, "out_file.gltf");
  const aiExportDataBlob *blob = exporter.ExportToBlob(scene.get(), formatName);
  if (!blob) {
    LOG("Assimp exporter failed: %1$s.", exporter.GetErrorString());
    printf("%s\n", exporter.GetErrorString());
    return false;
  }
  output.write(reinterpret_cast<const char*>(blob->data), blob->size);
  return true;
}

#else // ENABLE_ASSIMP

bool export_assimp(const std::shared_ptr<const Geometry>& geom, std::ostream& output, FileFormat format)
{
  LOG("Export to assimp was not enabled when building the application.");
  return false;
}

#endif // ENABLE_ASSIMP
