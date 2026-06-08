#include <cmath>
#include <cstdlib>
#include <elementary_visualizer/elementary_visualizer.hpp>
#include <framework.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <numbers>

namespace ev = elementary_visualizer;

int visualize_field(
    const std::vector<std::vector<float>> &visualized_fields, const size_t n_x
);

std::vector<float> potentials_to_visualized_field(
    const int n_t,
    const float l_t,
    const size_t n_x,
    const float l_x,
    const float c,
    const int sponge_width,
    const std::vector<float> &phi_prev,
    const std::vector<float> &A_y,
    const std::vector<float> &A_y_prev_prev
)
{
    const float dt = l_t / n_t;
    const float dx = l_x / n_x;

    // We cut off the sponge from the visualized field.
    std::vector<float> visualized_field =
        std::vector<float>((n_x - 2 * sponge_width) * (n_x - 2 * sponge_width));
    const int iwidth = n_x;
    for (int x = 0; x != iwidth - 2 * sponge_width; ++x)
    {
        for (int y = sponge_width; y != iwidth - sponge_width; ++y)
        {
            const int x_minus_dx = (x == 0) ? 0 : (x - 1);
            const int x_plus_dx = (x == (iwidth - 1)) ? (iwidth - 1) : (x + 1);
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
            const int visualized_field_index =
                (y - sponge_width) * (iwidth - 2 * sponge_width) + x;
            //visualized_field[visualized_field_index] = E_x;
            //visualized_field[visualized_field_index] = E_y;
            visualized_field[visualized_field_index] = E_x * E_x + E_y * E_y;
        }
    }

    return visualized_field;
}

ev::SurfaceData visualized_field_to_surface_data(
    const std::vector<float> &visualized_field, const size_t n_x
)
{
    std::vector<ev::Vertex> vertices(n_x * n_x);

    for (size_t i = 0; i < visualized_field.size(); ++i)
    {
        //const float v = 40.0f * visualized_field[i] + 0.5f;
        const float v = 20000.0f * visualized_field[i];
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
    bool electric_and_not_magnetic_potential,
    const int sponge_width
)
{
    // In (slightly modified) Gaussian units the equations are
    // d_mu d^mu phi = rho
    // d_mu d^mu A = j / c.
    // Note, here x means the radial coordinate.

    const float dt = l_t / n_t;

    const float dx = l_x / n_x;
    const float nu = c * dt / dx; // Courant number.
    const float nu_sq = nu * nu;

    const int iwidth = n_x;

    const float source_omega = 0.1f;
    const float source_phase = std::numbers::pi / 2.0f;
    const float source_v = 0.5f;
    const float source_amp =
        (t < 0.1f * l_t ? t / (0.1f * l_t) : 1.0f) * 3.0f *
        (electric_and_not_magnetic_potential
             ? 1.0f
             : source_v * std::cos(source_omega * t + source_phase) / c);
    const float source_movement_amp = source_v / source_omega;
    const float source_x = 0.0f;
    const float source_y = l_x / 2.0f;
    const float source_dy =
        source_movement_amp * std::sin(source_omega * t + source_phase);
    const float source_r = 7.5f;
    std::vector<float> source = std::vector<float>(n_x * n_x);
    for (int x = 0; x != iwidth; ++x)
    {
        for (int y = 0; y != iwidth; ++y)
        {
            const float r1 = std::sqrt(
                std::pow(source_x - x * dx, 2) +
                std::pow((source_y + source_dy) - y * dx, 2)
            );
            const float r2 = std::sqrt(
                std::pow(source_x - x * dx, 2) +
                std::pow((source_y - source_dy) - y * dx, 2)
            );
            const float sigma = 0.3;
            const float sq2 = std::sqrt(2.0f);
            const float sq2pi = std::sqrt(2.0f * std::numbers::pi);
            if (r1 <= source_r)
            {
                source[y * iwidth + x] +=
                    source_amp *
                    std::exp(-std::pow(r1 / source_r / (sq2 * sigma), 2)) /
                    (sigma * sq2pi);
            }
            if (r2 <= source_r)
            {
                source[y * iwidth + x] -=
                    (electric_and_not_magnetic_potential ? 1.0f : -1.0f) *
                    source_amp *
                    std::exp(-std::pow(r2 / source_r / (sq2 * sigma), 2)) /
                    (sigma * sq2pi);
            }
        }
    }

    std::vector<float> new_field = std::vector<float>(n_x * n_x);
    for (int x = 0; x != iwidth; ++x)
    {
        for (int y = 0; y != iwidth; ++y)
        {
            const int x_minus_dx = (x == 0) ? 0 : (x - 1);
            const int x_plus_dx = (x == (iwidth - 1)) ? (iwidth - 1) : (x + 1);
            const int y_minus_dy = (y == 0) ? (iwidth - 1) : (y - 1);
            const int y_plus_dy = (y == (iwidth - 1)) ? 0 : (y + 1);

            const int sponge_r = std::min(
                std::abs(l_x - x * dx),
                std::min(std::abs(y * dx), std::abs(l_x - y * dx))
            );
            const float sponge_sigma =
                0.00225f *
                std::pow(
                    std::max(
                        0.0f, 10.0f * (1.0f - sponge_r / (sponge_width * dx))
                    ),
                    2
                );
            const float sponge_gamma = sponge_sigma * dt / 2.0f;

            // In the calculations, 1/r appear, and at r=0 it should blow up.
            // We could use the L'hopital rule and f(r)=f(-r) assumptions to get
            // a different update function at r=0, but that solution is very unstable,
            // at r=0, and creates some numerical instability waves there.
            const float x_shifted = x + 0.5f;
            new_field[y * iwidth + x] =
                (1.0f / (1.0f + sponge_gamma)) *
                    (nu_sq * ((1.0f + 1.0f / (2.0f * x_shifted)) *
                                  field[y * iwidth + x_plus_dx] +
                              (1.0f - 1.0f / (2.0f * x_shifted)) *
                                  field[y * iwidth + x_minus_dx] +
                              field[y_minus_dy * iwidth + x] +
                              field[y_plus_dy * iwidth + x]) +
                     2.0f * (1.0f - 2.0f * nu_sq) * field[y * iwidth + x] -
                     (1.0f - sponge_gamma) * previous_field[y * iwidth + x]) +
                source[y * iwidth + x] * dt * dt * c * c;
        }
    }

    return new_field;
}

std::vector<std::vector<float>> generate_visualized_field(
    const int n_t,
    const float l_t,
    const size_t n_x,
    const float l_x,
    const float c,
    const int sponge_width
)
{
    // Electric scalar potential.
    std::vector<float> phi(n_x * n_x, 0.0f);
    std::vector<float> phi_prev(n_x * n_x, 0.0f);

    // Magnetic vector potential.
    std::vector<float> A_y(n_x * n_x, 0.0f);
    std::vector<float> A_y_prev(n_x * n_x, 0.0f);

    std::vector<std::vector<float>> visualized_fields;
    const auto start_time = std::chrono::system_clock::now();
    for (int frame = 0; frame != n_t; ++frame)
    {
        const float t = static_cast<float>(frame) * l_t / (n_t - 1);

        std::vector<float> phi_next = iterate_potential(
            n_t, l_t, n_x, l_x, c, t, phi, phi_prev, true, sponge_width
        );
        phi_prev = phi;
        phi = phi_next;

        std::vector<float> A_y_next = iterate_potential(
            n_t, l_t, n_x, l_x, c, t, A_y, A_y_prev, false, sponge_width
        );
        std::vector<float> A_y_prev_prev = A_y_prev;
        A_y_prev = A_y;
        A_y = A_y_next;

        visualized_fields.push_back(potentials_to_visualized_field(
            n_t, l_t, n_x, l_x, c, sponge_width, phi_prev, A_y, A_y_prev_prev
        ));

        print_progress(static_cast<float>(frame) / (n_t - 1), start_time);
    }

    return visualized_fields;
}

int main(int, char **)
{
    // Setup parameters of the simulation.

    const int n_t = 1500;
    const float l_t = 600.0f;

    const size_t n_x = 700;
    const float l_x = 1000.0f;

    const float c = 2.0f;

    const int sponge_width = 120;

    // Running the simulation.
    const std::vector<std::vector<float>> visualized_fields =
        generate_visualized_field(n_t, l_t, n_x, l_x, c, sponge_width);

    // Visualizing the simulation.
    return visualize_field(visualized_fields, n_x - 2 * sponge_width);
}

int visualize_field(
    const std::vector<std::vector<float>> &visualized_fields, const size_t n_x
)
{
    if (visualized_fields.size() == 0)
        return EXIT_FAILURE;

    auto surface = ev::SurfaceVisual::create(
        visualized_field_to_surface_data(visualized_fields[0], n_x)
    );
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
    glm::uvec2 video_size(1080, 1080);
    glm::uvec2 window_size(720, 720);
    auto framework = Framework::create(
        file_name,
        bit_rate,
        video_size,
        window_size,
        glm::vec4(1.0f),
        visualized_fields.size(),
        frame_rate
    );
    if (!framework)
        return EXIT_FAILURE;
    framework.value()->add_visual(surface.value());

    return framework.value()->run(
        [&](const int frame, const int, const float)
        {
            surface.value()->set_surface_data(
                visualized_field_to_surface_data(visualized_fields[frame], n_x)
            );
        }
    );
}
