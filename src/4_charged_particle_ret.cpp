#include <cmath>
#include <cstdlib>
#include <elementary_visualizer/elementary_visualizer.hpp>
#include <framework.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <numbers>

namespace ev = elementary_visualizer;

int visualize_field(
    const std::vector<std::vector<float>> &visualized_fields, const size_t n_r
);

std::vector<float> potentials_to_visualized_field(
    const int n_t,
    const float l_t,
    const size_t n_r,
    const float l_r,
    const float c,
    const std::vector<float> &phi_prev,
    const std::vector<float> &A_z,
    const std::vector<float> &A_z_prev_prev
)
{
    const float dt = l_t / n_t;
    const float dr = l_r / n_r;

    std::vector<float> visualized_field = std::vector<float>(n_r * n_r);
    const int iwidth = n_r;
    for (int r = 0; r != iwidth; ++r)
    {
        for (int z = 0; z != iwidth; ++z)
        {
            const int r_minus_dr = (r == 0) ? 0 : (r - 1);
            const int r_plus_dr = (r == (iwidth - 1)) ? (iwidth - 1) : (r + 1);
            const int z_minus_dz = (z == 0) ? (iwidth - 1) : (z - 1);
            const int z_plus_dz = (z == (iwidth - 1)) ? 0 : (z + 1);
            const float E_r = -(phi_prev[z * iwidth + r_plus_dr] -
                                phi_prev[z * iwidth + r_minus_dr]) /
                              (2 * dr);
            const float E_z =
                -((phi_prev[z_plus_dz * iwidth + r] -
                   phi_prev[z_minus_dz * iwidth + r]) /
                      (2 * dr) +
                  (A_z[z * iwidth + r] - A_z_prev_prev[z * iwidth + r]) /
                      (2 * dt) / c);
            //visualized_field[z * iwidth + r] = E_r;
            //visualized_field[z * iwidth + r] = E_z;
            //visualized_field[z * iwidth + r] = E_r * E_r + E_z * E_z;
            visualized_field[z * iwidth + r] = phi_prev[z * iwidth + r];
        }
    }

    return visualized_field;
}

ev::SurfaceData visualized_field_to_surface_data(
    const std::vector<float> &visualized_field, const size_t n_r
)
{
    std::vector<ev::Vertex> vertices(n_r * n_r);

    for (size_t i = 0; i < visualized_field.size(); ++i)
    {
        //const float v = 40.0f * visualized_field[i] + 0.5f;
        //const float v = 20000.0f * visualized_field[i];

        const float v = 0.75f * visualized_field[i];

        glm::vec4 color(v, v, v, 1.0f);

        size_t u = i % n_r;
        float r = 2.0f * static_cast<float>(u) / (n_r - 1) - 1.0f;
        float z = -2.0f * static_cast<float>((i - u) / n_r) / (n_r - 1) + 1.0f;
        glm::vec3 position(r, z, 0.0f);
        vertices[i] = ev::Vertex(position, color);
    }

    return ev::SurfaceData(vertices, n_r, ev::SurfaceMode::flat);
}

std::vector<float> iterate_potential(
    const int n_t,
    const float l_t,
    const size_t n_r,
    const float l_r,
    const float c,
    const float t,
    std::vector<float> field,
    std::vector<float> previous_field,
    bool electric_and_not_magnetic_potential
)
{
    // In (slightly modified) Gaussian units the equations are
    // d_mu d^mu phi = rho
    // d_mu d^mu A = j / c.
    // (1/c^2 d^2/dt^2 - d^2/dx^2) phi = source.
    // (1/c^2 d^2/dt^2 - d^2/dx^2) A = 1/c source.
    // Then, our solution is
    // phi = 1/4pi * int rho(x', t') / r' d^x,
    // A = 1/4pic * int j(x', t') / r' d^x.

    const float dt = l_t / n_t;

    const float dr = l_r / n_r;
    const float nu = c * dt / dr; // Courant number.
    const float nu_sq = nu * nu;

    const int iwidth = n_r;

    const float source_omega = 0.1f;
    const float source_phase = std::numbers::pi / 2.0f;
    const float source_v = 0.5f;
    const float source_amp =
        3.0f * (electric_and_not_magnetic_potential
                    ? 1.0f
                    : source_v * std::cos(source_omega * t + source_phase) / c);
    const float source_movement_amp = source_v / source_omega;
    const float source_z = l_r / 2.0f;
    const float source_size = 7.5f;
    //std::vector<float> source = std::vector<float>(n_r * n_r);
    //for (int r = 0; r != iwidth; ++r)
    //{
    //    for (int z = 0; z != iwidth; ++z)
    //    {
    //        const float r1 = std::sqrt(
    //            std::pow(source_r - r * dr, 2) +
    //            std::pow((source_z + source_dz) - z * dr, 2)
    //        );
    //        const float r2 = std::sqrt(
    //            std::pow(source_r - r * dr, 2) +
    //            std::pow((source_z - source_dz) - z * dr, 2)
    //        );
    //        const float sigma = 0.3;
    //        const float sq2 = std::sqrt(2.0f);
    //        const float sq2pi = std::sqrt(2.0f * std::numbers::pi);
    //        if (r1 <= source_size)
    //        {
    //            source[z * iwidth + r] +=
    //                source_amp *
    //                std::exp(-std::pow(r1 / source_size / (sq2 * sigma), 2)) /
    //                (sigma * sq2pi);
    //        }
    //        if (r2 <= source_size)
    //        {
    //            source[z * iwidth + r] -=
    //                (electric_and_not_magnetic_potential ? 1.0f : -1.0f) *
    //                source_amp *
    //                std::exp(-std::pow(r2 / source_size / (sq2 * sigma), 2)) /
    //                (sigma * sq2pi);
    //        }
    //    }
    //}

    std::vector<float> new_field = std::vector<float>(n_r * n_r);
    for (int r_i = 0; r_i != iwidth; ++r_i)
    {
        for (int z_i = 0; z_i != iwidth; ++z_i)
        {
            for (int r_source_i = 0; r_source_i * dr <= source_size;
                 ++r_source_i)
            {
                for (int z_source_i = std::floor(
                         (source_z - source_movement_amp - source_size) / dr
                     );
                     z_source_i <=
                     std::ceil(
                         (source_z + source_movement_amp + source_size) / dr
                     );
                     ++z_source_i)
                {
                    const int num_theta = 4;
                    for (int theta_source_i = 0; theta_source_i != num_theta;
                         ++theta_source_i)
                    {
                        const float theta_source = theta_source_i * 2.0f *
                                                   std::numbers::pi / num_theta;
                        const float z_source = z_source_i * dr;
                        const float z = z_i * dr;
                        // The +0.5f is because in the field simulation (3_charged_particle.cpp)
                        // we shift the calculation at r=0 slightly for numerical stability, and
                        // we want to be consistent with that calculation.
                        const float r_source = (r_source_i + 0.5f) * dr;
                        const float r = (r_i + 0.5f) * dr;

                        const float x_source =
                            r_source * std::sin(theta_source);
                        const float y_source =
                            r_source * std::cos(theta_source);

                        const float x = r;
                        const float y = 0.0f;

                        const float distance = std::sqrt(
                            std::pow(x_source - x, 2) +
                            std::pow(y_source - y, 2) +
                            std::pow(z_source - z, 2)
                        );

                        // Preventing division by zero,
                        // because in the discretized version we
                        // no longer deal with charge densities and real space,
                        // but point charges and discretized space points.
                        if (distance >= 0.000001f)
                        {
                            const float delay = distance / c;
                            const float t_delayed = t - delay;

                            const float source_center_z_delayed =
                                source_z +
                                source_movement_amp *
                                    std::sin(
                                        source_omega * t_delayed + source_phase
                                    );
                            // Here we need to take into account the r which is
                            // unshifted by 0.5f, because that's how we setup the
                            // values in 3_charged_particle.cpp, and
                            // we want to be consistent with it.
                            const float distance_from_source_center = std::sqrt(
                                std::pow(r_source_i * dr, 2) +
                                std::pow(source_center_z_delayed - z_source, 2)
                            );
                            const float sigma = 0.3;
                            const float sq2 = std::sqrt(2.0f);
                            const float sq2pi =
                                std::sqrt(2.0f * std::numbers::pi);
                            const float source_density =
                                source_amp *
                                std::exp(-std::pow(
                                    distance_from_source_center / source_size /
                                        (sq2 * sigma),
                                    2
                                )) /
                                (sigma * sq2pi);

                            const float source_volume =
                                (2.0f * std::numbers::pi * r_source * dr /
                                 num_theta) *
                                dr;
                            const float source_in_volume =
                                source_volume * source_density;

                            new_field[z_i * iwidth + r_i] +=
                                source_in_volume / distance /
                                (4 * std::numbers::pi);
                        }
                    }
                }
            }
        }
    }

    return new_field;
}

std::vector<std::vector<float>> generate_visualized_field(
    const int n_t,
    const float l_t,
    const size_t n_r,
    const float l_r,
    const float c
)
{
    // Electric scalar potential.
    std::vector<float> phi(n_r * n_r, 0.0f);
    std::vector<float> phi_prev(n_r * n_r, 0.0f);

    // Magnetic vector potential.
    std::vector<float> A_z(n_r * n_r, 0.0f);
    std::vector<float> A_z_prev(n_r * n_r, 0.0f);

    std::vector<std::vector<float>> visualized_fields;
    const auto start_time = std::chrono::system_clock::now();
    for (int frame = 0; frame != n_t; ++frame)
    {
        const float t = static_cast<float>(frame) * l_t / (n_t - 1);

        std::vector<float> phi_next =
            iterate_potential(n_t, l_t, n_r, l_r, c, t, phi, phi_prev, true);
        phi_prev = phi;
        phi = phi_next;

        std::vector<float> A_z_next =
            iterate_potential(n_t, l_t, n_r, l_r, c, t, A_z, A_z_prev, false);
        std::vector<float> A_z_prev_prev = A_z_prev;
        A_z_prev = A_z;
        A_z = A_z_next;

        visualized_fields.push_back(potentials_to_visualized_field(
            n_t, l_t, n_r, l_r, c, phi_prev, A_z, A_z_prev_prev
        ));

        print_progress(static_cast<float>(frame) / (n_t - 1), start_time);
    }

    return visualized_fields;
}

int main(int, char **)
{
    // Setup parameters of the simulation.

    //const int n_t = 1500;
    //const float l_t = 600.0f;
    const int n_t = 30;
    const float l_t = 12.0f;

    //const size_t n_r = 700;
    //const float l_r = 1000.0f;
    //const int sponge_width = 120;

    //const size_t n_r = 700 - 2 * 120;
    //const float l_r = 1000.0f * n_r / 700;
    //const size_t n_r = 460;
    const size_t n_r = 300;
    const float l_r = 657.0f;

    const float c = 2.0f;

    // Running the simulation.
    const std::vector<std::vector<float>> visualized_fields =
        generate_visualized_field(n_t, l_t, n_r, l_r, c);

    // Visualizing the simulation.
    return visualize_field(visualized_fields, n_r);
}

int visualize_field(
    const std::vector<std::vector<float>> &visualized_fields, const size_t n_r
)
{
    if (visualized_fields.size() == 0)
        return EXIT_FAILURE;

    auto surface = ev::SurfaceVisual::create(
        visualized_field_to_surface_data(visualized_fields[0], n_r)
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

    std::string file_name("4_charged_particle_ret.mp4");
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
                visualized_field_to_surface_data(visualized_fields[frame], n_r)
            );
        }
    );
}
