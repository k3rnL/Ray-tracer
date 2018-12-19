/**
 * @Author: daniel_b
 * @Date:   2017-09-12T22:43:55+02:00
 * @Last modified by:   daniel_b
 * @Last modified time: 2017-11-13T15:24:20+01:00
 */



#include "Map.hpp"

using namespace game;
using namespace fse;

Map::Map(scene::SceneManager &scn, size_t x, size_t y, float wide)
{
  _water = scene::object::Wavefront::load("Ressource/plan.obj");
  addChild(_water);
  _water->getMaterial()->setShader(ShaderManager::getInstance().addShader("water"));
  _water->setPosition(glm::vec3(0, -0.05, 0));
  _water->setScale(glm::vec3((float) x * wide / 2, 1, (float) y * wide / 2));
  // _water->getMaterial()->setColor(30, 50, 10);
  _water->getMaterial()->setTexture("Ressource/water.jpg");

  // getMaterial()->setShader(fse::ShaderManager::getInstance().addShader("basic_light_map"));

    _mesh = std::make_shared<fse::gl_item::Mesh>();
    createShape(x, y, wide);
}

void    Map::randomize()
{
    std::shared_ptr<fse::gl_item::Mesh> mesh = std::make_shared<fse::gl_item::Mesh>(*_mesh);

    auto &indices = _mesh->getIndices();
    auto &vertexes = _mesh->getVertexes();
    static std::default_random_engine e;
    static std::uniform_real_distribution<> dis(-0.5, 0.5);

    size_t new_mesh_index = 0;
    mesh->getIndices().clear();

    std::vector<glm::vec3>      new_vertex;
    std::vector<size_t>         new_vertex_index;

    for (size_t index = 0 ; index < indices.size() ; index += 3) {
        size_t face[4];

        face[0] = indices[index];
        face[1] = indices[index + 1];
        face[2] = indices[index + 2];

        turnFaceToFindLongestEdge(vertexes, face);

        glm::vec3 v4 = (vertexes[face[0]] + vertexes[face[1]]) / glm::vec3(2.0);
        if ((face[3] = getExistingVertexIndex(mesh->getVertexes(), v4)) == -1) {
            glm::vec3 normal = (_mesh->getNormals()[face[0]] + _mesh->getNormals()[face[1]]) / glm::vec3(2.0);
            glm::vec2 uv = (_mesh->getUVs()[face[0]] + _mesh->getUVs()[face[1]]) / glm::vec2(2.0);
            face[3] = mesh->addVertex(v4);
            mesh->addUV(uv);
            mesh->addNormal(normal);
            new_vertex_index.push_back(face[3]);
        }

        mesh->addFace({face[0], face[3], face[2]});
        mesh->addFace({face[3], face[1], face[2]});

        if (findTriangleByEdge(face[0], face[1], face, *_mesh)) {
            turnFaceToFindLongestEdge(vertexes, face);

            mesh->addFace({face[0], face[3], face[2]});
            mesh->addFace({face[3], face[1], face[2]});
        }

    }

    for (auto index : new_vertex_index) {
        mesh->getVertexes()[index][1] += dis(e);
    }
    mesh->finish();
    _mesh = mesh;
}

size_t  Map::getExistingVertexIndex(std::vector<glm::vec3> &vertexes, const glm::vec3 &v)
{
    for (size_t i = 0 ; i < vertexes.size() ; i++) {
        if (vertexes[i] == v)
            return (i);
    }
    return (-1);
}

bool    Map::findTriangleByEdge(size_t v1, size_t v2, size_t *face, fse::gl_item::Mesh &mesh)
{
    auto &indices = mesh.getIndices();

    for (size_t i = 0 ; i < indices.size() / 3 ; i += 3) {
        if (v1 == indices[i + 0] || v1 == indices[i + 1] || v1 == indices[i + 2])
            if (v2 == indices[i + 0] || v2 == indices[i + 1] || v2 == indices[i + 2])
            {
                face[0] = indices[i + 0];
                face[1] = indices[i + 1];
                face[2] = indices[i + 2];
                return (true);
            }
    }
    return (false);
}

void    Map::turnFaceToFindLongestEdge(std::vector<glm::vec3> &vertexes, size_t *face)
{
    for (char i = 0 ; i < 3 ; i++) {
        float d1 = glm::distance(vertexes[face[0]], vertexes[face[1]]);
        float d2 = glm::distance(vertexes[face[0]], vertexes[face[2]]);
        float d3 = glm::distance(vertexes[face[1]], vertexes[face[2]]);

        float d = glm::max(d1, glm::max(d2, d3));
        if (d1 == d)
            break;

        float tmp = face[0];
        face[0] = face[2];
        face[2] = face[1];
        face[1] = tmp;
    }
}

void    Map::createShape(size_t size_x, size_t size_y, float wide)
{
    for (size_t x = 0 ; x < size_x ; x++) {
        for (size_t y = 0 ; y < size_y ; y++) {
            _mesh->addVertex({x * wide, 0, y * wide});
            _mesh->addNormal({0, 1, 0});
            _mesh->addUV({(float) x / (size_x - 1), (float) y / (size_y - 1)});
        }
    }

    for (size_t x = 1 ; x < size_x ; x++) {
        for (size_t y = 1 ; y < size_y ; y++) {
            _mesh->addFace({x - 1 + (y - 1) * size_x, x + (y - 1) * size_x, x + y * size_x});
            _mesh->addFace({x - 1 + (y - 1) * size_x, x + y * size_x, x - 1 + y * size_x});
        }
    }

    _mesh->finish();
}
