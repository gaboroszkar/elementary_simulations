#include <cstdlib>
#include <elementary_visualizer/elementary_visualizer.hpp>
#include <framework.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace ev = elementary_visualizer;

ev::SurfaceData
    field_to_surface_data(const size_t width, const std::vector<float> &field)
{
    std::vector<ev::Vertex> vertices(field.size());

    for (size_t i = 0; i < field.size(); ++i)
    {
        const float v = 1.0f - 0.5f * (0.5f + field[i]);
        glm::vec4 color(v, v, v, 1.0f);

        size_t u = i % width;
        float x = 2.0f * static_cast<float>(u) / (width - 1) - 1.0f;
        float y =
            -2.0f * static_cast<float>((i - u) / width) / (width - 1) + 1.0f;
        glm::vec3 position(x, y, 0.0f);
        vertices[i] = ev::Vertex(position, color);
    }

    return ev::SurfaceData(vertices, width, ev::SurfaceMode::flat);
}

std::vector<float> iterate_field(
    const size_t width,
    std::vector<float> field,
    std::vector<float> previous_field
)
{
    const float c = 1.0f;
    const float dt = 0.001f;

    const float dx = 0.01f;
    const float rx = c * dt / dx;
    const float rx_sq = rx * rx;

    const float dy = 0.01f;
    const float ry = c * dt / dy;
    const float ry_sq = ry * ry;

    const int iwidth = width;

    std::vector<float> new_field = std::vector<float>(width * width);
    for (int x = 0; x != iwidth; ++x)
    {
        for (int y = 0; y != iwidth; ++y)
        {
            const int x_minus_dx = (x == 0) ? (iwidth - 1) : (x - 1);
            const int x_plus_dx = (x == (iwidth - 1)) ? 0 : (x + 1);
            const int y_minus_dy = (y == 0) ? (iwidth - 1) : (y - 1);
            const int y_plus_dy = (y == (iwidth - 1)) ? 0 : (y + 1);

            new_field[y * iwidth + x] =
                rx_sq * (field[y * iwidth + x_minus_dx] +
                         field[y * iwidth + x_plus_dx]) +
                ry_sq * (field[y_minus_dy * iwidth + x] +
                         field[y_plus_dy * iwidth + x]) +
                2.0f * (1.0f - rx_sq - ry_sq) * field[y * iwidth + x] -
                previous_field[y * iwidth + x];
        }
    }

    return new_field;
}

int main(int, char **)
{
    const int frames = 300;

    const size_t width = 200;
    std::vector<float> current_field(width * width, 0.0f);

    current_field[49 * width + 49] = 1.0f;
    current_field[49 * width + 50] = 1.0f;
    current_field[50 * width + 49] = 1.0f;
    current_field[50 * width + 50] = 1.0f;

    current_field[48 * width + 49] = 0.5f;
    current_field[48 * width + 50] = 0.5f;

    current_field[51 * width + 49] = 0.5f;
    current_field[51 * width + 50] = 0.5f;

    current_field[49 * width + 48] = 0.5f;
    current_field[50 * width + 48] = 0.5f;

    current_field[49 * width + 51] = 0.5f;
    current_field[50 * width + 51] = 0.5f;

    std::vector<float> previous_field = current_field;

    std::vector<ev::SurfaceData> surface_datas;
    for (int frame = 0; frame != frames; ++frame)
    {
        surface_datas.push_back(field_to_surface_data(width, current_field));
        std::vector<float> new_field =
            iterate_field(width, current_field, previous_field);
        previous_field = current_field;
        current_field = new_field;
    }

    auto surface =
        ev::SurfaceVisual::create(field_to_surface_data(width, current_field));
    if (!surface)
        return EXIT_FAILURE;
    surface.value()->set_ambient_color(glm::vec3(1.0f));
    surface.value()->set_diffuse_color(glm::vec3(0.0f));
    surface.value()->set_specular_color(glm::vec3(0.0f));
    surface.value()->set_shininess(0.0f);

    surface.value()->set_view(glm::mat4(1.0f));
    glm::mat4 projection = glm::ortho(-1.0f, +1.0f, -1.0f, +1.0f);
    surface.value()->set_projection(projection);

    std::string file_name("0_simulation.mp4");
    unsigned int bit_rate = 10000000;
    unsigned int frame_rate = 30;
    glm::uvec2 video_size(1920, 1080);
    glm::uvec2 window_size(1280, 720);
    auto framework = Framework::create(
        file_name,
        bit_rate,
        video_size,
        window_size,
        glm::vec4(1.0f),
        frames,
        frame_rate
    );
    if (!framework)
        return EXIT_FAILURE;
    framework.value()->add_visual(surface.value());

    int run_result = framework.value()->run(
        [&](const int frame, const int, const float)
        { surface.value()->set_surface_data(surface_datas[frame]); }
    );

    return run_result;
}
