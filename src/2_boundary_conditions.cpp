#include <cstdlib>
#include <elementary_visualizer/elementary_visualizer.hpp>
#include <framework.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace ev = elementary_visualizer;

class Field
{
public:

    Field(size_t size_x, size_t size_y)
        : size(glm::uvec2(size_x, size_y)), field(size_x * size_y)
    {}

    Field(glm::uvec2 size) : size(size), field(size.x * size.y) {}

    size_t index(const int x, const int y) const
    {
        return Field::mod(y, this->size.y) * this->size.x +
               Field::mod(x, this->size.x);
    }

    size_t index(const glm::ivec2 &i) const
    {
        return index(i.x, i.y);
    }

    float operator()(const int x, const int y) const
    {
        return this->field[this->index(x, y)];
    }

    float &operator()(const int x, const int y)
    {
        return this->field[this->index(x, y)];
    }

    glm::uvec2 get_size() const
    {
        return this->size;
    }

private:

    static size_t mod(int n, const int m)
    {
        n = n % m;
        if (n < 0)
            n += m;
        return static_cast<size_t>(n);
    }

    glm::uvec2 size;
    std::vector<float> field;
};

Field operator*(float c, Field field)
{
    const glm::vec2 size = field.get_size();
    for (size_t x = 0; x != size.x; ++x)
    {
        for (size_t y = 0; y < size.y; ++y)
        {
            field(x, y) *= c;
        }
    }
    return field;
}

Field operator+(Field field_0, const Field &field_1)
{
    const glm::vec2 size = field_0.get_size();
    for (size_t x = 0; x != size.x; ++x)
    {
        for (size_t y = 0; y < size.y; ++y)
        {
            field_0(x, y) += field_1(x, y);
        }
    }
    return field_0;
}

float interp(
    float t, float min, float max, float t_min = 0.0f, float t_max = 1.0f
)
{
    t = (t - t_min) / (t_max - t_min);
    return min + (max - min) * t;
}

template <typename T>
T runge_kutta_iteration(
    const float t, const T &y, std::function<T(float, T)> f, const float h
)
{
    T k_1 = f(t, y);
    T k_2 = f(t + 0.5f * h, y + (0.5f * h) * k_1);
    T k_3 = f(t + 0.5f * h, y + (0.5f * h) * k_2);
    T k_4 = f(t + h, y + h * k_3);
    return y + (h / 6.0f) * (k_1 + 2.0f * k_2 + 2.0f * k_3 + k_4);
}

struct FieldState
{
    FieldState(const Field &amp, const Field &vel) : amp(amp), vel(vel) {}

    Field amp;
    Field vel;
};

FieldState operator*(float c, const FieldState &field_state)
{
    return FieldState(c * field_state.amp, c * field_state.vel);
}

FieldState
    operator+(const FieldState &field_state_0, const FieldState &field_state_1)
{
    return FieldState(
        field_state_0.amp + field_state_1.amp,
        field_state_0.vel + field_state_1.vel
    );
}

const std::vector<std::pair<float, glm::vec4>> &colormap_amplitude()
{
    // static std::vector<std::pair<float, glm::vec4>> colormap = {
    //     std::make_pair(-1.00f, glm::vec4(0.30f, 0.13f, 0.45f, 1.00f)),
    //     std::make_pair(-0.33f, glm::vec4(0.18f, 0.44f, 0.56f, 1.00f)),
    //     std::make_pair(+0.00f, glm::vec4(1.00f, 1.00f, 1.00f, 1.00f)),
    //     std::make_pair(+0.33f, glm::vec4(0.16f, 0.69f, 0.50f, 1.00f)),
    //     std::make_pair(+1.00f, glm::vec4(0.74f, 0.87f, 0.15f, 1.00f))};
    static std::vector<std::pair<float, glm::vec4>> colormap = {
        std::make_pair(-1.00f, glm::vec4(0.00f, 0.40f, 0.70f, 1.00f)),
        std::make_pair(+0.00f, glm::vec4(0.00f, 0.60f, 0.90f, 1.00f)),
        std::make_pair(+1.00f, glm::vec4(0.30f, 0.70f, 1.00f, 1.00f))};

    return colormap;
}

glm::vec4
    to_color(float v, const std::vector<std::pair<float, glm::vec4>> &colormap)
{
    if (v < colormap[0].first)
        return colormap[0].second;
    if (v > colormap.back().first)
        return colormap.back().second;

    std::optional<std::pair<float, glm::vec4>> color_begin;
    std::optional<std::pair<float, glm::vec4>> color_end;
    for (const auto &color_value : colormap)
    {
        if (color_value.first <= v)
        {
            color_begin = color_value;
        }
        else if (!color_end)
        {
            color_end = color_value;
            break;
        }
    }

    if (color_begin && color_end)
    {
        return glm::vec4(
            interp(
                v,
                color_begin->second.r,
                color_end->second.r,
                color_begin->first,
                color_end->first
            ),
            interp(
                v,
                color_begin->second.g,
                color_end->second.g,
                color_begin->first,
                color_end->first
            ),
            interp(
                v,
                color_begin->second.b,
                color_end->second.b,
                color_begin->first,
                color_end->first
            ),
            interp(
                v,
                color_begin->second.a,
                color_end->second.a,
                color_begin->first,
                color_end->first
            )
        );
    }

    return glm::vec4();
}

ev::SurfaceData
    field_state_to_surface_data(const FieldState &field_state, const bool side)
{
    const glm::uvec2 size(field_state.amp.get_size());
    const size_t y_shift = side ? 0 : (size.y - 1) / 2;
    std::vector<ev::Vertex> vertices(size.x * (size.y + 1) / 2);
    for (size_t x = 0; x != size.x; ++x)
    {
        for (size_t y = 0; y < (size.y + 1) / 2; ++y)
        {
            glm::vec4 color =
                to_color(field_state.amp(x, y + y_shift), colormap_amplitude());

            float fx =
                4.0f * (1.0f * static_cast<float>(x) / (size.x - 1) - 0.5f);
            float fy =
                4.0f *
                (1.0f * static_cast<float>(y + y_shift) / (size.y - 1) - 0.5f);
            glm::vec3 position(fx, fy, 0.1f * field_state.amp(x, y + y_shift));
            vertices[y * size.x + x] = ev::Vertex(position, color);
        }
    }

    return ev::SurfaceData(vertices, size.x, ev::SurfaceMode::smooth);
}

FieldState
    iterate_field(const float t, const FieldState &state, const bool boundary)
{
    const float c = 1.0f;
    const float dx = 0.01f;
    const float rx = c * c / (dx * dx);
    const float dy = dx;
    const float ry = c * c / (dy * dy);

    const glm::uvec2 size(state.amp.get_size());
    Field acc(state.amp.get_size());

    for (size_t x = 0; x != size.x; ++x)
    {
        for (size_t y = 0; y != size.y; ++y)
        {
            float fx = static_cast<float>(x) / (size.x - 1) - 0.5f;
            float fy = static_cast<float>(y) / (size.y - 1) - 0.5f;

            fx += 0.4f;
            const float tt = t * 25.0f;
            float source = sinf(tt * 2 * std::numbers::pi) * 2700.0f *
                           expf(-750.0f * (fx * fx + fy * fy));
            if (tt > 4.0f)
                source = 0.0f;

            float amp_plus_dx =
                (x != (size.x - 1))
                    ? state.amp(x + 1, y)
                    // Only outgoing wave.
                    // : state.amp(x + 1, y) - 2 * dx * state.vel(x, y) / c;
                    : (boundary ?
                                // Dirichlet.
                           0.0f
                                // Neumann.
                                : state.amp(x, y));
            float amp_minus_dx =
                (x != 0) ? state.amp(x - 1, y)
                         : state.amp(x + 1, y) - 2 * dx * state.vel(x, y) / c;

            float amp_plus_dy =
                (y != (size.y - 1))
                    ? state.amp(x, y + 1)
                    : state.amp(x, y - 1) - 2.0f * dy * state.vel(x, y) / c;
            float amp_minus_dy = (y != 0) ? state.amp(x, y - 1)
                                          : state.amp(x, y + 1) -
                                                2.0f * dy * state.vel(x, y) / c;
            // float amp_plus_dy = state.amp(x, y + 1);
            // float amp_minus_dy = state.amp(x, y - 1);

            acc(x, y) = rx * (amp_minus_dx + amp_plus_dx) +
                        ry * (amp_minus_dy + amp_plus_dy) -
                        2.0f * (rx + ry) * state.amp(x, y) + source;
        }
    }

    return FieldState(state.vel, acc);
}

FieldState iterate_field_0(const float t, const FieldState &state)
{
    return iterate_field(t, state, false);
}

FieldState iterate_field_1(const float t, const FieldState &state)
{
    return iterate_field(t, state, true);
}

std::shared_ptr<ev::SurfaceVisual> create_surface()
{
    auto surface =
        ev::SurfaceVisual::create(ev::SurfaceData(std::vector<ev::Vertex>(), 0)
        );
    if (!surface)
        return nullptr;
    surface.value()->set_ambient_color(glm::vec3(0.2f));
    surface.value()->set_diffuse_color(glm::vec3(1.0f));
    surface.value()->set_specular_color(glm::vec3(0.5f));
    surface.value()->set_shininess(8.0f);
    surface.value()->set_light_position(glm::vec3(10.0f, 10.0f, 10.0f));

    const glm::vec3 eye = glm::vec3(2.25f, -2.25f, 2.5f);
    const glm::vec3 center = glm::vec3(0.75f, -0.55f, 0.0f);
    const glm::vec3 up(0.0f, 0.0f, 1.0f);
    const glm::mat4 view = glm::lookAt(eye, center, up);
    surface.value()->set_view(view);

    const float fov = 45.0f;
    const float near = 0.01f;
    const float far = 200.0f;
    glm::mat4 projection = glm::perspective(fov, 1.0f, near, far);
    surface.value()->set_projection(projection);

    surface.value()->set_model(glm::mat4(1.0f));

    return surface.value();
}

int main(int, char **)
{
    const int frames = 1000;
    const size_t width = 201;
    // const float dt = 0.01f;
    const float dt = 0.005;

    Field field(width, width);

    for (size_t x = 0; x != width; ++x)
    {
        for (size_t y = 0; y != width; ++y)
        {
            // const float fx = static_cast<float>(x) / (width - 1) - 0.5f;
            // const float fy = static_cast<float>(y) / (width - 1) - 0.5f;
            // field(x, y) = 10.0f * expf(-750.0f * (fx * fx + fy * fy));
            field(x, y) = 0.0f;
        }
    }

    FieldState field_state_0(field, field);
    FieldState field_state_1(field, field);

    std::cout << std::endl << "Generating fields..." << std::endl << std::endl;

    std::vector<ev::SurfaceData> surface_datas_0(
        frames, ev::SurfaceData(std::vector<ev::Vertex>(), 0)
    );
    std::vector<ev::SurfaceData> surface_datas_1(
        frames, ev::SurfaceData(std::vector<ev::Vertex>(), 0)
    );
    const auto start_time = std::chrono::system_clock::now();
    for (int frame = 0; frame != frames; ++frame)
    {
        const float t = static_cast<float>(frame) / (frames - 1);

        surface_datas_0[frame] =
            field_state_to_surface_data(field_state_0, false);
        field_state_0 = runge_kutta_iteration<FieldState>(
            t, field_state_0, iterate_field_0, dt
        );
        surface_datas_1[frame] =
            field_state_to_surface_data(field_state_1, true);
        field_state_1 = runge_kutta_iteration<FieldState>(
            t, field_state_1, iterate_field_1, dt
        );

        print_progress(t, start_time);
    }

    std::cout << std::endl;

    std::string file_name("2_boundary_conditions.webm");
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
        frame_rate,
        4,
        1,
        2,
        1
    );
    if (!framework)
        return EXIT_FAILURE;

    auto surface_0 = create_surface();
    if (!surface_0)
        return EXIT_FAILURE;
    auto surface_1 = create_surface();
    if (!surface_1)
        return EXIT_FAILURE;
    framework.value()->add_visual(surface_0);
    framework.value()->add_visual(surface_1);

    int run_result = framework.value()->run(
        [&](const int frame, const int, const float)
        {
            surface_0->set_surface_data(surface_datas_0[frame]);
            surface_1->set_surface_data(surface_datas_1[frame]);
        }
    );

    return run_result;
}
