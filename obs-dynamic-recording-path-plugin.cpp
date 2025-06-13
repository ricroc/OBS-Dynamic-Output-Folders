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

static std::string selected_source_name;
static bool use_active_source = true;

static std::string get_active_source_name()
{
    obs_source_t *scene = obs_frontend_get_current_scene();
    if (!scene)
        return "UnknownScene";

    obs_scene_t *scene_data = obs_scene_from_source(scene);
    if (!scene_data)
        return "UnknownScene";

    obs_sceneitem_t *item = obs_scene_get_sceneitem(scene_data, 0);
    if (!item)
        return "UnknownSource";

    obs_source_t *source = obs_sceneitem_get_source(item);
    if (!source)
        return "UnknownSource";

    const char *name = obs_source_get_name(source);
    return name ? name : "UnknownSource";
}

static std::string get_selected_source_name()
{
    return use_active_source ? get_active_source_name() : (selected_source_name.empty() ? "UnknownSource" : selected_source_name);
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
    std::string source = get_selected_source_name();
    std::string date = get_date_string();

    std::string suffix = "%DATE%/%SOURCE%";
    size_t pos;
    while ((pos = suffix.find("%SOURCE%")) != std::string::npos)
        suffix.replace(pos, 9, source);
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

static bool source_filter(void *data, obs_source_t *source)
{
    return obs_source_get_type(source) == OBS_SOURCE_TYPE_INPUT;
}

static void dynamic_path_properties_modified(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
    const char *name = obs_data_get_string(settings, "selected_source");
    if (name)
        selected_source_name = name;
    use_active_source = obs_data_get_bool(settings, "use_active_source");
}

static obs_properties_t *dynamic_path_properties(void *data)
{
    obs_properties_t *props = obs_properties_create();

    obs_properties_add_bool(props, "use_active_source", "Use Active Source Automatically");

    obs_property_t *p = obs_properties_add_list(props, "selected_source",
        "Fallback Source for Folder Naming", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

    obs_property_list_add_sources(p, source_filter, nullptr);
    obs_property_set_modified_callback(p, dynamic_path_properties_modified);

    return props;
}

bool obs_module_load(void)
{
    blog(LOG_INFO, "Dynamic Recording Path Plugin loaded.");
    obs_frontend_add_event_callback(on_frontend_event, nullptr);
    obs_register_module_config_path("obs-dynamic-path-plugin");
    obs_register_module_properties(dynamic_path_properties);
    return true;
}
