#include <glm/gtc/matrix_transform.hpp>
#include <slider.hpp>

ev::Expected<std::shared_ptr<Slider>, ev::Error> Slider::create(
    const glm::ivec2 &start,
    const glm::ivec2 &end,
    const float line_width,
    const float circle_radius,
    const glm::vec4 &color_0,
    const glm::vec4 &color_1,
    std::shared_ptr<ev::Scene> scene,
    std::shared_ptr<ev::Window> window
)
{
    ev::Expected<std::shared_ptr<ev::LinesegmentsVisual>, ev::Error>
        linesegments = ev::LinesegmentsVisual::create(
            std::vector<ev::Linesegment>(), ev::LineCap::round
        );
    if (!linesegments)
        return ev::Unexpected<ev::Error>(ev::Error());
    linesegments.value()->set_view(glm::mat4(1.0f));
    linesegments.value()->set_model(glm::mat4(1.0f));
    linesegments.value()->set_projection_aspect_correction(false);
    scene->add_visual(linesegments.value());

    ev::Expected<std::shared_ptr<ev::CircleVisual>, ev::Error> circle =
        ev::CircleVisual::create(color_0);
    if (!circle)
        return ev::Unexpected<ev::Error>(ev::Error());
    circle.value()->set_view(glm::mat4(1.0f));
    circle.value()->set_model(glm::mat4(1.0f));
    circle.value()->set_projection_aspect_correction(false);
    scene->add_visual(circle.value());

    return std::shared_ptr<Slider>(new Slider(
        start,
        end,
        color_0,
        color_1,
        line_width,
        linesegments.value(),
        circle_radius,
        circle.value(),
        window
    ));
}

glm::vec2 Slider::absolute_position(
    const glm::vec2 &relative_position, const glm::vec2 window_size
)
{
    glm::vec2 p = relative_position;
    if (p.x < 0.0f)
        p.x += window_size.x;
    if (p.y < 0.0f)
        p.y += window_size.y;
    return p;
}

void Slider::update()
{
    const glm::vec2 window_size = this->window->get_size();

    const glm::vec2 start_absolute =
        this->absolute_position(this->start, window_size);
    const glm::vec2 end_absolute =
        this->absolute_position(this->end, window_size);

    if (this->t < 0.0f)
        this->t = 0.0f;
    if (this->t > 1.0f)
        this->t = 1.0f;

    const glm::vec2 middle =
        start_absolute + this->t * (end_absolute - start_absolute);

    const float z = 0.99f;

    std::vector<ev::Linesegment> linesegments_data(2);
    linesegments_data[0] = ev::Linesegment(
        ev::Vertex(glm::vec3(start_absolute, z), this->color_0),
        ev::Vertex(glm::vec3(middle, z), this->color_0),
        this->line_width
    );
    linesegments_data[1] = ev::Linesegment(
        ev::Vertex(glm::vec3(middle, z), this->color_1),
        ev::Vertex(glm::vec3(end_absolute, z), this->color_1),
        this->line_width
    );
    linesegments->set_linesegments_data(linesegments_data);

    const glm::mat4 circle_model_0 =
        glm::translate(glm::mat4(1.0f), glm::vec3(middle, z + 0.001f));
    const glm::mat4 circle_model_1 =
        glm::scale(circle_model_0, glm::vec3(this->circle_radius));
    circle->set_model(circle_model_1);
    circle->set_color(this->color_0);

    glm::mat4 projection = glm::ortho(0.0f, window_size.x, window_size.y, 0.0f);
    linesegments->set_projection(projection);
    circle->set_projection(projection);
}

Slider::Slider(
    const glm::ivec2 &start,
    const glm::ivec2 &end,
    const glm::vec4 &color_0,
    const glm::vec4 &color_1,
    const float line_width,
    std::shared_ptr<ev::LinesegmentsVisual> linesegments,
    const float circle_radius,
    std::shared_ptr<ev::CircleVisual> circle,
    std::shared_ptr<ev::Window> window
)
    : start(start),
      end(end),
      line_width(line_width),
      linesegments(linesegments),
      circle_radius(circle_radius),
      circle(circle),
      window(window),
      color_0(color_0),
      color_1(color_1),
      t(0.0f)
{
    this->update();
}
