#ifndef SIMULATION_VISUALIZATIONS_FRAMEWORK_HPP
#define SIMULATION_VISUALIZATIONS_FRAMEWORK_HPP

#include <chrono>
#include <elementary_visualizer/elementary_visualizer.hpp>
#include <slider.hpp>

namespace ev = elementary_visualizer;

void print_progress(
    const float t, const std::chrono::system_clock::time_point start_time
);

struct Recording
{
    std::shared_ptr<ev::Video> video;
    std::chrono::system_clock::time_point start_time;
};

class Framework
{
public:

    static ev::Expected<std::shared_ptr<Framework>, ev::Error> create(
        const std::string &file_name,
        const unsigned int bit_rate,
        const glm::uvec2 video_size,
        const glm::uvec2 window_size,
        const glm::vec4 background_color,
        const int frames,
        const int frame_rate,
        const std::optional<int> samples_video = 4,
        const int depth_peeling_passes_video = 4,
        const std::optional<int> samples_window = 4,
        const int depth_peeling_passes_window = 2
    );

    void add_visual(std::shared_ptr<ev::Visual> visual);

    int run(std::function<void(const int, const int, const float)> run_function
    );

    Framework(Framework &&other);

    Framework &operator=(Framework &&other);

    Framework(const Framework &) = delete;
    Framework &operator=(const Framework &) = delete;

private:

    Framework(
        std::string file_name,
        unsigned int bit_rate,
        std::shared_ptr<ev::Scene> video_scene,
        std::shared_ptr<ev::Scene> window_scene,
        std::shared_ptr<ev::Window> window,
        int slider_position,
        std::shared_ptr<Slider> slider,
        int frames,
        int frame_rate
    );

    void setup_events();

    void update_frame_by_mouse_position();

    void update_slider();

    float t() const;

    std::string file_name;
    unsigned int bit_rate;
    std::shared_ptr<ev::Scene> video_scene;
    std::shared_ptr<ev::Scene> window_scene;
    std::shared_ptr<ev::Window> window;
    int slider_position;
    std::shared_ptr<Slider> slider;
    int frames;
    int frame_rate;
    std::optional<Recording> recording;
    int frame;
    bool slider_drag;
    glm::vec2 mouse_position;
};

#endif
