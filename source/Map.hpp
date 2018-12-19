/**
 * @Author: daniel_b
 * @Date:   2017-09-12T22:39:54+02:00
 * @Last modified by:   daniel_b
 * @Last modified time: 2017-11-13T01:06:26+01:00
 */

#include <random>
#include <cstring>

#include <fse/Scene/SceneManager.hpp>
#include <fse/Scene/Object/Object.hpp>
#include <fse/Scene/Object/Wavefront.hpp>


namespace game {

    class Map : public fse::scene::object::Object
    {
    public:
        Map(fse::scene::SceneManager &scn, size_t size_x, size_t size_y, float wide = 1.0);

        void        randomize();
    private:
      fse::scene::object::Object  *_water;

        void        createShape(size_t size_x, size_t size_y, float wide);
        void        turnFaceToFindLongestEdge(std::vector<glm::vec3> &vertexes, size_t *face);
        bool        findTriangleByEdge(size_t v1, size_t v2, size_t *face, fse::gl_item::Mesh &mesh);
        size_t      getExistingVertexIndex(std::vector<glm::vec3> &vertexes, glm::vec3 const &v);
    };

}
