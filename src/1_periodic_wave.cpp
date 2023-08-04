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
    static std::vector<std::pair<float, glm::vec4>> colormap = {
        std::make_pair(-1.00f, glm::vec4(0.35f, 0.10f, 0.10f, 1.00f)),
        std::make_pair(-0.66f, glm::vec4(1.00f, 0.20f, 0.40f, 1.00f)),
        std::make_pair(-0.33f, glm::vec4(1.00f, 0.80f, 0.70f, 1.00f)),
        std::make_pair(+0.00f, glm::vec4(1.00f, 1.00f, 1.00f, 1.00f)),
        std::make_pair(+0.33f, glm::vec4(0.55f, 0.75f, 0.85f, 1.00f)),
        std::make_pair(+0.66f, glm::vec4(0.15f, 0.30f, 0.55f, 1.00f)),
        std::make_pair(+1.00f, glm::vec4(0.00f, 0.20f, 0.25f, 1.00f))};

    return colormap;
}

const std::vector<std::pair<float, glm::vec4>> &colormap_energy()
{
    static std::vector<std::pair<float, glm::vec4>> colormap = {
        std::make_pair(+0.00f, glm::vec4(0.00f, 0.00f, 0.15f, 1.00f)),
        std::make_pair(+256.00f, glm::vec4(1.00f, 0.70f, 0.00f, 1.00f)),
        std::make_pair(+512.00f, glm::vec4(1.00f, 1.00f, 1.00f, 1.00f))};

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
    field_state_to_surface_data(const FieldState &field_state, bool show_energy)
{
    const glm::uvec2 size(field_state.amp.get_size());
    std::vector<ev::Vertex> vertices(size.x * size.y);
    for (size_t x = 0; x != size.x; ++x)
    {
        for (size_t y = 0; y < size.y; ++y)
        {
            glm::vec4 color;
            if (show_energy)
            {
                const float c = 1.0f;
                const float dx = 0.005f;
                const float dy = dx;
                const float dadx =
                    (field_state.amp(x + 1, y) - field_state.amp(x - 1, y)) /
                    (2 * dx);
                const float dady =
                    (field_state.amp(x, y + 1) - field_state.amp(x, y - 1)) /
                    (2 * dy);
                const float energy =
                    0.5f * (field_state.vel(x, y) * field_state.vel(x, y) +
                            c * c * (dadx * dadx + dady * dady));
                color = to_color(energy, colormap_energy());
            }
            else
            {
                color = to_color(field_state.amp(x, y), colormap_amplitude());
            }

            float fx = 2.0f * static_cast<float>(x) / (size.x - 1) - 1.0f;
            float fy = 2.0f * static_cast<float>(y) / (size.y - 1) - 1.0f;
            glm::vec3 position(fx, fy, 0.0f);
            vertices[field_state.amp.index(x, y)] = ev::Vertex(position, color);
        }
    }

    return ev::SurfaceData(vertices, size.x, ev::SurfaceData::Mode::smooth);
}

FieldState iterate_field(const float, const FieldState &state)
{
    const float c = 1.0f;
    const float dx = 0.005f;
    const float rx = c * c / (dx * dx);
    const float dy = dx;
    const float ry = c * c / (dy * dy);

    const glm::uvec2 size(state.amp.get_size());
    Field acc(state.amp.get_size());

    for (size_t x = 0; x != size.x; ++x)
    {
        for (size_t y = 0; y != size.y; ++y)
        {
            acc(x, y) = rx * (state.amp(x - 1, y) + state.amp(x + 1, y)) +
                        ry * (state.amp(x, y - 1) + state.amp(x, y + 1)) -
                        2.0f * (rx + ry) * state.amp(x, y);
        }
    }

    return FieldState(state.vel, acc);
}

int main(int, char **)
{
    const int frames = 1650;
    const size_t width = 300;
    const float dt = 0.005f;
    const bool show_energy = false;

    Field field(width, width);

    for (size_t x = 0; x != width; ++x)
    {
        for (size_t y = 0; y != width; ++y)
        {
            const float fx = static_cast<float>(x) / (width - 1) - 0.5f;
            const float fy = static_cast<float>(y) / (width - 1) - 0.5f;
            field(x, y) = 10.0f * expf(-750.0f * (fx * fx + fy * fy));
        }
    }

    FieldState field_state(field, field);

    std::vector<ev::SurfaceData> surface_datas(
        frames, ev::SurfaceData(std::vector<ev::Vertex>(), 0)
    );
    for (int frame = 0; frame != frames; ++frame)
    {
        surface_datas[frame] =
            field_state_to_surface_data(field_state, show_energy);
        field_state = runge_kutta_iteration<FieldState>(
            0.0f, field_state, iterate_field, dt
        );

        if (frame % 50 == 0)
            std::cout << frame << std::endl;
    }

    std::vector<std::shared_ptr<ev::SurfaceVisual>> surfaces;
    for (int i = -1; i != 2; ++i)
    {
        auto surface = ev::SurfaceVisual::create(
            ev::SurfaceData(std::vector<ev::Vertex>(), 0)
        );
        if (!surface)
            return EXIT_FAILURE;
        surface.value()->set_ambient_color(glm::vec3(1.0f));
        surface.value()->set_diffuse_color(glm::vec3(0.0f));
        surface.value()->set_specular_color(glm::vec3(0.0f));
        surface.value()->set_shininess(0.0f);

        surface.value()->set_view(glm::mat4(1.0f));
        const glm::mat4 projection = glm::ortho(-1.0f, +1.0f, -1.0f, +1.0f);
        surface.value()->set_projection(projection);

        const glm::mat4 model =
            glm::translate(glm::mat4(1.0f), glm::vec3(i * 2.0f, 0.0f, 0.0f));
        surface.value()->set_model(model);

        surfaces.push_back(surface.value());
    }

    std::string file_name(
        show_energy ? "1_periodic_wave_energy.webm"
                    : "1_periodic_wave_amplitude.webm"
    );
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
        std::nullopt,
        1
    );
    if (!framework)
        return EXIT_FAILURE;

    for (auto surface : surfaces)
        framework.value()->add_visual(surface);

    int run_result = framework.value()->run(
        [&](const int frame, const int, const float)
        {
            for (auto surface : surfaces)
                surface->set_surface_data(surface_datas[frame]);
        }
    );

    return run_result;
}
