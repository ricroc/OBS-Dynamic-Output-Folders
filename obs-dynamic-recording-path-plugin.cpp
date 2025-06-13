// obs-dynamic-recording-path-plugin.cpp

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <ctime>
#include <string>
#include <sstream>
#include <filesystem>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-dynamic-path-plugin", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
    return "Dynamic Recording Path Plugin";
}

static std::string get_current_scene_name()
{
    obs_source_t *scene = obs_frontend_get_current_scene();
    const char *name = scene ? obs_source_get_name(scene) : "UnknownScene";
    std::string result = name;
    if (scene)
        obs_source_release(scene);
    return result;
}

static std::string get_date_string()
{
    std::time_t t = std::time(nullptr);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm);
    return buffer;
}

static std::string expand_path_template(const std::string &base_path)
{
    std::string scene = get_current_scene_name();
    std::string date = get_date_string();

    std::string suffix = "%DATE%/%SCENE%";
    size_t pos;
    while ((pos = suffix.find("%SCENE%")) != std::string::npos)
        suffix.replace(pos, 8, scene);
    while ((pos = suffix.find("%DATE%")) != std::string::npos)
        suffix.replace(pos, 7, date);

    std::filesystem::path full_path = std::filesystem::path(base_path) / suffix;
    std::filesystem::create_directories(full_path);
    return full_path.string();
}

static void on_frontend_event(enum obs_frontend_event event)
{
    if (event == OBS_FRONTEND_EVENT_RECORDING_STARTING)
    {
        obs_output_t *output = obs_frontend_get_recording_output();
        if (output)
        {
            const char *current_path = obs_output_get_path(output);
            std::string new_path = expand_path_template(current_path);
            obs_output_set_path(output, new_path.c_str());
            obs_output_release(output);
        }
    }
}

bool obs_module_load(void)
{
    blog(LOG_INFO, "Dynamic Recording Path Plugin loaded.");
    obs_frontend_add_event_callback(on_frontend_event, nullptr);
    return true;
}
