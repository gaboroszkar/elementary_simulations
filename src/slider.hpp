#ifndef SIMULATION_VISUALIZATIONS_SLIDER_HPP
#define SIMULATION_VISUALIZATIONS_SLIDER_HPP

#include <cstdlib>
#include <elementary_visualizer/elementary_visualizer.hpp>

namespace ev = elementary_visualizer;

class Slider
{
public:

    static ev::Expected<std::shared_ptr<Slider>, ev::Error> create(
        const glm::ivec2 &start,
        const glm::ivec2 &end,
        const float line_width,
        const float circle_radius,
        const glm::vec4 &color_0,
        const glm::vec4 &color_1,
        std::shared_ptr<ev::Scene> scene,
        std::shared_ptr<ev::Window> window
    );

    static glm::vec2 absolute_position(
        const glm::vec2 &relative_position, const glm::vec2 window_size
    );

    void update();

    Slider(Slider &&other);
    Slider &operator=(Slider &&other);

    Slider(const Slider &) = delete;
    Slider &operator=(const Slider &) = delete;

private:

    Slider(
        const glm::ivec2 &start,
        const glm::ivec2 &end,
        const glm::vec4 &color_0,
        const glm::vec4 &color_1,
        const float line_width,
        std::shared_ptr<ev::LinesegmentsVisual> linesegments,
        const float circle_radius,
        std::shared_ptr<ev::CircleVisual> circle,
        std::shared_ptr<ev::Window> window
    );

    glm::ivec2 start;
    glm::ivec2 end;
    float line_width;
    std::shared_ptr<ev::LinesegmentsVisual> linesegments;
    float circle_radius;
    std::shared_ptr<ev::CircleVisual> circle;
    std::shared_ptr<ev::Window> window;

public:

    glm::vec4 color_0;
    glm::vec4 color_1;
    float t;
};

#endif
