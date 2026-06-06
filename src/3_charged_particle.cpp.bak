#include <cmath>
#include <cstdlib>
#include <elementary_visualizer/elementary_visualizer.hpp>
#include <framework.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <numbers>

namespace ev = elementary_visualizer;

ev::SurfaceData field_to_surface_data(
    const size_t width,
    const std::vector<float> &previous_electric_potential,
    const std::vector<float> &magnetic_potential,
    const std::vector<float> &previous_magnetic_potential
)
{
    std::vector<ev::Vertex> vertices(width * width);

    const float dt = 0.1f;
    const float dy = 1.0f;
    const float dx = 1.0f;

    std::vector<float> new_field = std::vector<float>(width * width);
    const int iwidth = width;
    for (int x = 0; x != iwidth; ++x)
    {
        for (int y = 0; y != iwidth; ++y)
        {
            const int x_minus_dx = (x == 0) ? (iwidth - 1) : (x - 1);
            const int x_plus_dx = (x == (iwidth - 1)) ? 0 : (x + 1);
            const int y_minus_dy = (y == 0) ? (iwidth - 1) : (y - 1);
            const int y_plus_dy = (y == (iwidth - 1)) ? 0 : (y + 1);
            new_field[y * iwidth + x] = 0.0f;
            //new_field[y * iwidth + x] +=
            //    (electric_potential[y_plus_dy * iwidth + x] -
            //     electric_potential[y_minus_dy * iwidth + x]) /
            //    (2 * dy);
            const float E_y =
                (((previous_electric_potential[y_plus_dy * iwidth + x] -
                   previous_electric_potential[y_minus_dy * iwidth + x]) /
                  (dy)) +
                 ((magnetic_potential[y * iwidth + x] -
                   previous_magnetic_potential[y * iwidth + x]) /
                  (2 * dt)));
            const float E_x =
                ((previous_electric_potential[y * iwidth + x_plus_dx] -
                  previous_electric_potential[y * iwidth + x_minus_dx]) /
                 (dx));
            new_field[y * iwidth + x] = E_y * E_y + E_x * E_x;
        }
    }

    for (size_t i = 0; i < new_field.size(); ++i)
    {
        //const float v =
        //    1.0f - 0.5f * (0.5f + 5.0f * new_field[i] * new_field[i]);
        const float v = 600.0f * new_field[i];
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

std::vector<float> iterate_potential(
    const size_t width,
    std::vector<float> field,
    std::vector<float> previous_field,
    int frame,
    bool electric_and_not_magnetic_potential
)
{
    const float c = 2.0f;
    const float dt = 0.1f;

    const float dx = 1.0f;
    const float rx = c * dt / dx;
    const float rx_sq = rx * rx;

    const float dy = 1.0f;
    const float ry = c * dt / dy;
    const float ry_sq = ry * ry;

    const float t = static_cast<float>(frame) * dt;

    const int iwidth = width;

    const float source_amp = 0.003f * (electric_and_not_magnetic_potential
                                           ? 1.0f
                                           : 1.5f * std::cos(0.3f * t) / c);
    const float source_x = static_cast<float>(iwidth) / 2.0f;
    const float source_y = source_x + 5.0f * std::sin(0.3f * t);
    const float source_r = 4.0f;
    std::vector<float> source = std::vector<float>(width * width);
    for (int x = 0; x != iwidth; ++x)
    {
        for (int y = 0; y != iwidth; ++y)
        {
            const float r = std::sqrt(
                std::pow(source_x - x, 2) + std::pow(source_y - y, 2)
            );
            if (r <= source_r)
            {
                source[y * iwidth + x] = source_amp * (1.0f - r / source_r);
            }
        }
    }

    std::vector<float> new_field = std::vector<float>(width * width);
    for (int x = 0; x != iwidth; ++x)
    {
        for (int y = 0; y != iwidth; ++y)
        {
            const int x_minus_dx = (x == 0) ? (iwidth - 1) : (x - 1);
            const int x_plus_dx = (x == (iwidth - 1)) ? 0 : (x + 1);
            const int y_minus_dy = (y == 0) ? (iwidth - 1) : (y - 1);
            const int y_plus_dy = (y == (iwidth - 1)) ? 0 : (y + 1);

            const int sponge_r = std::min(
                std::min(std::abs(x), std::abs(iwidth - x)),
                std::min(std::abs(y), std::abs(iwidth - y))
            );
            const float sponge_width = 30.0f;
            const float sponge_sigma =
                0.01f *
                std::pow(
                    std::max(0.0f, 10.0f * (1.0f - sponge_r / sponge_width)), 2
                );
            const float sponge_gamma = sponge_sigma * dt / 2.0f;

            new_field[y * iwidth + x] =
                (1.0f / (1.0f + sponge_gamma)) *
                    (rx_sq * (field[y * iwidth + x_minus_dx] +
                              field[y * iwidth + x_plus_dx]) +
                     ry_sq * (field[y_minus_dy * iwidth + x] +
                              field[y_plus_dy * iwidth + x]) +
                     2.0f * (1.0f - rx_sq - ry_sq) * field[y * iwidth + x] -
                     (1.0f - sponge_gamma) * previous_field[y * iwidth + x]) +
                source[y * iwidth + x];
        }
    }

    return new_field;
}

int main(int, char **)
{
    const int frames = 1000;

    const size_t width = 300;
    std::vector<float> current_electric_potential(width * width, 0.0f);
    std::vector<float> current_magnetic_potential(width * width, 0.0f);

    std::vector<float> previous_electric_potential = current_electric_potential;
    std::vector<float> previous_magnetic_potential = current_magnetic_potential;

    const auto start_time = std::chrono::system_clock::now();
    std::vector<ev::SurfaceData> surface_datas;
    for (int frame = 0; frame != frames; ++frame)
    {
        const float t = static_cast<float>(frame) / (frames - 1);

        std::vector<float> new_electric_potential = iterate_potential(
            width,
            current_electric_potential,
            previous_electric_potential,
            frame,
            true
        );
        std::vector<float> previous_previous_electric_potential =
            previous_electric_potential;
        previous_electric_potential = current_electric_potential;
        current_electric_potential = new_electric_potential;

        std::vector<float> new_magnetic_potential = iterate_potential(
            width,
            current_magnetic_potential,
            previous_magnetic_potential,
            frame,
            false
        );
        std::vector<float> previous_previous_magnetic_potential =
            previous_magnetic_potential;
        previous_magnetic_potential = current_magnetic_potential;
        current_magnetic_potential = new_magnetic_potential;

        surface_datas.push_back(field_to_surface_data(
            width,
            previous_electric_potential,
            current_magnetic_potential,
            previous_previous_magnetic_potential
        ));

        print_progress(t, start_time);
    }

    auto surface = ev::SurfaceVisual::create(field_to_surface_data(
        width,
        previous_electric_potential,
        current_magnetic_potential,
        previous_magnetic_potential
    ));
    if (!surface)
        return EXIT_FAILURE;
    surface.value()->set_ambient_color(glm::vec3(1.0f));
    surface.value()->set_diffuse_color(glm::vec3(0.0f));
    surface.value()->set_specular_color(glm::vec3(0.0f));
    surface.value()->set_shininess(1.0f);

    surface.value()->set_view(glm::mat4(1.0f));
    glm::mat4 projection = glm::ortho(-1.0f, +1.0f, -1.0f, +1.0f);
    surface.value()->set_projection(projection);

    std::string file_name("3_charged_particle.mp4");
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
