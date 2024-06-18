#include "import.h"
#include "Feature.h"
#include "PolySet.h"
#include "ManifoldGeometry.h"
#include "manifold.h"
#include "printutils.h"
#include "AST.h"
#include <fstream>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

// References:
// http://www.geomview.org/docs/html/OFF.html

std::unique_ptr<Geometry> import_off(const std::string& filename, const Location& loc)
{
  boost::regex ex_magic(R"(^(ST)?(C)?(N)?(4)?(n)?OFF( BINARY)? *)");
  // XXX: are ST C N always in order?
  boost::regex ex_cr(R"(\r$)");
  boost::regex ex_comment(R"(\s*#.*$)");
  boost::smatch results;

  std::ifstream f(filename.c_str(), std::ios::in | std::ios::binary);

  int lineno = 0;
  std::string line;

  auto AsciiError = [&](const auto& errstr){
    LOG(message_group::Error, loc, "",
    "OFF File line %1$s, %2$s line '%3$s' importing file '%4$s'",
    lineno, errstr, line, filename);
  };

  auto getline_clean = [&](const auto& errstr){
    do {
      lineno++;
      std::getline(f, line);
      if (line.empty() && f.eof()) {
        AsciiError(errstr);
        return false;
      }
      // strip DOS line endings
      if (boost::regex_search(line, results, ex_cr)) {
        line = line.erase(results.position(), results[0].length());
      }
      // strip comments
      if (boost::regex_search(line, results, ex_comment)) {
        line = line.erase(results.position(), results[0].length());
      }
      boost::trim(line);
    } while (line.empty());

    return true;
  };

  if (!f.good()) {
    AsciiError("File error");
    return PolySet::createEmpty();
  }

  bool got_magic = false;
  // defaults
  bool has_normals = false;
  bool has_color = false;
  bool has_textures = false;
  bool has_ndim = false;
  bool is_binary = false;
  unsigned int dimension = 3;

  if (line.empty() && !getline_clean("bad header: end of file")) {
      return PolySet::createEmpty();
  }

  if (boost::regex_search(line, results, ex_magic) > 0) {
    got_magic = true;
    // Remove the matched part, we might have numbers next.
    line = line.erase(0, results[0].length());
    has_normals = results[3].matched;
    has_color = results[2].matched;
    has_textures = results[1].matched;
    is_binary = results[6].matched;
    if (results[4].matched)
      dimension = 4;
    has_ndim = results[5].matched;
  }

  // TODO: handle binary format
  if (is_binary) {
    AsciiError("binary OFF format not supported");
    return PolySet::createEmpty();
  }

  std::vector<std::string> words;

  if (has_ndim) {
    if (line.empty() && !getline_clean("bad header: end of file")) {
        return PolySet::createEmpty();
    }
    boost::split(words, line, boost::is_any_of(" \t"), boost::token_compress_on);
    if (f.eof() || words.size() < 1) {
      AsciiError("bad header: missing Ndim");
      return PolySet::createEmpty();
    }
    line = line.erase(0, words[0].length() + ((words.size() > 1) ? 1 : 0));
    try {
      dimension = boost::lexical_cast<unsigned int>(words[0]) + dimension - 3;
    } catch (const boost::bad_lexical_cast& blc) {
      AsciiError("bad header: bad data for Ndim");
      return PolySet::createEmpty();
    }
  }

  PRINTDB("Header flags: N:%d C:%d ST:%d Ndim:%d B:%d", has_normals % has_color % has_textures % dimension % is_binary);

  if (dimension != 3) {
    AsciiError((boost::format("unhandled vertex dimensions (%d)") % dimension).str().c_str());
    return PolySet::createEmpty();
  }

  if (line.empty() && !getline_clean("bad header: end of file")) {
      return PolySet::createEmpty();
  }

  boost::split(words, line, boost::is_any_of(" \t"), boost::token_compress_on);
  if (f.eof() || words.size() < 3) {
    AsciiError("bad header: missing data");
    return PolySet::createEmpty();
  }

  unsigned long vertices_count;
  unsigned long faces_count;
  unsigned long edges_count;
  unsigned long vertex = 0;
  unsigned long face = 0;
  try {
    vertices_count = boost::lexical_cast<unsigned long>(words[0]);
    faces_count = boost::lexical_cast<unsigned long>(words[1]);
    edges_count = boost::lexical_cast<unsigned long>(words[2]);
    (void)edges_count; // ignored
  } catch (const boost::bad_lexical_cast& blc) {
    AsciiError("bad header: bad data");
    return PolySet::createEmpty();
  }

  if (f.eof() || vertices_count < 1 || faces_count < 1) {
    AsciiError("bad header: not enough data");
    return PolySet::createEmpty();
  }

  PRINTDB("%d vertices, %d faces, %d edges.", vertices_count % faces_count % edges_count);

  std::vector<Vector3d> vertices;

  while ((!f.eof()) && (vertex++ < vertices_count)) {
    if (!getline_clean("reading vertices: end of file")) {
      return PolySet::createEmpty();
    }

    boost::split(words, line, boost::is_any_of(" \t"), boost::token_compress_on);
    if (words.size() < 3) {
      AsciiError("can't parse vertex: not enough data");
      return PolySet::createEmpty();
    }

    try {
      Vector3d v = {0, 0, 0};
      int i;
      for (i = 0; i < dimension; i++) {
        v[i]= boost::lexical_cast<double>(words[i]);
      }
      //PRINTDB("Vertex[%ld] = { %f, %f, %f }", vertex % v[0] % v[1] % v[2]);
      if (has_normals) {
        ; // TODO words[i++]
        i += 0;
      }
      if (has_color) {
        ; // TODO: Meshlab appends color there, probably to allow gradients
        i += 3; // 4?
      }
      if (has_textures) {
        ; // TODO words[i++]
      }
      vertices.push_back(v);
    } catch (const boost::bad_lexical_cast& blc) {
      AsciiError("can't parse vertex: bad data");
      return PolySet::createEmpty();
    }
  }

  auto read_faces = [&](const std::function<void(const IndexedFace&, const std::optional<Color4f>& color)>& add_face) {
    IndexedFace indexed_face;
    while (!f.eof() && (face++ < faces_count)) {
      if (!getline_clean("reading faces: end of file")) {
        return false;
      }

      boost::split(words, line, boost::is_any_of(" \t"), boost::token_compress_on);
      if (words.size() < 1) {
        AsciiError("can't parse face: not enough data");
        return false;
      }

      try {
        unsigned long face_size=boost::lexical_cast<unsigned long>(words[0]);
        unsigned long i;
        if (words.size() - 1 < face_size) {
          AsciiError("can't parse face: missing indices");
          return false;
        }
        indexed_face.clear();
        //PRINTDB("Index[%d] [%d] = { ", face % n);
        for (i = 1; i <= face_size; i++) {
          int ind=boost::lexical_cast<int>(words[i]);
          //PRINTDB("%d, ", ind);
          if (ind >= 0 && ind < vertices_count) {
            indexed_face.push_back(ind);
          } else {
            AsciiError((boost::format("ignored bad face vertex index: %d") % ind).str().c_str());
          }
        }
        //PRINTD("}");
        std::optional<Color4f> color;
        if (words.size() >= face_size + 4) {
          i = face_size + 1;
          // handle optional color info (r g b [a])
          int r=boost::lexical_cast<int>(words[i++]);
          int g=boost::lexical_cast<int>(words[i++]);
          int b=boost::lexical_cast<int>(words[i++]);
          int a=i < words.size() ? boost::lexical_cast<int>(words[i++]) : 255;
          color = Color4f(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
        }
        add_face(indexed_face, color);
      } catch (const boost::bad_lexical_cast& blc) {
        AsciiError("can't parse face: bad data");
        return false;
      }
    }
    return true;
  };

  if (Feature::ExperimentalColors.is_enabled()) {
    manifold::MeshGL mesh;
    mesh.vertProperties.resize(vertices.size() * 3);
    for (size_t i = 0; i < vertices.size(); i++) {
      mesh.vertProperties[i * 3 + 0] = vertices[i][0];
      mesh.vertProperties[i * 3 + 1] = vertices[i][1];
      mesh.vertProperties[i * 3 + 2] = vertices[i][2];
    }
    mesh.triVerts.reserve(faces_count * 3);
    std::map<std::optional<Color4f>, std::vector<IndexedFace>> facesByColor;
    auto ok = read_faces([&](const IndexedFace& indexed_face, const std::optional<Color4f>& color) {
      facesByColor[color].push_back(indexed_face);
    });
    if (!ok) {
      return PolySet::createEmpty();
    }
    std::map<uint32_t, Color4f> colorMap;
    auto next_id = manifold::Manifold::ReserveIDs(facesByColor.size());
    for (const auto& [color, faces] : facesByColor) {
      mesh.runIndex.push_back(mesh.triVerts.size());
      auto id = next_id++;
      mesh.runOriginalID.push_back(id);
      if (color.has_value()) {
        colorMap[id] = color.value();
      }
      for (const auto& indexed_face : faces) {
        // Trianglate the face if needed
        auto len = indexed_face.size();
        for (size_t tri = 0, numTri = len - 2; tri < numTri; tri++) {
          mesh.triVerts.push_back(indexed_face[0]);
          mesh.triVerts.push_back(indexed_face[tri + 1]);
          mesh.triVerts.push_back(indexed_face[tri + 2]);
        }
      }
    }
    mesh.runIndex.push_back(mesh.triVerts.size());
    auto mani = std::make_unique<ManifoldGeometry>(std::make_shared<manifold::Manifold>(mesh), colorMap);
    if (!mani->isValid()) {
      LOG(message_group::Error, loc, "", "Imported geometry is not a valid manifold");
      return PolySet::createEmpty();
    }
    return mani;
  } else {
    auto ps = PolySet::createEmpty();
    ps->vertices.swap(vertices);
    ps->indices.reserve(faces_count);
    auto logged_color_warning = false;
    auto ok = read_faces([&](const IndexedFace& face, const std::optional<Color4f>& color) {
      if (color.has_value() && !logged_color_warning) {
        LOG(message_group::Warning, "Ignoring color information in OFF file (enable `colors` feature to read it).");
        logged_color_warning = true;
      }
      ps->indices.emplace_back(face);
    });
    if (!ok) {
      return PolySet::createEmpty();
    }
    return ps;
    //PRINTDB("PS: %ld vertices, %ld indices", ps->vertices.size() % ps->indices.size());
  }
}
