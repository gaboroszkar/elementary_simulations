#include <framework.hpp>

ev::Expected<std::shared_ptr<Framework>, ev::Error> Framework::create(
    const std::string &file_name,
    const unsigned int bit_rate,
    const glm::uvec2 video_size,
    const glm::uvec2 window_size,
    const glm::vec4 background_color,
    const int frames,
    const int frame_rate,
    const std::optional<int> samples_video,
    const int depth_peeling_passes_video,
    const std::optional<int> samples_window,
    const int depth_peeling_passes_window
)
{
    ev::Expected<std::shared_ptr<ev::Scene>, ev::Error> video_scene =
        ev::Scene::create(
            video_size,
            background_color,
            samples_video,
            depth_peeling_passes_video
        );
    if (!video_scene)
        return ev::Unexpected<ev::Error>(ev::Error());

    ev::Expected<std::shared_ptr<ev::Scene>, ev::Error> window_scene =
        ev::Scene::create(
            window_size,
            background_color,
            samples_window,
            depth_peeling_passes_window
        );
    if (!window_scene)
        return ev::Unexpected<ev::Error>(ev::Error());
    ev::Expected<std::shared_ptr<ev::Window>, ev::Error> window =
        ev::Window::create("Window", window_size, false);
    if (!window)
        return ev::Unexpected<ev::Error>(ev::Error());

    const int slider_position = 32;
    const float slider_width = 5.0f;

    ev::Expected<std::shared_ptr<Slider>, ev::Error> slider = Slider::create(
        glm::ivec2(slider_position, -slider_position),
        glm::ivec2(-slider_position, -slider_position),
        slider_width,
        1.25f * slider_width,
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        window_scene.value(),
        window.value()
    );
    if (!slider)
        return ev::Unexpected<ev::Error>(ev::Error());

    return std::shared_ptr<Framework>(new Framework(
        file_name,
        bit_rate,
        std::move(video_scene.value()),
        std::move(window_scene.value()),
        std::move(window.value()),
        slider_position,
        slider.value(),
        frames,
        frame_rate
    ));
}

void Framework::add_visual(std::shared_ptr<ev::Visual> visual)
{
    this->video_scene->add_visual(visual);
    this->window_scene->add_visual(visual);
}

int Framework::run(
    std::function<void(const int, const int, const float)> run_function
)
{
    while (!this->window->should_close_or_invalid())
    {
        if (this->recording && (this->frame + 1 >= this->frames))
        {
            this->recording = std::nullopt;
        }

        this->update_slider();

        run_function(this->frame, this->frames, this->t());

        this->window->render(this->window_scene->render());
        if (this->recording)
        {
            this->recording.value()->render(this->video_scene->render());
            ++this->frame;
        }

        ev::poll_window_events();
    }

    return EXIT_SUCCESS;
}

Framework::Framework(Framework &&other)
    : video_scene(std::move(other.video_scene)),
      window_scene(std::move(other.window_scene)),
      window(std::move(other.window)),
      slider_position(other.slider_position),
      slider(other.slider),
      frames(other.frames),
      frame_rate(other.frame_rate),
      recording(other.recording),
      frame(other.frame),
      mouse_position(other.mouse_position)
{
    this->setup_events();
}

Framework &Framework::operator=(Framework &&other)
{
    this->video_scene = std::move(other.video_scene);
    this->window_scene = std::move(other.window_scene);
    this->window = std::move(other.window);
    this->slider_position = other.slider_position;
    this->slider = other.slider;
    this->frames = other.frames;
    this->frame_rate = other.frame_rate;
    this->recording = other.recording;
    this->frame = other.frame;
    this->mouse_position = other.mouse_position;

    this->setup_events();

    return *this;
}

Framework::Framework(
    std::string file_name,
    unsigned int bit_rate,
    std::shared_ptr<ev::Scene> video_scene,
    std::shared_ptr<ev::Scene> window_scene,
    std::shared_ptr<ev::Window> window,
    int slider_position,
    std::shared_ptr<Slider> slider,
    int frames,
    int frame_rate
)
    : file_name(file_name),
      bit_rate(bit_rate),
      video_scene(video_scene),
      window_scene(window_scene),
      window(std::move(window)),
      slider_position(slider_position),
      slider(slider),
      frames(frames),
      frame_rate(frame_rate),
      recording(std::nullopt),
      frame(0),
      slider_drag(false),
      mouse_position(0.0f)
{
    this->setup_events();
    this->slider->update();
}

void Framework::setup_events()
{
    this->window->on_keyboard_event(
        [&](const ev::EventAction action,
            const ev::Key key,
            const ev::ModifierKey)
        {
            if (action == ev::EventAction::press && key == ev::Key::q)
            {
                this->window->destroy();
            }
            else if (action == ev::EventAction::press && key == ev::Key::r)
            {
                if (!this->recording)
                {
                    ev::Expected<std::shared_ptr<ev::Video>, ev::Error> video =
                        ev::Video::create(
                            this->file_name,
                            this->video_scene->get_size(),
                            this->frame_rate,
                            this->bit_rate,
                            AV_CODEC_ID_AV1,
                            false
                        );
                    if (video)
                    {
                        this->recording = video.value();
                        this->frame = 0;
                        this->slider_drag = false;
                        this->slider->color_0 =
                            glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
                    }
                }
                else
                {
                    this->recording = std::nullopt;
                }
            }
        }
    );

    this->window->on_mouse_button_event(
        [&](const ev::EventAction action,
            const ev::MouseButton button,
            const ev::ModifierKey)
        {
            const glm::uvec2 window_size = this->window->get_size();
            if (button == ev::MouseButton::left)
            {
                if (action == ev::EventAction::press)
                {
                    if (!this->recording &&
                        (this->mouse_position.y + 2 * this->slider_position) >
                            window_size.y)
                    {
                        this->slider_drag = true;
                        this->update_frame_by_mouse_position();
                    }
                }
                else if (action == ev::EventAction::release)
                {
                    this->slider_drag = false;
                }
            }
        }
    );

    this->window->on_mouse_move_event(
        [&](const glm::vec2 mouse_position_in)
        {
            this->mouse_position = mouse_position_in;

            if (this->slider_drag)
                this->update_frame_by_mouse_position();
        }
    );
}

void Framework::update_frame_by_mouse_position()
{
    const glm::uvec2 window_size = this->window->get_size();

    const float slider_frame_width =
        static_cast<float>(window_size.x - 2 * this->slider_position) /
        (this->frames - 1);
    const float mouse_relative_x =
        this->mouse_position.x - this->slider_position;
    this->frame = roundf(mouse_relative_x / slider_frame_width);
    if (this->frame < 0)
        this->frame = 0;
    if (this->frame > (frames - 1))
        this->frame = frames - 1;
}

void Framework::update_slider()
{
    if (this->recording)
        this->slider->color_0 = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    else
        this->slider->color_0 = glm::vec4(0.1f, 0.3f, 0.8f, 1.0f);

    this->slider->t = this->t();
    this->slider->update();
}

float Framework::t() const
{
    return static_cast<float>(this->frame) / (this->frames - 1);
}
