#include <cmath>
#include <cstdlib>
#include <elementary_visualizer/elementary_visualizer.hpp>
#include <framework.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <numbers>

namespace ev = elementary_visualizer;

int visualize_surface_datas(const std::vector<ev::SurfaceData> &surface_datas);

ev::SurfaceData field_to_surface_data(
    const int n_t,
    const float l_t,
    const size_t n_x,
    const float l_x,
    const float c,
    const std::vector<float> &phi_prev,
    const std::vector<float> &A_y,
    const std::vector<float> &A_y_prev_prev
)
{
    std::vector<ev::Vertex> vertices(n_x * n_x);

    const float dt = l_t / n_t;
    const float dx = l_x / n_x;

    std::vector<float> visualized_field = std::vector<float>(n_x * n_x);
    const int iwidth = n_x;
    for (int x = 0; x != iwidth; ++x)
    {
        for (int y = 0; y != iwidth; ++y)
        {
            const int x_minus_dx = (x == 0) ? (iwidth - 1) : (x - 1);
            const int x_plus_dx = (x == (iwidth - 1)) ? 0 : (x + 1);
            const int y_minus_dy = (y == 0) ? (iwidth - 1) : (y - 1);
            const int y_plus_dy = (y == (iwidth - 1)) ? 0 : (y + 1);
            const float E_x = -(phi_prev[y * iwidth + x_plus_dx] -
                                phi_prev[y * iwidth + x_minus_dx]) /
                              (2 * dx);
            const float E_y =
                -((phi_prev[y_plus_dy * iwidth + x] -
                   phi_prev[y_minus_dy * iwidth + x]) /
                      (2 * dx) +
                  (A_y[y * iwidth + x] - A_y_prev_prev[y * iwidth + x]) /
                      (2 * dt) / c);
            visualized_field[y * iwidth + x] = E_x * E_x + E_y * E_y;
        }
    }

    for (size_t i = 0; i < visualized_field.size(); ++i)
    {
        const float v = 600.0f * visualized_field[i];
        glm::vec4 color(v, v, v, 1.0f);

        size_t u = i % n_x;
        float x = 2.0f * static_cast<float>(u) / (n_x - 1) - 1.0f;
        float y = -2.0f * static_cast<float>((i - u) / n_x) / (n_x - 1) + 1.0f;
        glm::vec3 position(x, y, 0.0f);
        vertices[i] = ev::Vertex(position, color);
    }

    return ev::SurfaceData(vertices, n_x, ev::SurfaceMode::flat);
}

std::vector<float> iterate_potential(
    const int n_t,
    const float l_t,
    const size_t n_x,
    const float l_x,
    const float c,
    const float t,
    std::vector<float> field,
    std::vector<float> previous_field,
    bool electric_and_not_magnetic_potential
)
{
    // In Gaussian units the equations are
    // d_mu d^mu phi = 4 * pi * rho
    // d_mu d^mu A = 4 * pi * j / c.

    const float dt = l_t / n_t;

    const float dx = l_x / n_x;
    const float nu = c * dt / dx; // Courant number.
    const float nu_sq = nu * nu;

    const int iwidth = n_x;

    const float source_omega = 0.3f;
    const float source_v = 1.5f;
    const float source_amp =
        0.003f * (electric_and_not_magnetic_potential
                      ? 1.0f
                      : source_v * std::cos(source_omega * t) / c);
    const float source_x = static_cast<float>(iwidth) / 2.0f;
    const float source_y =
        source_x + (source_v / source_omega) * std::sin(source_omega * t);
    const float source_r = 4.0f;
    std::vector<float> source = std::vector<float>(n_x * n_x);
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

    std::vector<float> new_field = std::vector<float>(n_x * n_x);
    for (int x = 0; x != iwidth; ++x)
    {
        for (int y = 0; y != iwidth; ++y)
        {
            const int x_minus_dx = (x == 0) ? (iwidth - 1) : (x - 1);
            const int x_plus_dx = (x == (iwidth - 1)) ? 0 : (x + 1);
            const int y_minus_dy = (y == 0) ? (iwidth - 1) : (y - 1);
            const int y_plus_dy = (y == (iwidth - 1)) ? 0 : (y + 1);

            new_field[y * iwidth + x] =
                (nu_sq * (field[y * iwidth + x_minus_dx] +
                          field[y * iwidth + x_plus_dx] +
                          field[y_minus_dy * iwidth + x] +
                          field[y_plus_dy * iwidth + x] -
                          4.0f * field[y * iwidth + x]) +
                 2.0f * field[y * iwidth + x] -
                 previous_field[y * iwidth + x]) +
                source[y * iwidth + x];
        }
    }

    return new_field;
}

std::vector<ev::SurfaceData> generate_surface_datas(
    const int n_t,
    const float l_t,
    const size_t n_x,
    const float l_x,
    const float c
)
{
    // Electric scalar potential.
    std::vector<float> phi(n_x * n_x, 0.0f);
    std::vector<float> phi_prev(n_x * n_x, 0.0f);

    // Magnetic vector potential.
    std::vector<float> A_y(n_x * n_x, 0.0f);
    std::vector<float> A_y_prev(n_x * n_x, 0.0f);

    std::vector<ev::SurfaceData> surface_datas;
    const auto start_time = std::chrono::system_clock::now();
    for (int frame = 0; frame != n_t; ++frame)
    {
        const float t = static_cast<float>(frame) * l_t / (n_t - 1);

        std::vector<float> phi_next =
            iterate_potential(n_t, l_t, n_x, l_x, c, t, phi, phi_prev, true);
        phi_prev = phi;
        phi = phi_next;

        std::vector<float> A_y_next =
            iterate_potential(n_t, l_t, n_x, l_x, c, t, A_y, A_y_prev, false);
        std::vector<float> A_y_prev_prev = A_y_prev;
        A_y_prev = A_y;
        A_y = A_y_next;

        surface_datas.push_back(field_to_surface_data(
            n_t, l_t, n_x, l_x, c, phi_prev, A_y, A_y_prev_prev
        ));

        print_progress(static_cast<float>(frame) / (n_t - 1), start_time);
    }

    return surface_datas;
}

int main(int, char **)
{
    // Setup parameters of the simulation.

    const int n_t = 1000;
    const float l_t = 100.0f;

    const size_t n_x = 250;
    const float l_x = 250.0f;

    const float c = 2.0f;

    // Running the simulation.
    const std::vector<ev::SurfaceData> surface_datas =
        generate_surface_datas(n_t, l_t, n_x, l_x, c);

    // Visualizing the simulation.
    return visualize_surface_datas(surface_datas);
}

int visualize_surface_datas(const std::vector<ev::SurfaceData> &surface_datas)
{
    if (surface_datas.size() == 0)
        return EXIT_FAILURE;

    auto surface = ev::SurfaceVisual::create(surface_datas[0]);
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
        surface_datas.size(),
        frame_rate
    );
    if (!framework)
        return EXIT_FAILURE;
    framework.value()->add_visual(surface.value());

    return framework.value()->run(
        [&](const int frame, const int, const float)
        { surface.value()->set_surface_data(surface_datas[frame]); }
    );
}
